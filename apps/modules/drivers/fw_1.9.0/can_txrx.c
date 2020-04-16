

/*================================================================
 *
 *
 *   文件名称：can_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时07分55秒
 *   修改日期：2020年04月16日 星期四 17时40分45秒
 *   描    述：
 *
 *================================================================*/
#include "can_txrx.h"

#include "os_utils.h"

static LIST_HEAD(can_info_list);
static osMutexId can_info_list_mutex = NULL;

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

typedef struct {
	CAN_HandleTypeDef *hcan;
	CAN_HandleTypeDef *config_can;
	uint32_t filter_number;
	uint32_t filter_fifo;
	uint32_t receive_fifo;
	uint32_t filter_id;
	uint32_t filter_maskid;
	uint8_t filter_rtr;
	uint8_t filter_ext;
} can_config_t;

typedef struct {
	uint32_t unused : 1;
	uint32_t rtr : 1;
	uint32_t ext : 1;
	uint32_t id : 29;
} can_filter_id_t;

typedef struct {
	uint16_t l;
	uint16_t h;
} can_filter_id_lh_t;

typedef union {
	can_filter_id_t s;
	can_filter_id_lh_t s_lh;
	uint32_t v;
} u_can_filter_id_t;

static can_config_t can_config_sz[] = {
	{
		.hcan = &hcan1,
		.filter_number = 0,
		.filter_fifo = CAN_FILTER_FIFO0,
		.receive_fifo = CAN_IT_RX_FIFO0_MSG_PENDING,
		.config_can = &hcan1,
		.filter_id = 0,
		.filter_maskid = 0,
		.filter_rtr = 0,
		.filter_ext = 0,
	},
	{
		.hcan = &hcan2,
		.filter_number = 14,
		.filter_fifo = CAN_FILTER_FIFO1,
		.receive_fifo = CAN_IT_RX_FIFO1_MSG_PENDING,
		.config_can = &hcan1,
		.filter_id = 0,
		.filter_maskid = 0,
		.filter_rtr = 0,
		.filter_ext = 0,
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
	CAN_FilterTypeDef filter;
	can_info_t *can_info = get_can_info(hcan);
	HAL_StatusTypeDef status;
	u_can_filter_id_t id;
	u_can_filter_id_t id_mask;


	if(can_info == NULL) {
		return;
	}

	id.v = can_info->filter_id;
	id_mask.v = can_info->filter_maskid;

	filter.FilterBank = can_info->filter_number;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterIdHigh = id.s_lh.h;
	filter.FilterIdLow = id.s_lh.l;
	filter.FilterMaskIdHigh = id_mask.s_lh.h;
	filter.FilterMaskIdLow = id_mask.s_lh.l;
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
	osStatus os_status;

	if(can_info == NULL) {
		return;
	}

	if(can_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(can_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&can_info->list);

	os_status = osMutexRelease(can_info_list_mutex);

	if(os_status != osOK) {
	}

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

	u_can_filter_id_t id;
	u_can_filter_id_t id_mask;

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

	id.s.id = can_config->filter_id;
	id_mask.s.id = can_config->filter_maskid;

	if(can_config->filter_rtr != 0) {
		id.s.rtr = 1;
		id_mask.s.rtr = 1;
	}

	if(can_config->filter_ext != 0) {
		id.s.ext = 1;
		id_mask.s.ext = 1;
	}

	can_info->filter_id = id.v;
	can_info->filter_maskid = id_mask.v;

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
