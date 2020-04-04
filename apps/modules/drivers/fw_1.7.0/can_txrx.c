

/*================================================================
 *
 *
 *   文件名称：can_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时07分55秒
 *   修改日期：2020年04月04日 星期六 18时49分56秒
 *   描    述：
 *
 *================================================================*/
#include "can_txrx.h"

#include "os_utils.h"
#include "usart_txrx.h"

static LIST_HEAD(can_info_list);
static osMutexId can_info_list_mutex = NULL;

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

typedef struct {
	CAN_HandleTypeDef *hcan;
	CAN_HandleTypeDef *config_can;
	uint32_t filter_number;
	uint32_t filter_fifo;
	uint8_t receive_fifo;
} can_config_t;

static can_config_t can_config_sz[] = {
	{
		.hcan = &hcan1,
		.filter_number = 0,
		.filter_fifo = CAN_FILTER_FIFO0,
		.receive_fifo = CAN_FIFO0,
		.config_can = &hcan1,
	},
	{
		.hcan = &hcan2,
		.filter_number = 14,
		.filter_fifo = CAN_FILTER_FIFO1,
		.receive_fifo = CAN_FIFO1,
		.config_can = &hcan1,
	}
};

static can_config_t *get_can_config(CAN_HandleTypeDef *hcan)
{
	uint8_t i;
	can_config_t *can_config = NULL;
	can_config_t *can_config_item = NULL;

	for(i = 0; i < (sizeof(can_config_sz) / sizeof(can_config_t)); i++) {
		can_config_item = can_config_sz + i;

		if(hcan == can_config_item->hcan) {
			can_config = can_config_item;
			break;
		}
	}

	return can_config;
}

static can_info_t *get_can_info(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = NULL;
	can_info_t *can_info_item = NULL;
	osStatus os_status;

	if(can_info_list_mutex == NULL) {
		return can_info;
	}

	os_status = osMutexWait(can_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(can_info_item, &can_info_list, can_info_t, list) {
		if(can_info_item->hcan == hcan) {
			can_info = can_info_item;
			break;
		}
	}

	os_status = osMutexRelease(can_info_list_mutex);

	if(os_status != osOK) {
	}

	return can_info;
}

static void receive_init(CAN_HandleTypeDef *hcan)
{
	CAN_FilterConfTypeDef filter;
	can_info_t *can_info = get_can_info(hcan);

	if(can_info == NULL) {
		return;
	}

	can_info->hcan->pRxMsg = &can_info->rx_msg;
	can_info->hcan->pRx1Msg = &can_info->rx_msg1;

	filter.FilterNumber = can_info->filter_number;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterIdHigh = 0x0000;
	filter.FilterIdLow = 0x0000;
	filter.FilterMaskIdHigh = 0x0000;
	filter.FilterMaskIdLow = 0x0000;
	filter.FilterFIFOAssignment = can_info->filter_fifo;
	filter.FilterActivation = ENABLE;
	filter.BankNumber = 14;

	HAL_CAN_ConfigFilter(can_info->config_can, &filter);
}

void free_can_info(can_info_t *can_info)
{
	osStatus os_status;

	if(can_info == NULL) {
		return;
	}

	list_del(&can_info->list);

	if(can_info->rx_msg_q) {
		os_status = osMessageDelete(can_info->rx_msg_q);

		if(osOK != os_status) {
		}
	}

	if(can_info->tx_msg_q) {
		os_status = osMessageDelete(can_info->tx_msg_q);

		if(osOK != os_status) {
		}
	}

	if(can_info->hcan_mutex) {
		os_status = osMutexDelete(can_info->hcan_mutex);

		if(osOK != os_status) {
		}
	}

	os_free(can_info);
}

can_info_t *get_or_alloc_can_info(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = NULL;
	can_config_t *can_config = NULL;
	osStatus os_status;

	osMutexDef(hcan_mutex);
	osMessageQDef(tx_msg_q, 1, uint16_t);
	osMessageQDef(rx_msg_q, 1, uint16_t);

	can_info = get_can_info(hcan);

	if(can_info != NULL) {
		return can_info;
	}

	if(can_info_list_mutex == NULL) {
		osMutexDef(can_info_list_mutex);
		can_info_list_mutex = osMutexCreate(osMutex(can_info_list_mutex));

		if(can_info_list_mutex == NULL) {
			return can_info;
		}
	}

	can_config = get_can_config(hcan);

	if(can_config == NULL) {
		return can_info;
	}

	can_info = (can_info_t *)os_alloc(sizeof(can_info_t));

	if(can_info == NULL) {
		return can_info;
	}

	can_info->hcan = can_config->hcan;
	can_info->config_can = can_config->config_can;
	can_info->filter_number = can_config->filter_number;
	can_info->filter_fifo = can_config->filter_fifo;
	can_info->receive_fifo = can_config->receive_fifo;
	can_info->hcan_mutex = osMutexCreate(osMutex(hcan_mutex));
	can_info->tx_msg_q = osMessageCreate(osMessageQ(tx_msg_q), NULL);
	can_info->rx_msg_q = osMessageCreate(osMessageQ(rx_msg_q), NULL);

	can_info->receive_init = receive_init;

	os_status = osMutexWait(can_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&can_info->list, &can_info_list);

	os_status = osMutexRelease(can_info_list_mutex);

	if(os_status != osOK) {
	}

	return can_info;
}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = get_can_info(hcan);
	osStatus os_status;

	if(can_info == NULL) {
		return;
	}

	if(can_info->tx_msg_q != NULL) {
		os_status = osMessagePut(can_info->tx_msg_q, 0, 0);

		if(os_status != osOK) {
		}
	}
}
void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = get_can_info(hcan);
	osStatus os_status;

	if(can_info == NULL) {
		return;
	}

	if(can_info->rx_msg_q != NULL) {
		os_status = osMessagePut(can_info->rx_msg_q, 0, 0);

		if(os_status != osOK) {
		}
	}
}
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
}

