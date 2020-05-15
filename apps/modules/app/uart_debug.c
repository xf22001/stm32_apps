

/*================================================================
 *
 *
 *   文件名称：uart_debug.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月13日 星期三 10时45分00秒
 *   修改日期：2020年05月15日 星期五 08时36分23秒
 *   描    述：
 *
 *================================================================*/
#include "uart_debug_handler.h"
#include <stdio.h>

#define LOG_UART
#include "log.h"

static char buffer[DEBUG_BUFFER_SIZE];
static uint8_t received = 0;

void task_uart_debug(void const *argument)
{
	int ret = 0;
	int fn;
	int catched;
	char c;
	uart_info_t *uart_info = (uart_info_t *)argument;

	if(uart_info == NULL) {
		app_panic();
	}

	received = 0;

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

		if(received < DEBUG_BUFFER_SIZE - 1) {
			if(c != 0) {
				continue;
			}
		} else {
			_printf("\n");

			received = DEBUG_BUFFER_SIZE - 1;
		}

		//_hexdump("buffer", (const char *)buffer, received);

		buffer[received] = 0;

		ret = sscanf(buffer, "%d%n", &fn, &catched);

		if(ret == 1) {
			int i;
			char *arguments = &buffer[catched];
			uint8_t found = 0;

			//_printf("fn:%d!\n", fn);
			//_printf("catched:%d!\n", catched);
			//_printf("arguments:%s!\n", arguments);

			for(i = 0; i < uart_fn_map_info.uart_fn_map_size; i++) {
				uart_fn_item_t *uart_fn_item = uart_fn_map_info.uart_fn_map + i;

				if(uart_fn_item->fn == fn) {
					uart_fn_item->uart_fn_response(arguments);
					found = 1;
					break;
				}
			}

			if(found == 0) {
				_printf("invalid function:%d!\n", fn);
			}
		} else {
			_printf("invalid command:\'%s\'\n", buffer);
		}

		received = 0;
	}
}
