

/*================================================================
 *
 *
 *   文件名称：uart_data_task.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月25日 星期一 12时51分31秒
 *   修改日期：2021年06月23日 星期三 10时06分29秒
 *   描    述：
 *
 *================================================================*/
#include "uart_data_task.h"
#include <string.h>

#include "object_class.h"
#include "os_utils.h"
#include "log.h"

static object_class_t *uart_data_task_class = NULL;

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

int set_uart_data_task_request_delay(uart_data_task_info_t *uart_data_task_info, uint32_t delay)
{
	uart_data_task_info->request_delay = delay;
}

static void uart_data_task(void const *argument)
{
	uart_data_task_info_t *uart_data_task_info = (uart_data_task_info_t *)argument;

	if(uart_data_task_info == NULL) {
		app_panic();
	}

	for(;;) {
		if(callback_chain_empty(uart_data_task_info->uart_data_chain)) {
			osDelay(10);
		} else {
			do_callback_chain(uart_data_task_info->uart_data_chain, uart_data_task_info);

			if(uart_data_task_info->request_delay != 0) {
				osDelay(uart_data_task_info->request_delay);
			}
		}
	}
}

static void free_uart_data_task_info(uart_data_task_info_t *uart_data_task_info)
{
	app_panic();
}

static uart_data_task_info_t *alloc_uart_data_task_info(UART_HandleTypeDef *huart)
{
	uart_data_task_info_t *uart_data_task_info = NULL;

	if(huart == NULL) {
		return uart_data_task_info;
	}

	uart_data_task_info = (uart_data_task_info_t *)os_calloc(1, sizeof(uart_data_task_info_t));
	OS_ASSERT(uart_data_task_info != NULL);
	uart_data_task_info->request_delay = 10;

	uart_data_task_info->uart_info = get_or_alloc_uart_info(huart);
	OS_ASSERT(uart_data_task_info->uart_info != NULL);

	uart_data_task_info->uart_data_chain = alloc_callback_chain();
	OS_ASSERT(uart_data_task_info->uart_data_chain != NULL);

	osThreadDef(uart_data, uart_data_task, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(uart_data), uart_data_task_info);

	return uart_data_task_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	uart_data_task_info_t *uart_data_task_info = (uart_data_task_info_t *)o;
	UART_HandleTypeDef *huart = (UART_HandleTypeDef *)ctx;

	if(uart_data_task_info->uart_info->huart == huart) {
		ret = 0;
	}

	return ret;
}

uart_data_task_info_t *get_or_alloc_uart_data_task_info(UART_HandleTypeDef *huart)
{
	uart_data_task_info_t *uart_data_task_info = NULL;

	os_enter_critical();

	if(uart_data_task_class == NULL) {
		uart_data_task_class = object_class_alloc();
	}

	os_leave_critical();

	uart_data_task_info = (uart_data_task_info_t *)object_class_get_or_alloc_object(uart_data_task_class, object_filter, huart, (object_alloc_t)alloc_uart_data_task_info, (object_free_t)free_uart_data_task_info);

	return uart_data_task_info;
}