int can_tx_data(can_info_t *can_info, can_tx_msg_t *msg, uint32_t timeout)
{
	int ret = -1;
	HAL_StatusTypeDef status;
	//osStatus os_status;

	can_info->hcan->pTxMsg = msg;

	if(can_info->hcan_mutex) {
		//os_status = osMutexWait(can_info->hcan_mutex, osWaitForever);

		//if(os_status != osOK) {
		//}
	}

	status = HAL_CAN_Transmit_IT(can_info->hcan);

	if(status != HAL_OK) {
	}

	if(can_info->hcan_mutex) {
		//os_status = osMutexRelease(can_info->hcan_mutex);

		//if(os_status != osOK) {
		//}
	}

	if(can_info->tx_msg_q != NULL) {
		osEvent event = osMessageGet(can_info->tx_msg_q, timeout);

		if(event.status == osEventMessage) {
			ret = 0;
		} else {
		}
	}

	return ret;
}

int can_rx_data(can_info_t *can_info, uint32_t timeout)
{
	int ret = -1;
	//osStatus os_status;
	HAL_StatusTypeDef status;

	if(can_info == NULL) {
		return ret;
	}

	if(can_info->hcan_mutex) {
		//os_status = osMutexWait(can_info->hcan_mutex, osWaitForever);

		//if(os_status != osOK) {
		//}
	}

	status = HAL_CAN_Receive_IT(can_info->hcan, can_info->receive_fifo);

	if(status != HAL_OK) {//重新初始化
		HAL_CAN_DeInit(can_info->hcan);

		if(can_info->can_hal_init) {
			can_info->can_hal_init();
		}

		if(can_info->receive_init) {
			can_info->receive_init(can_info->hcan);
		}

		status = HAL_CAN_Receive_IT(can_info->hcan, can_info->receive_fifo);

		if(status != HAL_OK) {
		}
	}

	if(can_info->hcan_mutex) {
		//os_status = osMutexRelease(can_info->hcan_mutex);

		//if(os_status != osOK) {
		//}
	}

	if(can_info->rx_msg_q != NULL) {
		osEvent event = osMessageGet(can_info->rx_msg_q, timeout);

		if(event.status == osEventMessage) {
			ret = 0;
		} else {
			//can_transmit_dummy(can_info->hcan);
		}
	}

	return ret;
}

void set_can_info_hal_init(can_info_t *can_info, can_hal_init_t can_hal_init)
{
	can_info->can_hal_init = can_hal_init;
}

can_rx_msg_t *can_get_msg(can_info_t *can_info)
{
	can_rx_msg_t *rx_msg = NULL;

	if(can_info->receive_fifo == CAN_FIFO0) {
		rx_msg = &can_info->rx_msg;
	} else if((can_info->receive_fifo == CAN_FIFO1)) {
		rx_msg = &can_info->rx_msg1;
	}

	return rx_msg;
}
