

/*================================================================
 *
 *
 *   文件名称：usart_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月25日 星期五 22时38分35秒
 *   修改日期：2020年03月29日 星期日 14时28分31秒
 *   描    述：
 *
 *================================================================*/
#include "usart_txrx.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "os_utils.h"
#include "task_probe_tool.h"

#define UART_RX_DMA_DATA_SIZE 128
#define LOG_BUFFER_SIZE 128

static LIST_HEAD(uart_info_list);

uart_info_t *get_uart_info(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = NULL;
	uart_info_t *uart_info_item = NULL;

	list_for_each_entry(uart_info_item, &uart_info_list, uart_info_t, list) {
		if(uart_info_item->huart == huart) {
			uart_info = uart_info_item;
			break;
		}
	}

	return uart_info;
}

void free_uart_info(uart_info_t *uart_info)
{
	osStatus os_status;

	if(uart_info == NULL) {
		return;
	}

	list_del(&uart_info->list);

	if(uart_info->tx_msg_q) {
		os_status = osMessageDelete(uart_info->tx_msg_q);

		if(osOK != os_status) {
		}
	}

	if(uart_info->huart_mutex) {
		os_status = osMutexDelete(uart_info->huart_mutex);

		if(osOK != os_status) {
		}
	}

	os_free(uart_info);
}

uart_info_t *alloc_uart_info(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = NULL;
	osMessageQDef(tx_msg_q, 1, uint16_t);
	osMutexDef(huart_mutex);

	uart_info = get_uart_info(huart);

	if(uart_info != NULL) {
		return uart_info;
	}

	uart_info = (uart_info_t *)os_alloc(sizeof(uart_info_t));

	if(uart_info == NULL) {
		return uart_info;
	}

	uart_info->huart = huart;
	uart_info->tx_msg_q = osMessageCreate(osMessageQ(tx_msg_q), NULL);
	uart_info->huart_mutex = osMutexCreate(osMutex(huart_mutex));
	uart_info->rx_poll_interval = 5;

	list_add_tail(&uart_info->list, &uart_info_list);

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

uint16_t crc_check_for_dcph(uint8_t *data, uint16_t size)
{
	uint16_t crc = 0xFFFF;
	uint16_t i;

	//udp_log_printf("crc_check_for_dcph size:%d\n", size);

	for(i = 0; i < size; i++) {
		uint16_t loop;

		crc = crc ^ data[i];

		for(loop = 0; loop < 8; loop++) {
			if(crc & 1) {
				crc >>= 1;
				crc ^= 0xa001;
			} else {
				crc >>= 1;
			}
		}
	}

	return (crc);
}

static uint16_t get_uart_sent(uart_info_t *uart_info)
{
	return (uart_info->huart->TxXferSize - __HAL_DMA_GET_COUNTER(uart_info->huart->hdmatx));
}

static uint16_t get_uart_received(uart_info_t *uart_info)
{
	return (uart_info->huart->RxXferSize - __HAL_DMA_GET_COUNTER(uart_info->huart->hdmarx));
}

static void set_rx_poll_duration(uart_info_t *uart_info, uint32_t rx_poll_interval)
{
	uart_info->rx_poll_interval = rx_poll_interval;
}

int uart_tx_data(uart_info_t *uart_info, uint8_t *data, uint16_t size, uint32_t timeout)
{
	int ret = 0;
	HAL_StatusTypeDef status;
	//osStatus os_status;

	if(uart_info->huart_mutex) {
		//os_status = osMutexWait(uart_info->huart_mutex, osWaitForever);

		//if(os_status != osOK) {
		//}
	}

	HAL_UART_Transmit_DMA(uart_info->huart, data, size);

	if(status != HAL_OK) {
	}

	if(uart_info->huart_mutex) {
		//os_status = osMutexRelease(uart_info->huart_mutex);

		//if(os_status != osOK) {
		//}
	}

	if(uart_info->tx_msg_q != NULL) {
		osEvent event = osMessageGet(uart_info->tx_msg_q, timeout);

		ret = get_uart_sent(uart_info);

		if(event.status != osEventMessage) {
			HAL_UART_AbortTransmit(uart_info->huart);
		}
	}

	return ret;
}

static uint16_t wait_for_uart_receive(uart_info_t *uart_info, uint16_t size, uint32_t timeout)
{
	uint16_t pre_received = 0;
	uint16_t received = get_uart_received(uart_info);
	uint32_t enter_ticks = osKernelSysTick();
	uint32_t duration = 0;
	uint32_t wait_ticks;

	while(duration < timeout) {
		if(received > 0) {
			if(received == size) {//complete
				break;
			}

			if(pre_received == received) {//pending for a long time(poll interval)
				//udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
				break;
			} else {
				pre_received = received;
			}
		}

		wait_ticks = timeout - duration;

		if(wait_ticks > uart_info->rx_poll_interval) {
			wait_ticks = uart_info->rx_poll_interval;
		}

		osDelay(wait_ticks);

		duration = osKernelSysTick() - enter_ticks;

		received = get_uart_received(uart_info);
	}

	if(received != size) {
		HAL_UART_AbortReceive(uart_info->huart);
	}

	return received;
}

int uart_rx_data(uart_info_t *uart_info, uint8_t *data, uint16_t size, uint32_t timeout)
{
	int ret = 0;
	HAL_StatusTypeDef status;
	//osStatus os_status;

	if(uart_info->huart_mutex) {
		//os_status = osMutexWait(uart_info->huart_mutex, osWaitForever);

		//if(os_status != osOK) {
		//}
	}

	status = HAL_UART_Receive_DMA(uart_info->huart, data, size);

	if(status != HAL_OK) {
	}

	if(uart_info->huart_mutex) {
		//os_status = osMutexRelease(uart_info->huart_mutex);

		//if(os_status != osOK) {
		//}
	}

	ret = wait_for_uart_receive(uart_info, size, timeout);

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

	if(uart_info != NULL) {
		ret = uart_tx_data(uart_info, (uint8_t *)data, size, 1000);
	}

	return ret;
}
