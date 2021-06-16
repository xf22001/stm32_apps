

/*================================================================
 *
 *
 *   文件名称：can_ops_hal.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月15日 星期二 19时47分17秒
 *   修改日期：2021年06月16日 星期三 12时31分31秒
 *   描    述：
 *
 *================================================================*/
#include "can_txrx.h"

static void _can_init(void *_can_info)
{
	can_info_t *can_info = (can_info_t *)_can_info;
	CAN_FilterTypeDef filter;
	HAL_StatusTypeDef status;
	u_can_filter_id_t id;
	u_can_filter_id_t id_mask;

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

static void can_rxfifo_pending_callback(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = get_or_alloc_can_info(hcan);
	HAL_StatusTypeDef status;
	CAN_RxHeaderTypeDef rx_header;
	can_rx_msg_t *rx_msg;

	if(can_info == NULL) {
		return;
	}

	rx_msg = &can_info->rx_msg[can_info->rx_msg_w];

	if(can_info->rx_msg_w + 1 >= CAN_RX_MSG_BUFFER_SIZE) {
		can_info->rx_msg_w = 0;
	} else {
		can_info->rx_msg_w++;
	}

	status = HAL_CAN_GetRxMessage(can_info->hcan, can_info->can_config->filter_fifo, &rx_header, rx_msg->Data);

	if(status == HAL_OK) {
		rx_msg->StdId = rx_header.StdId;
		rx_msg->ExtId = rx_header.ExtId;
		rx_msg->IDE = rx_header.IDE;
		rx_msg->RTR = rx_header.RTR;
		rx_msg->DLC = rx_header.DLC;
	}

	signal_send(can_info->rx_msg_q, 0, 0);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	can_rxfifo_pending_callback(hcan);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	can_rxfifo_pending_callback(hcan);
}

static void can_tx_abort(can_info_t *can_info)
{
	uint32_t mailbox_mask = CAN_TX_MAILBOX0 | CAN_TX_MAILBOX1 | CAN_TX_MAILBOX2;
	HAL_StatusTypeDef status;

	status = HAL_CAN_AbortTxRequest(can_info->hcan, mailbox_mask);

	if(status != HAL_OK) {
	}
}

#define CAN_TX_ERROR_LIMIT 10

static int _can_tx_data(void *_can_info, can_tx_msg_t *msg, uint32_t timeout)
{
	int ret = -1;
	can_info_t *can_info = (can_info_t *)_can_info;
	uint32_t stamp = osKernelSysTick();
	HAL_StatusTypeDef status;
	CAN_TxHeaderTypeDef tx_header;

	msg->tx_mailbox = 0;
	tx_header.StdId = msg->StdId;
	tx_header.ExtId = msg->ExtId;
	tx_header.RTR = msg->RTR;
	tx_header.IDE = msg->IDE;
	tx_header.DLC = msg->DLC;
	tx_header.TransmitGlobalTime = DISABLE;

	status = HAL_BUSY;

	while(status != HAL_OK) {
		mutex_lock(can_info->hcan_mutex);

		status = HAL_CAN_AddTxMessage(can_info->hcan, &tx_header, msg->Data, &msg->tx_mailbox);

		mutex_unlock(can_info->hcan_mutex);

		if(ticks_duration(osKernelSysTick(), - stamp) >= timeout) {
			break;
		}

		if(status != HAL_OK) {
			osDelay(5);
		}
	}

	if(status == HAL_OK) {
		can_info->tx_error = 0;
		ret = 0;
	} else {
		can_info->tx_error++;

		if(can_info->tx_error >= CAN_TX_ERROR_LIMIT) {
			can_info->tx_error = 0;
			can_tx_abort(can_info);
		}
	}

	return ret;
}

static int _can_rx_data(void *_can_info, uint32_t timeout)
{
	int ret = -1;
	can_info_t *can_info = (can_info_t *)_can_info;

	if(can_info->rx_msg_r == can_info->rx_msg_w) {//没有数据
		ret = signal_wait(can_info->rx_msg_q, NULL, timeout);
	} else {//有数据
		ret = 0;
	}

	if(ret == 0) {
		can_info->rx_msg_pos = can_info->rx_msg_r;

		if((can_info->rx_msg_r + 1) >= CAN_RX_MSG_BUFFER_SIZE) {
			can_info->rx_msg_r = 0;
		} else {
			can_info->rx_msg_r++;
		}
	}

	return ret;
}

can_ops_t can_ops_hal = {
	.type = CAN_TYPE_HAL,
	.can_init = _can_init,
	.can_tx_data = _can_tx_data,
	.can_rx_data = _can_rx_data,
};
