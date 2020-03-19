

/*================================================================
 *
 *
 *   文件名称：can_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时07分55秒
 *   修改日期：2020年03月18日 星期三 11时14分53秒
 *   描    述：
 *
 *================================================================*/
#include "can_txrx.h"

#include "os_utils.h"
#include "usart_txrx.h"

static LIST_HEAD(can_info_list);

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

typedef struct {
	CAN_HandleTypeDef *hcan;
	CAN_HandleTypeDef *config_can;
	uint32_t filter_number;
	uint32_t filter_fifo;
	uint32_t receive_fifo;
} can_config_t;

static can_config_t can_config_sz[] = {
	{
		.hcan = &hcan1,
		.filter_number = 0,
		.filter_fifo = CAN_FILTER_FIFO0,
		.receive_fifo = CAN_IT_RX_FIFO0_MSG_PENDING,
		.config_can = &hcan1,
	},
	{
		.hcan = &hcan2,
		.filter_number = 14,
		.filter_fifo = CAN_FILTER_FIFO1,
		.receive_fifo = CAN_IT_RX_FIFO1_MSG_PENDING,
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

can_info_t *get_can_info(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = NULL;
	can_info_t *can_info_item = NULL;

	list_for_each_entry(can_info_item, &can_info_list, can_info_t, list) {
		if(can_info_item->hcan == hcan) {
			can_info = can_info_item;
			break;
		}
	}

	return can_info;
}

static void receive_init(CAN_HandleTypeDef *hcan)
{
	CAN_FilterTypeDef filter;
	can_info_t *can_info = get_can_info(hcan);
	HAL_StatusTypeDef status;

	if(can_info == NULL) {
		return;
	}

	filter.FilterBank = can_info->filter_number;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterIdHigh = 0x0000;
	filter.FilterIdLow = 0x0000;
	filter.FilterMaskIdHigh = 0x0000;
	filter.FilterMaskIdLow = 0x0000;
	filter.FilterFIFOAssignment = can_info->filter_fifo;
	filter.FilterActivation = ENABLE;
	filter.SlaveStartFilterBank = 14;

	HAL_CAN_ConfigFilter(can_info->config_can, &filter);

	status = HAL_CAN_ActivateNotification(can_info->hcan, can_info->receive_fifo);

	if(status != HAL_OK) {
		/* Notification Error */
	}

	status = HAL_CAN_Start(can_info->hcan);

	if (status != HAL_OK) {
		/* Start Error */
	}
}

void free_can_info(can_info_t *can_info)
{
	osStatus status;

	if(can_info == NULL) {
		return;
	}

	list_del(&can_info->list);

	if(can_info->rx_msg_q) {
		status = osMessageDelete(can_info->rx_msg_q);

		if(osOK != status) {
		}
	}

	if(can_info->tx_msg_q) {
		status = osMessageDelete(can_info->tx_msg_q);

		if(osOK != status) {
		}
	}

	if(can_info->hcan_mutex) {
		status = osMutexDelete(can_info->hcan_mutex);

		if(osOK != status) {
		}
	}

	os_free(can_info);
}

can_info_t *alloc_can_info(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = NULL;
	can_config_t *can_config = NULL;

	osMutexDef(hcan_mutex);
	osMessageQDef(tx_msg_q, 1, uint16_t);
	osMessageQDef(rx_msg_q, 1, uint16_t);

	can_info = get_can_info(hcan);

	if(can_info != NULL) {
		return can_info;
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

	list_add_tail(&can_info->list, &can_info_list);

	return can_info;
}

void can_rxfifo_pending_callback(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = get_can_info(hcan);
	HAL_StatusTypeDef status;

	if(can_info == NULL) {
		return;
	}

	status = HAL_CAN_DeactivateNotification(can_info->hcan, can_info->receive_fifo);

	if(status == HAL_OK) {
	}

	if(can_info->rx_msg_q != NULL) {
		osStatus status = osMessagePut(can_info->rx_msg_q, 0, 0);

		if(status != osOK) {
		}
	}

}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	can_rxfifo_pending_callback(hcan);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	can_rxfifo_pending_callback(hcan);
}

int can_tx_data(can_info_t *can_info, can_tx_msg_t *msg, uint32_t timeout)
{
	int ret = -1;
	uint32_t stamp = osKernelSysTick();
	HAL_StatusTypeDef status;
	//osStatus os_status;
	CAN_TxHeaderTypeDef tx_header;

	tx_header.StdId = msg->StdId;
	tx_header.ExtId = msg->ExtId;
	tx_header.RTR = msg->RTR;
	tx_header.IDE = msg->IDE;
	tx_header.DLC = msg->DLC;
	tx_header.TransmitGlobalTime = DISABLE;

	if(can_info->hcan_mutex) {
		//os_status = osMutexWait(can_info->hcan_mutex, osWaitForever);

		//if(os_status != osOK) {
		//}
	}

	status = HAL_BUSY;

	while(status != HAL_OK) {
		status = HAL_CAN_AddTxMessage(can_info->hcan, &tx_header, msg->Data, &msg->tx_mailbox);

		if(osKernelSysTick() - stamp >= timeout) {
			break;
		}

		osDelay(1);
	}

	if(status == HAL_OK) {
		ret = 0;
	}

	if(can_info->hcan_mutex) {
		//os_status = osMutexRelease(can_info->hcan_mutex);

		//if(os_status != osOK) {
		//}
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

	status = HAL_CAN_ActivateNotification(can_info->hcan, can_info->receive_fifo);

	if(status != HAL_OK) {
		/* Notification Error */
	}

	if(can_info->hcan_mutex) {
		//os_status = osMutexRelease(can_info->hcan_mutex);

		//if(os_status != osOK) {
		//}
	}

	if(can_info->rx_msg_q != NULL) {
		osEvent event = osMessageGet(can_info->rx_msg_q, timeout);

		if(event.status == osEventMessage) {
			CAN_RxHeaderTypeDef rx_header;
			can_rx_msg_t *rx_msg = &can_info->rx_msg;

			status = HAL_CAN_GetRxMessage(can_info->hcan, can_info->filter_fifo, &rx_header, rx_msg->Data);

			if(status != HAL_OK) {
			} else {
				rx_msg->StdId = rx_header.StdId;
				rx_msg->ExtId = rx_header.ExtId;
				rx_msg->IDE = rx_header.IDE;
				rx_msg->RTR = rx_header.RTR;
				rx_msg->DLC = rx_header.DLC;
				ret = 0;
			}
		} else {
			//can_transmit_dummy(can_info->hcan);

			//重新初始化
			//HAL_CAN_DeInit(can_info->hcan);

			//if(can_info->can_hal_init) {
			//	can_info->can_hal_init();
			//}

			//if(can_info->receive_init) {
			//	can_info->receive_init(can_info->hcan);
			//}
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
	can_rx_msg_t *rx_msg = &can_info->rx_msg;

	return rx_msg;
}
