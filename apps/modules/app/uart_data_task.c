

/*================================================================
 *   
 *   
 *   文件名称：uart_data_task.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月25日 星期一 12时51分31秒
 *   修改日期：2021年01月25日 星期一 13时16分11秒
 *   描    述：
 *
 *================================================================*/
#include "uart_data_task.h"
#include <string.h>

#include "map_utils.h"
#include "os_utils.h"
#include "log.h"

static map_utils_t *uart_data_task_map = NULL;

int add_uart_data_task_info_cb(uart_data_task_info_t *uart_data_task_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = register_callback(uart_data_task_info->uart_data_chain, callback_item);
	return ret;
}

int remove_uart_data_task_info_cb(uart_data_task_info_t *uart_data_task_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = remove_callback(uart_data_task_info->uart_data_chain, callback_item);
	return ret;
}

static void uart_data_task(void const *argument)
{
	uart_data_task_info_t *uart_data_task_info = (uart_data_task_info_t *)argument;

	if(uart_data_task_info == NULL) {
		app_panic();
	}

	for(;;) {
		do_callback_chain(uart_data_task_info->uart_data_chain, uart_data_task_info);
		osDelay(10);
	}
}

static void free_uart_data_task_info(uart_data_task_info_t *uart_data_task_info)
{
	if(uart_data_task_info == NULL) {
		return;
	}

	if(uart_data_task_info->uart_data_chain != NULL) {
		free_callback_chain(uart_data_task_info->uart_data_chain);
	}

	os_free(uart_data_task_info);
}

static uart_data_task_info_t *alloc_uart_data_task_info(UART_HandleTypeDef *huart)
{
	uart_data_task_info_t *uart_data_task_info = NULL;

	if(huart == NULL) {
		return uart_data_task_info;
	}

	uart_data_task_info = (uart_data_task_info_t *)os_alloc(sizeof(uart_data_task_info_t));

	if(uart_data_task_info == NULL) {
		return uart_data_task_info;
	}

	memset(uart_data_task_info, 0, sizeof(uart_data_task_info_t));

	uart_data_task_info->uart_info = get_or_alloc_uart_info(huart);

	if(uart_data_task_info->uart_info == NULL) {
		debug("\n");
		goto failed;
	}

	uart_data_task_info->uart_data_chain = alloc_callback_chain();

	if(uart_data_task_info->uart_data_chain == NULL) {
		debug("\n");
		goto failed;
	}

	osThreadDef(uart_data, uart_data_task, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(uart_data), uart_data_task_info);

	return uart_data_task_info;
failed:
	free_uart_data_task_info(uart_data_task_info);
	uart_data_task_info = NULL;
	return uart_data_task_info;
}

uart_data_task_info_t *get_or_alloc_uart_data_task_info(UART_HandleTypeDef *huart)
{
	uart_data_task_info_t *uart_data_task_info = NULL;

	__disable_irq();

	if(uart_data_task_map == NULL) {
		uart_data_task_map = map_utils_alloc(NULL);
	}

	__enable_irq();

	uart_data_task_info = (uart_data_task_info_t *)map_utils_get_or_alloc_value(uart_data_task_map, huart, (map_utils_value_alloc_t)alloc_uart_data_task_info, (map_utils_value_free_t)free_uart_data_task_info);

	return uart_data_task_info;
}
