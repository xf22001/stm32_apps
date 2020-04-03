

/*================================================================
 *
 *
 *   文件名称：test_serial.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 10时54分01秒
 *   修改日期：2020年04月03日 星期五 12时37分28秒
 *   描    述：
 *
 *================================================================*/
#include "test_serial.h"
#include <string.h>

#define UART_LOG
#include "usart_txrx.h"

#include "os_utils.h"
#define UDP_LOG
#include "task_probe_tool.h"

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
			udp_log_printf("\n\n");
			udp_log_printf("sent msg!\n");
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
			udp_log_hexdump("rx_msg", (const char *)rx_msg, ret);
		}
	}
}

void serial_self_test(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = alloc_uart_info(huart);

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

	set_log_uart_info(uart_info->huart);

	while(1) {
		ret = uart_rx_data(uart_info, (uint8_t *)buffer, 128, 1000);

		if(ret > 0) {
			uart_log_hexdump("buffer", (const char *)buffer, ret);
		}
	}
}
