

/*================================================================
 *
 *
 *   文件名称：test_serial.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 10时54分01秒
 *   修改日期：2020年03月30日 星期一 14时20分59秒
 *   描    述：
 *
 *================================================================*/
#include "test_serial.h"
#define UART_LOG
#include "usart_txrx.h"
#include "os_utils.h"

char buffer[128];

void task_test_serial(void const *argument)
{
	int ret = 0;
	uart_info_t *uart_info = (uart_info_t *)argument;

	if(uart_info == NULL) {
		app_panic();
	}

	set_log_uart_info(uart_info->huart);

	while(1) {
		ret = uart_rx_data(uart_info, (uint8_t *)buffer, 128, 1000);

		if(ret > 0) {
			uart_log_hexdump("buffer", (const char *)buffer, ret);
		}
	}
}
