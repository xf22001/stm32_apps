

/*================================================================
 *
 *
 *   文件名称：usart_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月25日 星期五 22时38分35秒
 *   修改日期：2021年02月04日 星期四 11时52分00秒
 *   描    述：
 *
 *================================================================*/
#include "usart_txrx.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "os_utils.h"
#include "object_class.h"

#define LOG_NONE
#include "log.h"

static object_class_t *uart_class = NULL;
static uart_info_t *log_uart_info = NULL;

static void free_uart_info(uart_info_t *uart_info)
{
	if(uart_info == NULL) {
		return;
	}

	mutex_lock(uart_info->huart_mutex);

	mutex_lock(uart_info->log_mutex);

	if(log_uart_info == uart_info) {
		log_uart_info = NULL;
	}

	mutex_unlock(uart_info->log_mutex);

	mutex_delete(uart_info->log_mutex);

	signal_delete(uart_info->tx_msg_q);
	signal_delete(uart_info->rx_msg_q);

	mutex_unlock(uart_info->huart_mutex);

	mutex_delete(uart_info->huart_mutex);

	os_free(uart_info);
}

static uart_info_t *alloc_uart_info(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = NULL;

	if(huart == NULL) {
		return uart_info;
	}

	uart_info = (uart_info_t *)os_alloc(sizeof(uart_info_t));

	if(uart_info == NULL) {
		return uart_info;
	}

	memset(uart_info, 0, sizeof(uart_info_t));

	uart_info->huart = huart;
	uart_info->tx_msg_q = signal_create(1);
	uart_info->rx_msg_q = signal_create(1);
	uart_info->huart_mutex = mutex_create();
	uart_info->log_mutex = mutex_create();
	uart_info->rx_poll_interval = 5;
	uart_info->max_pending_duration = 50;

	if(uart_info->tx_msg_q == NULL) {
		goto failed;
	}

	if(uart_info->rx_msg_q == NULL) {
		goto failed;
	}

	if(uart_info->huart_mutex == NULL) {
		goto failed;
	}

	if(uart_info->log_mutex == NULL) {
		goto failed;
	}

	return uart_info;

failed:
	free_uart_info(uart_info);
	uart_info = NULL;
	return uart_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	uart_info_t *uart_info = (uart_info_t *)o;
	UART_HandleTypeDef *huart = (UART_HandleTypeDef *)ctx;

	if(uart_info->huart == huart) {
		ret = 0;
	}

	return ret;
}

uart_info_t *get_or_alloc_uart_info(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = NULL;

	__disable_irq();

	if(uart_class == NULL) {
		uart_class = object_class_alloc();
	}

	__enable_irq();

	uart_info = (uart_info_t *)object_class_get_or_alloc_object(uart_class, object_filter, huart, (object_alloc_t)alloc_uart_info, (object_free_t)free_uart_info);

	return uart_info;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = get_or_alloc_uart_info(huart);

	if(uart_info == NULL) {
		return;
	}

	signal_send(uart_info->tx_msg_q, 0, 0);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = get_or_alloc_uart_info(huart);

	if(uart_info == NULL) {
		return;
	}

	signal_send(uart_info->rx_msg_q, 0, 0);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
}

void HAL_UART_AbortCpltCallback (UART_HandleTypeDef *huart)
{
}

void HAL_UART_AbortTransmitCpltCallback (UART_HandleTypeDef *huart)
{
}

void HAL_UART_AbortReceiveCpltCallback (UART_HandleTypeDef *huart)
{
}

static uint16_t get_uart_sent(uart_info_t *uart_info)
{
	return (uart_info->huart->TxXferSize - __HAL_DMA_GET_COUNTER(uart_info->huart->hdmatx));
}

static uint16_t get_uart_received(uart_info_t *uart_info)
{
	return (uart_info->huart->RxXferSize - __HAL_DMA_GET_COUNTER(uart_info->huart->hdmarx));
}

void set_rx_poll_duration(uart_info_t *uart_info, uint32_t rx_poll_interval)
{
	uart_info->rx_poll_interval = rx_poll_interval;
}

void set_max_pending_duration(uart_info_t *uart_info, uint32_t max_pending_duration)
{
	uart_info->max_pending_duration = max_pending_duration;
}

