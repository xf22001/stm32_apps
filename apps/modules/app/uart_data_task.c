

/*================================================================
 *   
 *   
 *   文件名称：uart_data_task.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月25日 星期一 12时51分31秒
 *   修改日期：2021年02月02日 星期二 13时10分24秒
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
		debug("");
		goto failed;
	}

	uart_data_task_info->uart_data_chain = alloc_callback_chain();

	if(uart_data_task_info->uart_data_chain == NULL) {
		debug("");
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
