

/*================================================================
 *
 *
 *   文件名称：can_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时07分55秒
 *   修改日期：2021年01月18日 星期一 09时49分25秒
 *   描    述：
 *
 *================================================================*/
#include "can_txrx.h"

#include "os_utils.h"
#include "map_utils.h"

static map_utils_t *can_map = NULL;

static can_info_t *get_can_info(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = NULL;

	can_info = (can_info_t *)map_utils_get_value(can_map, hcan);

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
		app_panic();
		return;
	}

	if(can_info->can_config->filter_fifo == CAN_FILTER_FIFO0) {
		can_info->receive_fifo = CAN_IT_RX_FIFO0_MSG_PENDING;
	} else if(can_info->can_config->filter_fifo == CAN_FILTER_FIFO1) {
		can_info->receive_fifo = CAN_IT_RX_FIFO1_MSG_PENDING;
	} else {
		app_panic();
	}

	id.v = 0;
	id_mask.v = 0;

	id.s.id = can_info->can_config->filter_id;
	id_mask.s.id = can_info->can_config->filter_mask_id;

	id.s.rtr = can_info->can_config->filter_rtr;
	id_mask.s.rtr = can_info->can_config->filter_mask_rtr;

	id.s.ext = can_info->can_config->filter_ext;
	id_mask.s.ext = can_info->can_config->filter_mask_ext;

	filter.FilterBank = can_info->can_config->filter_number;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterIdHigh = id.s_lh.h;
	filter.FilterIdLow = id.s_lh.l;
	filter.FilterMaskIdHigh = id_mask.s_lh.h;
	filter.FilterMaskIdLow = id_mask.s_lh.l;
	filter.FilterFIFOAssignment = can_info->can_config->filter_fifo;
	filter.FilterActivation = ENABLE;
	filter.SlaveStartFilterBank = 14;

	HAL_CAN_ConfigFilter(can_info->can_config->config_can, &filter);

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
	int ret;

	if(can_info == NULL) {
		return;
	}

	ret = map_utils_remove_value(can_map, can_info->hcan);

	if(ret != 0) {
	}

	if(can_info->hcan_mutex != NULL) {
		os_status = osMutexWait(can_info->hcan_mutex, osWaitForever);

		if(osOK != os_status) {
		}
	}

	if(can_info->rx_msg_q != NULL) {
		os_status = osMessageDelete(can_info->rx_msg_q);

		if(osOK != os_status) {
		}
	}

	if(can_info->tx_msg_q != NULL) {
		os_status = osMessageDelete(can_info->tx_msg_q);

		if(osOK != os_status) {
		}
	}

	if(can_info->hcan_mutex != NULL) {
		os_status = osMutexRelease(can_info->hcan_mutex);

		if(osOK != os_status) {
		}
	}


	if(can_info->hcan_mutex != NULL) {
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
	int ret;

	osMutexDef(hcan_mutex);
	osMessageQDef(tx_msg_q, 1, uint16_t);
	osMessageQDef(rx_msg_q, 1, uint16_t);

	__disable_irq();

	if(can_map == NULL) {
		can_map = map_utils_alloc(NULL);
	}

	__enable_irq();

	can_info = get_can_info(hcan);

	if(can_info != NULL) {
		return can_info;
	}

	if(hcan == NULL) {
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

	can_info->hcan = hcan;
	can_info->can_config = can_config;
	can_info->hcan_mutex = osMutexCreate(osMutex(hcan_mutex));
	can_info->tx_msg_q = osMessageCreate(osMessageQ(tx_msg_q), NULL);
	can_info->rx_msg_q = osMessageCreate(osMessageQ(rx_msg_q), NULL);

	can_info->receive_init = receive_init;

	ret = map_utils_add_key_value(can_map, hcan, can_info);

	if(ret != 0) {
		free_can_info(can_info);
		can_info = NULL;
	}

	can_info->receive_init(hcan);

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
	osStatus os_status;
	CAN_TxHeaderTypeDef tx_header;

	tx_header.StdId = msg->StdId;
	tx_header.ExtId = msg->ExtId;
	tx_header.RTR = msg->RTR;
	tx_header.IDE = msg->IDE;
	tx_header.DLC = msg->DLC;
	tx_header.TransmitGlobalTime = DISABLE;

	status = HAL_BUSY;

	while(status != HAL_OK) {
		if(can_info->hcan_mutex != NULL) {
			os_status = osMutexWait(can_info->hcan_mutex, osWaitForever);

			if(os_status != osOK) {
			}
		}

		status = HAL_CAN_AddTxMessage(can_info->hcan, &tx_header, msg->Data, &msg->tx_mailbox);

		if(can_info->hcan_mutex != NULL) {
			os_status = osMutexRelease(can_info->hcan_mutex);

			if(os_status != osOK) {
			}
		}

		if(osKernelSysTick() - stamp >= timeout) {
			break;
		}

		if(status != HAL_OK) {
			osDelay(5);
		}
	}

	if(status == HAL_OK) {
		ret = 0;
	}


	return ret;
}

int can_rx_data(can_info_t *can_info, uint32_t timeout)
{
	int ret = -1;
	osStatus os_status;
	HAL_StatusTypeDef status;

	if(can_info == NULL) {
		return ret;
	}

	if(can_info->hcan_mutex != NULL) {
		os_status = osMutexWait(can_info->hcan_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	status = HAL_CAN_ActivateNotification(can_info->hcan, can_info->receive_fifo);

	if(status != HAL_OK) {
		/* Notification Error */
	}

	if(can_info->hcan_mutex != NULL) {
		os_status = osMutexRelease(can_info->hcan_mutex);

		if(os_status != osOK) {
		}
	}

	if(can_info->rx_msg_q != NULL) {
		osEvent event = osMessageGet(can_info->rx_msg_q, timeout);

		if(event.status == osEventMessage) {
			CAN_RxHeaderTypeDef rx_header;
			can_rx_msg_t *rx_msg = &can_info->rx_msg;

			status = HAL_CAN_GetRxMessage(can_info->hcan, can_info->can_config->filter_fifo, &rx_header, rx_msg->Data);

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
