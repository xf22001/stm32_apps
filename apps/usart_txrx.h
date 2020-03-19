

/*================================================================
 *   
 *   
 *   文件名称：usart_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月25日 星期五 22时38分40秒
 *   修改日期：2020年03月19日 星期四 13时43分41秒
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
#include "stm32f2xx_hal.h"
#include "list_utils.h"

typedef struct {
	struct list_head list;
	UART_HandleTypeDef *huart;
	osMessageQId tx_msg_q;
	osMutexId huart_mutex;
} uart_info_t;

uart_info_t *get_uart_info(UART_HandleTypeDef *huart);
uart_info_t *alloc_uart_info(UART_HandleTypeDef *huart);
void free_uart_info(uart_info_t *uart_info);
uint16_t crc_check_for_dcph(uint8_t *data, uint16_t size);
int uart_tx_data(uart_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
int uart_rx_data(uart_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
void set_log_uart_info(UART_HandleTypeDef *huart);
int uart_log_printf(const char *fmt, ...);
void uart_log_hexdump(const char *label, const char *data, int len);
int uart_log_puts(const char *s);
#endif //_USART_TXRX_H
