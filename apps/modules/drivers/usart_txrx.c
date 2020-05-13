

/*================================================================
 *
 *
 *   文件名称：usart_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月25日 星期五 22时38分35秒
 *   修改日期：2020年05月13日 星期三 15时11分32秒
 *   描    述：
 *
 *================================================================*/
#include "usart_txrx.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "os_utils.h"
//#define UDP_LOG
#include "task_probe_tool.h"

static LIST_HEAD(uart_info_list);
static osMutexId uart_info_list_mutex = NULL;

static uart_info_t *get_uart_info(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = NULL;
	uart_info_t *uart_info_item = NULL;
	osStatus os_status;

	if(uart_info_list_mutex == NULL) {
		return uart_info;
	}

	os_status = osMutexWait(uart_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(uart_info_item, &uart_info_list, uart_info_t, list) {
		if(uart_info_item->huart == huart) {
			uart_info = uart_info_item;
			break;
		}
	}

	os_status = osMutexRelease(uart_info_list_mutex);

	if(os_status != osOK) {
	}

	return uart_info;
}

void free_uart_info(uart_info_t *uart_info)
{
	osStatus os_status;

	if(uart_info == NULL) {
		return;
	}

	if(uart_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(uart_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&uart_info->list);

	os_status = osMutexRelease(uart_info_list_mutex);

	if(os_status != osOK) {
	}

	if(uart_info->tx_msg_q) {
		os_status = osMessageDelete(uart_info->tx_msg_q);

		if(osOK != os_status) {
		}
	}

	if(uart_info->rx_msg_q) {
		os_status = osMessageDelete(uart_info->rx_msg_q);

		if(osOK != os_status) {
		}
	}

	if(uart_info->huart_mutex) {
		os_status = osMutexDelete(uart_info->huart_mutex);

		if(osOK != os_status) {
		}
	}

	if(uart_info->log_mutex) {
		os_status = osMutexDelete(uart_info->log_mutex);

		if(osOK != os_status) {
		}
	}

	os_free(uart_info);
}

uart_info_t *get_or_alloc_uart_info(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = NULL;
	osMessageQDef(tx_msg_q, 1, uint16_t);
	osMessageQDef(rx_msg_q, 1, uint16_t);
	osMutexDef(huart_mutex);
	osMutexDef(log_mutex);
	osStatus os_status;

	uart_info = get_uart_info(huart);

	if(uart_info != NULL) {
		return uart_info;
	}

	if(uart_info_list_mutex == NULL) {
		osMutexDef(uart_info_list_mutex);
		uart_info_list_mutex = osMutexCreate(osMutex(uart_info_list_mutex));

		if(uart_info_list_mutex == NULL) {
			return uart_info;
		}
	}

	uart_info = (uart_info_t *)os_alloc(sizeof(uart_info_t));

	if(uart_info == NULL) {
		return uart_info;
	}

	uart_info->huart = huart;
	uart_info->tx_msg_q = osMessageCreate(osMessageQ(tx_msg_q), NULL);
	uart_info->rx_msg_q = osMessageCreate(osMessageQ(rx_msg_q), NULL);
	uart_info->huart_mutex = osMutexCreate(osMutex(huart_mutex));
	uart_info->log_mutex = osMutexCreate(osMutex(log_mutex));
	uart_info->rx_poll_interval = 5;
	uart_info->max_pending_duration = 50;

	os_status = osMutexWait(uart_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&uart_info->list, &uart_info_list);

	os_status = osMutexRelease(uart_info_list_mutex);

	if(os_status != osOK) {
	}

	return uart_info;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = get_uart_info(huart);

	if(uart_info == NULL) {
		return;
	}

	if(uart_info->tx_msg_q != NULL) {
		osStatus os_status = osMessagePut(uart_info->tx_msg_q, 0, 0);

		if(os_status != osOK) {
		}
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = get_uart_info(huart);

	if(uart_info == NULL) {
		return;
	}

	if(uart_info->rx_msg_q != NULL) {
		osStatus os_status = osMessagePut(uart_info->rx_msg_q, 0, 0);

		if(os_status != osOK) {
		}
	}
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
	osStatus os_status;

	if(uart_info->huart_mutex) {
		os_status = osMutexWait(uart_info->huart_mutex, osWaitForever);

		if(os_status != osOK) {
			udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		}
	}

	status = HAL_UART_Transmit_DMA(uart_info->huart, data, size);

	if(status != HAL_OK) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
	}

	if(uart_info->huart_mutex) {
		os_status = osMutexRelease(uart_info->huart_mutex);

		if(os_status != osOK) {
			udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		}
	}

	if(uart_info->tx_msg_q != NULL) {
		osEvent event = osMessageGet(uart_info->tx_msg_q, timeout);

		ret = get_uart_sent(uart_info);

		if(event.status == osEventTimeout) {
			HAL_UART_AbortTransmit(uart_info->huart);
		}
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

		if(uart_info->rx_msg_q != NULL) {
			osEvent event = osMessageGet(uart_info->rx_msg_q, wait_ticks);

			if(event.status == osEventTimeout) {
			}
		}

		cur_ticks = osKernelSysTick();

		received = get_uart_received(uart_info);

		if(received > 0) {
			if(received == size) {//complete
				HAL_UART_AbortReceive(uart_info->huart);
				break;
			}

			if(pre_received == received) {
				//pending for a long time(poll interval)
				if(cur_ticks - pre_received_ticks >= uart_info->max_pending_duration) {
					udp_log_printf("%s:%s:%d pending duration:%d\n", __FILE__, __func__, __LINE__, cur_ticks - pre_received_ticks);
					HAL_UART_AbortReceive(uart_info->huart);
					break;
				}
			} else {
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
	osStatus os_status;

	if(uart_info->huart_mutex) {
		os_status = osMutexWait(uart_info->huart_mutex, osWaitForever);

		if(os_status != osOK) {
			udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		}
	}

	status = HAL_UART_Receive_DMA(uart_info->huart, data, size);

	if(status != HAL_OK) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
	}

	if(uart_info->huart_mutex) {
		os_status = osMutexRelease(uart_info->huart_mutex);

		if(os_status != osOK) {
			udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		}
	}

	ret = wait_for_uart_receive(uart_info, size, timeout);

	return ret;
}

int uart_tx_rx_data(uart_info_t *uart_info, uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size, uint32_t timeout)
{
	int ret = 0;
	HAL_StatusTypeDef status;
	osStatus os_status;

	if(uart_info->huart_mutex) {
		os_status = osMutexWait(uart_info->huart_mutex, osWaitForever);

		if(os_status != osOK) {
			udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		}
	}

	status = HAL_UART_Receive_DMA(uart_info->huart, rx_data, rx_size);

	if(status != HAL_OK) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
	}

	status = HAL_UART_Transmit_DMA(uart_info->huart, tx_data, tx_size);

	if(status != HAL_OK) {
	}

	if(uart_info->huart_mutex) {
		os_status = osMutexRelease(uart_info->huart_mutex);

		if(os_status != osOK) {
			udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		}
	}

	ret = wait_for_uart_receive(uart_info, rx_size, timeout);

	if(uart_info->tx_msg_q != NULL) {
		osEvent event = osMessageGet(uart_info->tx_msg_q, 0);

		if(event.status == osEventTimeout) {
			HAL_UART_AbortTransmit(uart_info->huart);
		}
	}

	return ret;
}

static uart_info_t *log_uart_info = NULL;

void set_log_uart_info(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = get_uart_info(huart);

	if(uart_info == NULL) {
		log_uart_info = NULL;
	} else {
		log_uart_info = uart_info;
	}
}

static uart_info_t *get_log_uart_info(void)
{
	return log_uart_info;
}

int log_uart_data(void *data, size_t size)
{
	int ret = 0;
	uart_info_t *uart_info = get_log_uart_info();
	osStatus os_status;

	if(uart_info != NULL) {
		if(uart_info->log_mutex) {
			os_status = osMutexWait(uart_info->log_mutex, osWaitForever);

			if(os_status != osOK) {
			}
		}

		ret = uart_tx_data(uart_info, (uint8_t *)data, size, 1000);

		if(uart_info->log_mutex) {
			os_status = osMutexRelease(uart_info->log_mutex);

			if(os_status != osOK) {
			}
		}
	}

	return ret;
}
