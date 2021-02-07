

/*================================================================
 *
 *
 *   文件名称：test_serial.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 10时54分01秒
 *   修改日期：2021年02月07日 星期日 15时04分07秒
 *   描    述：
 *
 *================================================================*/
#include "test_serial.h"
#include <string.h>

#include "usart_txrx.h"
#include "os_utils.h"

#define LOG_UDP
#include "log.h"

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

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
			//_printf("%s sent msg %s\n", (uart_info->huart == &huart1) ? "uart1" : "uart3", msg);
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
			_printf("%s recv msg:", (uart_info->huart == &huart1) ? "uart1" : "uart3");
			_hexdump(NULL, (const char *)rx_msg, ret);
		}
	}
}

static void task_uart_tx_rx(void const *argument)
{
	int ret = 0;
	uart_info_t *uart_info = (uart_info_t *)argument;
	char *msg = "xiaofei";

	if(uart_info == NULL) {
		app_panic();
	}

	while(1) {
		ret = uart_tx_rx_data(uart_info, (uint8_t *)msg, strlen(msg) + 1, (uint8_t *)rx_msg, 64, 500);

		if(ret > 0) {
			_printf("%s recv msg:", (uart_info->huart == &huart1) ? "uart1" : "uart3");
			_hexdump(NULL, (const char *)rx_msg, ret);
		}
	}
}

void test_serial(UART_HandleTypeDef *huart)
{
	uart_info_t *uart_info = get_or_alloc_uart_info(huart);

	if(uart_info == NULL) {
		app_panic();
	}

	//osThreadDef(task_uart_tx, task_uart_tx, osPriorityNormal, 0, 128 * 3);
	//osThreadCreate(osThread(task_uart_tx), uart_info);

	//osThreadDef(task_uart_rx, task_uart_rx, osPriorityNormal, 0, 128 * 3);
	//osThreadCreate(osThread(task_uart_rx), uart_info);

	osThreadDef(task_uart_tx_rx, task_uart_tx_rx, osPriorityNormal, 0, 128 * 3);
	osThreadCreate(osThread(task_uart_tx_rx), uart_info);
}
