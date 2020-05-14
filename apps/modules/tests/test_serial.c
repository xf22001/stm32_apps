

/*================================================================
 *
 *
 *   文件名称：test_serial.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 10时54分01秒
 *   修改日期：2020年05月14日 星期四 14时44分53秒
 *   描    述：
 *
 *================================================================*/
#include "test_serial.h"
#include <string.h>
#include "usart_txrx.h"

#include "os_utils.h"
#define UDP_LOG
#include "task_probe_tool.h"

#define _printf udp_log_printf
#define _hexdump udp_log_hexdump
#define _puts udp_log_puts

static void task_uart_tx(void const *argument)
{
	int ret = 0;
	uart_info_t *uart_info = (uart_info_t *)argument;
	char *msg = "xiaofei";

	if(uart_info == NULL) {
		app_panic();
	}

	while(1) {
		ret = uart_tx_data(uart_info, (uint8_t *)msg, strlen(msg) + 1, 10);

		if(ret > 0) {
			_printf("\n\n");
			_printf("sent msg!\n");
		}

		osDelay(200);
	}
}


static char rx_msg[64];
static void task_uart_rx(void const *argument)
{
	int ret = 0;
	uart_info_t *uart_info = (uart_info_t *)argument;

	if(uart_info == NULL) {
		app_panic();
	}

	while(1) {
		ret = uart_rx_data(uart_info, (uint8_t *)rx_msg, 64, 500);

		if(ret > 0) {
			_hexdump("rx_msg", (const char *)rx_msg, ret);
		}
	}
}

void serial_self_test(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = get_or_alloc_uart_info(huart);

	if(uart_info == NULL) {
		app_panic();
	}

	osThreadDef(task_uart_tx, task_uart_tx, osPriorityNormal, 0, 128 * 3);
	osThreadCreate(osThread(task_uart_tx), uart_info);

	osThreadDef(task_uart_rx, task_uart_rx, osPriorityNormal, 0, 128 * 3);
	osThreadCreate(osThread(task_uart_rx), uart_info);
}

static char buffer[128];

void task_test_serial(void const *argument)
{
	int ret = 0;
	uart_info_t *uart_info = (uart_info_t *)argument;

	if(uart_info == NULL) {
		app_panic();
	}

	while(1) {
		ret = uart_rx_data(uart_info, (uint8_t *)buffer, 128, 1000);

		if(ret > 0) {
			_hexdump("buffer", (const char *)buffer, ret);
		}
	}
}
