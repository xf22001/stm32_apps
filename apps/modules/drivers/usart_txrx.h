

/*================================================================
 *   
 *   
 *   文件名称：usart_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月25日 星期五 22时38分40秒
 *   修改日期：2021年02月08日 星期一 00时05分28秒
 *   描    述：
 *
 *================================================================*/
#ifndef _USART_TXRX_H
#define _USART_TXRX_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "os_utils.h"
#include "list_utils.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	UART_RX_MODE_NORMAL = 0,
	UART_RX_MODE_MATCH,
} uart_rx_mode_t;

typedef struct {
	uint8_t *data;
	uint16_t size;
	uint16_t pos;

	uint8_t *match;
	uint16_t length;
} uart_rx_line_t;

typedef struct {
	UART_HandleTypeDef *huart;
	os_signal_t tx_msg_q;
	os_signal_t rx_msg_q;
	os_mutex_t huart_mutex;
	os_mutex_t log_mutex;
	uint32_t rx_poll_interval; //ticks
	uint32_t max_pending_duration; //ticks
	uart_rx_mode_t uart_rx_mode;
	uart_rx_line_t uart_rx_line;
} uart_info_t;

uart_info_t *get_or_alloc_uart_info(UART_HandleTypeDef *huart);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);
void HAL_UART_AbortCpltCallback (UART_HandleTypeDef *huart);
void HAL_UART_AbortTransmitCpltCallback (UART_HandleTypeDef *huart);
void HAL_UART_AbortReceiveCpltCallback (UART_HandleTypeDef *huart);
void set_rx_poll_duration(uart_info_t *uart_info, uint32_t rx_poll_interval);
void set_max_pending_duration(uart_info_t *uart_info, uint32_t max_pending_duration);
int uart_tx_data(uart_info_t *uart_info, uint8_t *data, uint16_t size, uint32_t timeout);
int uart_rx_data(uart_info_t *uart_info, uint8_t *data, uint16_t size, uint32_t timeout);
int uart_tx_rx_data(uart_info_t *uart_info, uint8_t *tx_data, uint16_t tx_size, uint8_t *rx_data, uint16_t rx_size, uint32_t timeout);
int uart_rx_line(uart_info_t *uart_info, uint8_t *data, uint16_t size, uint8_t *match, uint16_t length, uint32_t timeout);
void set_log_uart_info(uart_info_t *uart_info);
int log_uart_data(uint32_t log_mask, void *data, size_t size);
#endif //_USART_TXRX_H