int uart_tx_data(uart_info_t *uart_info, uint8_t *data, uint16_t size, uint32_t timeout)
{
	int ret = 0;
	HAL_StatusTypeDef status;

	mutex_lock(uart_info->huart_mutex);

	status = HAL_UART_Transmit_DMA(uart_info->huart, data, size);

	if(status != HAL_OK) {
		debug("\n");
	}

	mutex_unlock(uart_info->huart_mutex);

	if(signal_wait(uart_info->tx_msg_q, NULL, timeout) == 0) {
		ret = get_uart_sent(uart_info);
	} else {
		ret = get_uart_sent(uart_info);
		HAL_UART_AbortTransmit(uart_info->huart);
	}

	return ret;
}

static uint16_t wait_for_uart_receive(uart_info_t *uart_info, uint16_t size, uint32_t timeout)
{
	uint16_t pre_received = 0;
	uint16_t received = get_uart_received(uart_info);
	uint32_t cur_ticks = osKernelSysTick();
	uint32_t enter_ticks = cur_ticks;
	uint32_t pre_received_ticks = cur_ticks;
	uint32_t duration = 0;
	uint32_t wait_ticks;

	while(duration < timeout) {
		wait_ticks = timeout - duration;

		if(wait_ticks > uart_info->rx_poll_interval) {
			wait_ticks = uart_info->rx_poll_interval;
		}

		signal_wait(uart_info->rx_msg_q, NULL, wait_ticks);

		cur_ticks = osKernelSysTick();

		received = get_uart_received(uart_info);

		if(received > 0) {
			if(received == size) {//接收完成
				HAL_UART_AbortReceive(uart_info->huart);
				break;
			}

			if(pre_received == received) {//没有新数据进来
				//pending for a long time(poll interval)
				if(abs(cur_ticks - pre_received_ticks) >= uart_info->max_pending_duration) {
					debug("pending duration:%d\n", cur_ticks - pre_received_ticks);
					HAL_UART_AbortReceive(uart_info->huart);
					break;
				}
			} else {//有新数据进来
				if(pre_received == 0) {//如果是刚刚有数据进来，重新设定进入时间，保证在接收完一帧数据前不超时
					enter_ticks = cur_ticks;
				}

				pre_received_ticks = cur_ticks;
				pre_received = received;
			}
		}

		duration = cur_ticks - enter_ticks;
	}

	if(duration >= timeout) {
		HAL_UART_AbortReceive(uart_info->huart);
	}

	return received;
}

int uart_rx_data(uart_info_t *uart_info, uint8_t *data, uint16_t size, uint32_t timeout)
{
	int ret = 0;
	HAL_StatusTypeDef status;

	mutex_lock(uart_info->huart_mutex);

	status = HAL_UART_Receive_DMA(uart_info->huart, data, size);

	if(status != HAL_OK) {
		debug("\n");
	}

	mutex_unlock(uart_info->huart_mutex);

	ret = wait_for_uart_receive(uart_info, size, timeout);

	return ret;
}

int uart_tx_rx_data(uart_info_t *uart_info, uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size, uint32_t timeout)
{
	int ret = 0;
	HAL_StatusTypeDef status;

	mutex_lock(uart_info->huart_mutex);

	status = HAL_UART_Receive_DMA(uart_info->huart, rx_data, rx_size);

	if(status != HAL_OK) {
		debug("\n");
	}

	status = HAL_UART_Transmit_DMA(uart_info->huart, tx_data, tx_size);

	if(status != HAL_OK) {
	}

	mutex_unlock(uart_info->huart_mutex);

	ret = wait_for_uart_receive(uart_info, rx_size, timeout);

	if(signal_wait(uart_info->tx_msg_q, NULL, timeout) != 0) {
		HAL_UART_AbortTransmit(uart_info->huart);
	}

	return ret;
}

void set_log_uart_info(uart_info_t *uart_info)
{
	log_uart_info = uart_info;
}

static uart_info_t *get_log_uart_info(void)
{
	return log_uart_info;
}

int log_uart_data(uint32_t log_mask, void *data, size_t size)
{
	int ret = 0;
	uart_info_t *uart_info = get_log_uart_info();
	u_log_mask_t *u_log_mask = (u_log_mask_t *)&log_mask;

	if(uart_info == NULL) {
		return ret;
	}

	if(u_log_mask->s.enable_log_uart == 0) {
		ret = size;
		return ret;
	}

	mutex_lock(uart_info->log_mutex);

	ret = uart_tx_data(uart_info, (uint8_t *)data, size, 100);

	mutex_unlock(uart_info->log_mutex);

	return ret;
}
