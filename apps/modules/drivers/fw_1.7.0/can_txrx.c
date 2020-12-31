

/*================================================================
 *
 *
 *   文件名称：can_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时07分55秒
 *   修改日期：2020年12月31日 星期四 11时11分48秒
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

	__disable_irq();
	can_info = (can_info_t *)map_utils_get_value(can_map, hcan);
	__enable_irq();

	return can_info;
}

static void receive_init(CAN_HandleTypeDef *hcan)
{
	CAN_FilterConfTypeDef filter;
	can_info_t *can_info = get_can_info(hcan);
	u_can_filter_id_t id;
	u_can_filter_id_t id_mask;

	if(can_info == NULL) {
		return;
	}

	if(can_info->can_config->filter_fifo == CAN_FILTER_FIFO0) {
		can_info->receive_fifo = CAN_FIFO0;
	} else if(can_info->can_config->filter_fifo == CAN_FILTER_FIFO1) {
		can_info->receive_fifo = CAN_FIFO1;
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

	can_info->hcan->pRxMsg = &can_info->rx_msg;
	can_info->hcan->pRx1Msg = &can_info->rx_msg1;

	filter.FilterNumber = can_info->can_config->filter_number;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterIdHigh = id.s_lh.h;
	filter.FilterIdLow = id.s_lh.l;
	filter.FilterMaskIdHigh = id_mask.s_lh.h;
	filter.FilterMaskIdLow = id_mask.s_lh.l;
	filter.FilterFIFOAssignment = can_info->can_config->filter_fifo;
	filter.FilterActivation = ENABLE;
	filter.BankNumber = 14;

	HAL_CAN_ConfigFilter(can_info->can_config->config_can, &filter);
}

void free_can_info(can_info_t *can_info)
{
	osStatus os_status;
	int ret;

	if(can_info == NULL) {
		return;
	}

	__disable_irq();
	ret = map_utils_remove_value(can_map, can_info->hcan);
	__enable_irq();

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

	if(can_map == NULL) {
		can_map = map_utils_alloc(NULL);
	}

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

	can_info->hcan = hcan;
	can_info->can_config = can_config;
	can_info->hcan_mutex = osMutexCreate(osMutex(hcan_mutex));
	can_info->tx_msg_q = osMessageCreate(osMessageQ(tx_msg_q), NULL);
	can_info->rx_msg_q = osMessageCreate(osMessageQ(rx_msg_q), NULL);

	can_info->receive_init = receive_init;

	can_info->receive_init(hcan);

	__disable_irq();
	ret = map_utils_add_key_value(can_map, hcan, can_info);
	__enable_irq();

	if(ret != 0) {
		free_can_info(can_info);
		can_info = NULL;
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
	osStatus os_status;

	can_info->hcan->pTxMsg = msg;

	if(can_info->hcan_mutex != NULL) {
		os_status = osMutexWait(can_info->hcan_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	status = HAL_CAN_Transmit_IT(can_info->hcan);

	if(status != HAL_OK) {
	}

	if(can_info->hcan_mutex != NULL) {
		os_status = osMutexRelease(can_info->hcan_mutex);

		if(os_status != osOK) {
		}
	}

	if(can_info->tx_msg_q != NULL) {
		osEvent event = osMessageGet(can_info->tx_msg_q, timeout);

		if(event.status != osEventTimeout) {
			ret = 0;
		}
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

	status = HAL_CAN_Receive_IT(can_info->hcan, can_info->receive_fifo);

	if(status != HAL_OK) {//重新初始化
		//HAL_CAN_DeInit(can_info->hcan);

		//if(can_info->can_hal_init) {
		//	can_info->can_hal_init();
		//}

		//if(can_info->receive_init) {
		//	can_info->receive_init(can_info->hcan);
		//}

		//status = HAL_CAN_Receive_IT(can_info->hcan, can_info->receive_fifo);

		//if(status != HAL_OK) {
		//}
	}

	if(can_info->hcan_mutex != NULL) {
		os_status = osMutexRelease(can_info->hcan_mutex);

		if(os_status != osOK) {
		}
	}

	if(can_info->rx_msg_q != NULL) {
		osEvent event = osMessageGet(can_info->rx_msg_q, timeout);

		if(event.status != osEventTimeout) {
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
