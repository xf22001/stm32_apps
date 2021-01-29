

/*================================================================
 *   
 *   
 *   文件名称：usart_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月25日 星期五 22时38分40秒
 *   修改日期：2021年01月29日 星期五 15时18分23秒
 *   描    述：
 *
 *================================================================*/
#ifndef _USART_TXRX_H
#define _USART_TXRX_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "list_utils.h"

typedef struct {
	UART_HandleTypeDef *huart;
	osMessageQId tx_msg_q;
	osMessageQId rx_msg_q;
	osMutexId huart_mutex;
	osMutexId log_mutex;
	uint32_t rx_poll_interval; //ticks
	uint32_t max_pending_duration; //ticks
} uart_info_t;

uart_info_t *get_or_alloc_uart_info(UART_HandleTypeDef *huart);
uint16_t crc_check_for_dcph(uint8_t *data, uint16_t size);
void set_rx_poll_duration(uart_info_t *uart_info, uint32_t rx_poll_interval);
void set_max_pending_duration(uart_info_t *uart_info, uint32_t max_pending_duration);
int uart_tx_data(uart_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
int uart_rx_data(uart_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
int uart_tx_rx_data(uart_info_t *uart_info, uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size, uint32_t timeout);
void set_log_uart_info(uart_info_t *uart_info);
int log_uart_data(uint32_t log_mask, void *data, size_t size);
#endif //_USART_TXRX_H
