

/*================================================================
 *   
 *   
 *   文件名称：uart_debug.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月13日 星期三 10时45分08秒
 *   修改日期：2020年05月13日 星期三 14时04分49秒
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
#define UART_LOG
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

#endif //_UART_DEBUG_H
