

/*================================================================
 *
 *
 *   文件名称：uart_debug.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月13日 星期三 10时45分00秒
 *   修改日期：2021年02月19日 星期五 14时55分34秒
 *   描    述：
 *
 *================================================================*/
#include "uart_debug_handler.h"
#include <stdio.h>

#define LOG_UART
#include "log.h"

static char buffer[DEBUG_BUFFER_SIZE];
static uint8_t received = 0;

/*
static void debug_get_line(uart_info_t *uart_info, char *buffer, size_t size)
{
	int ret = 0;
	char c;

	while(1) {
		ret = uart_rx_data(uart_info, (uint8_t *)buffer + received, 1, 1000);

		if(ret == 0) {
			continue;
		}

		c = buffer[received];

		if(c == '\r') {
			buffer[received] = 0;
			c = 0;
		}

		received += ret;

		if(received < size - 1) {
			if(c != 0) {
				continue;
			} else {
				break;
			}
		} else {
			debug("\n");

			received = DEBUG_BUFFER_SIZE - 1;
			break;
		}
	}
}
*/

static void debug_get_line(uart_info_t *uart_info, char *buffer, size_t size)
{
	received = uart_rx_line(uart_info, (uint8_t *)buffer, size, (uint8_t *)"\r", 1);
}

void task_uart_debug(void const *argument)
{
	int ret = 0;
	int fn;
	int catched;
	uart_info_t *uart_info = (uart_info_t *)argument;

	if(uart_info == NULL) {
		app_panic();
	}

	while(1) {
		received = 0;

		buffer[received] = 0;

		debug_get_line(uart_info, buffer, DEBUG_BUFFER_SIZE);

		_hexdump("buffer", (const char *)buffer, received);

		if(received >= 1) {
			buffer[received - 1] = 0;
		}

		ret = sscanf(buffer, "%d%n", &fn, &catched);

		if(ret == 1) {
			int i;
			char *arguments = &buffer[catched];
			uint8_t found = 0;

			//debug("fn:%d!\n", fn);
			//debug("catched:%d!\n", catched);
			//debug("arguments:%s!\n", arguments);

			for(i = 0; i < uart_fn_map_info.uart_fn_map_size; i++) {
				uart_fn_item_t *uart_fn_item = uart_fn_map_info.uart_fn_map + i;

				if(uart_fn_item->fn == fn) {
					uart_fn_item->uart_fn_response(arguments);
					found = 1;
					break;
				}
			}

			if(found == 0) {
				debug("invalid function:%d!\n", fn);
			}
		} else {
			debug("invalid command:\'%s\'\n", buffer);
		}
	}
}
