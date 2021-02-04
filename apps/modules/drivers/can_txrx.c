

/*================================================================
 *
 *
 *   文件名称：can_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时07分55秒
 *   修改日期：2021年02月04日 星期四 10时39分46秒
 *   描    述：
 *
 *================================================================*/
#include "can_txrx.h"

#include <string.h>
#include <stdlib.h>

#include "os_utils.h"
#include "object_class.h"

static object_class_t *can_class = NULL;

static void free_can_info(can_info_t *can_info)
{
	if(can_info == NULL) {
		return;
	}

	mutex_lock(can_info->hcan_mutex);

	signal_delete(can_info->rx_msg_q);

	signal_delete(can_info->tx_msg_q);

	mutex_unlock(can_info->hcan_mutex);

	mutex_delete(can_info->hcan_mutex);

	os_free(can_info);
}

static void receive_init(void *ctx)
{
	can_info_t *can_info = (can_info_t *)ctx;
	CAN_FilterTypeDef filter;
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

static can_info_t *alloc_can_info(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = NULL;
	can_config_t *can_config = NULL;


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

	memset(can_info, 0, sizeof(can_info_t));

	can_info->hcan = hcan;
	can_info->can_config = can_config;
	can_info->hcan_mutex = mutex_create();
	can_info->tx_msg_q = signal_create();
	can_info->rx_msg_q = signal_create();

	can_info->receive_init = receive_init;
	can_info->rx_msg_r = 0;
	can_info->rx_msg_w = 0;
	can_info->rx_msg_pos = 0;
	can_info->tx_error = 0;

	can_info->receive_init(can_info);

	return can_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	can_info_t *can_info = (can_info_t *)o;
	CAN_HandleTypeDef *hcan = (CAN_HandleTypeDef *)ctx;

	if(can_info->hcan == hcan) {
		ret = 0;
	}

	return ret;
}

can_info_t *get_or_alloc_can_info(CAN_HandleTypeDef *hcan)
{
	can_info_t *can_info = NULL;

	__disable_irq();

	if(can_class == NULL) {
		can_class = object_class_alloc();
	}

	__enable_irq();

	can_info = (can_info_t *)object_class_get_or_alloc_object(can_class, object_filter, hcan, (object_alloc_t)alloc_can_info, (object_free_t)free_can_info);

	return can_info;
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
	can_info->rx_msg_w++;

	if(can_info->rx_msg_w >= CAN_RX_MSG_BUFFER_SIZE) {
		can_info->rx_msg_w = 0;
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

int can_tx_data(can_info_t *can_info, can_tx_msg_t *msg, uint32_t timeout)
{
	int ret = -1;
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

		if(abs(osKernelSysTick() - stamp) >= timeout) {
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

int can_rx_data(can_info_t *can_info, uint32_t timeout)
{
	int ret = -1;
	uint8_t rx_msg_w;

	if(can_info == NULL) {
		return ret;
	}

	rx_msg_w = can_info->rx_msg_w;

	if(can_info->rx_msg_r == rx_msg_w) {//没有数据
		ret = signal_wait(can_info->rx_msg_q, NULL, timeout);
	} else {//有数据
		ret = 0;
	}

	if(ret == 0) {
		can_info->rx_msg_pos = can_info->rx_msg_r;
		can_info->rx_msg_r++;

		if(can_info->rx_msg_r >= CAN_RX_MSG_BUFFER_SIZE) {
			can_info->rx_msg_r = 0;
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
	can_rx_msg_t *rx_msg = &can_info->rx_msg[can_info->rx_msg_pos];

	return rx_msg;
}
