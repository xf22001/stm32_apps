

/*================================================================
 *
 *
 *   文件名称：can_data_task.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月19日 星期二 16时15分20秒
 *   修改日期：2021年01月20日 星期三 14时21分37秒
 *   描    述：
 *
 *================================================================*/
#include "can_data_task.h"
#include <string.h>

#include "map_utils.h"
#include "os_utils.h"
#include "log.h"


static map_utils_t *can_data_task_map = NULL;

int add_can_data_task_info_request_cb(can_data_task_info_t *can_data_task_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = register_callback(can_data_task_info->can_data_request_chain, callback_item);
	return ret;
}

int remove_can_data_task_info_request_cb(can_data_task_info_t *can_data_task_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = remove_callback(can_data_task_info->can_data_request_chain, callback_item);
	return ret;
}

int add_can_data_task_info_response_cb(can_data_task_info_t *can_data_task_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = register_callback(can_data_task_info->can_data_response_chain, callback_item);
	return ret;
}

int remove_can_data_task_info_response_cb(can_data_task_info_t *can_data_task_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = remove_callback(can_data_task_info->can_data_response_chain, callback_item);
	return ret;
}


static void can_data_task_request(void const *argument)
{
	can_data_task_info_t *can_data_task_info = (can_data_task_info_t *)argument;

	if(can_data_task_info == NULL) {
		app_panic();
	}

	for(;;) {
		do_callback_chain(can_data_task_info->can_data_request_chain, can_data_task_info);
		osDelay(10);
	}
}

static void can_data_task_response(void const *argument)
{
	can_data_task_info_t *can_data_task_info = (can_data_task_info_t *)argument;

	if(can_data_task_info == NULL) {
		app_panic();
	}

	for(;;) {
		int ret = can_rx_data(can_data_task_info->can_info, osWaitForever);

		if(ret == 0) {
			do_callback_chain(can_data_task_info->can_data_response_chain, can_data_task_info);
		}
	}
}

static void free_can_data_task_info(can_data_task_info_t *can_data_task_info)
{
	if(can_data_task_info == NULL) {
		return;
	}

	free_callback_chain(can_data_task_info->can_data_request_chain);
	free_callback_chain(can_data_task_info->can_data_response_chain);

	can_data_task_info->can_data_request_chain = NULL;
	can_data_task_info->can_data_response_chain = NULL;

	os_free(can_data_task_info);
}

static can_data_task_info_t *alloc_can_data_task_info(CAN_HandleTypeDef *hcan)
{
	can_data_task_info_t *can_data_task_info = NULL;

	if(hcan == NULL) {
		return can_data_task_info;
	}

	can_data_task_info = (can_data_task_info_t *)os_alloc(sizeof(can_data_task_info_t));

	if(can_data_task_info == NULL) {
		return can_data_task_info;
	}

	memset(can_data_task_info, 0, sizeof(can_data_task_info_t));

	can_data_task_info->can_info = get_or_alloc_can_info(hcan);

	if(can_data_task_info->can_info == NULL) {
		debug("\n");
		goto failed;
	}

	can_data_task_info->can_data_request_chain = alloc_callback_chain();

	if(can_data_task_info->can_data_request_chain == NULL) {
		debug("\n");
		goto failed;
	}

	can_data_task_info->can_data_response_chain = alloc_callback_chain();

	if(can_data_task_info->can_data_response_chain == NULL) {
		debug("\n");
		goto failed;
	}

	osThreadDef(can_request, can_data_task_request, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(can_request), can_data_task_info);

	osThreadDef(can_response, can_data_task_response, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(can_response), can_data_task_info);

	return can_data_task_info;
failed:
	free_can_data_task_info(can_data_task_info);
	can_data_task_info = NULL;
	return can_data_task_info;
}

can_data_task_info_t *get_or_alloc_can_data_task_info(CAN_HandleTypeDef *hcan)
{
	can_data_task_info_t *can_data_task_info = NULL;

	__disable_irq();

	if(can_data_task_map == NULL) {
		can_data_task_map = map_utils_alloc(NULL);
	}

	__enable_irq();

	can_data_task_info = (can_data_task_info_t *)map_utils_get_or_alloc_value(can_data_task_map, hcan, (map_utils_value_alloc_t)alloc_can_data_task_info, (map_utils_value_free_t)free_can_data_task_info);

	return can_data_task_info;
}
