

/*================================================================
 *   
 *   
 *   文件名称：uart_debug.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月13日 星期三 10时45分08秒
 *   修改日期：2020年05月14日 星期四 12时00分52秒
 *   描    述：
 *
 *================================================================*/
#ifndef _UART_DEBUG_H
#define _UART_DEBUG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "os_utils.h"
#include "usart_txrx.h"

#ifdef __cplusplus
}
#endif

#define DEBUG_BUFFER_SIZE 128
typedef void (*uart_fn_response_t)(char *arguments);

typedef struct {
	uint32_t fn;
	uart_fn_response_t uart_fn_response;
} uart_fn_item_t;

typedef struct {
	uart_fn_item_t *uart_fn_map;
	size_t uart_fn_map_size;
} uart_fn_map_info_t;

void task_uart_debug(void const *argument);

#if defined(UART_LOG)
#define uart_log_printf(fmt, ...) log_printf((log_fn_t)log_uart_data, fmt, ## __VA_ARGS__)
#define uart_log_hexdump(label, data, len) log_hexdump((log_fn_t)log_uart_data, label, data, len)
#define uart_log_puts(s) log_puts((log_fn_t)log_uart_data, s)
#else//#if defined(UART_LOG)
#define uart_log_printf(fmt, ...)
#define uart_log_hexdump(label, data, len)
#define uart_log_puts(s)
#endif//#if defined(UART_LOG)

#endif //_UART_DEBUG_H
