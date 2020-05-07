

/*================================================================
 *
 *
 *   文件名称：test_charger_bms.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 14时28分36秒
 *   修改日期：2020年05月07日 星期四 10时12分59秒
 *   描    述：
 *
 *================================================================*/
#include "test_charger_bms.h"

#include "os_utils.h"

#include "charger.h"
#include "can_txrx.h"

#include "bms_config.h"
#include "bms.h"
#include "channel_config.h"
#include "charger.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
extern UART_HandleTypeDef huart1;
extern SPI_HandleTypeDef hspi3;

static void task_charger_request(void const *argument)
{
	charger_info_t *charger_info = (charger_info_t *)argument;

	if(charger_info == NULL) {
		app_panic();
	}

	for(;;) {
		charger_periodic(charger_info);
		charger_handle_request(charger_info);
		osDelay(10);
	}
}

static void task_charger_response(void const *argument)
{
	charger_info_t *charger_info = (charger_info_t *)argument;

	if(charger_info == NULL) {
		app_panic();
	}

	if(charger_info->can_info->receive_init) {
		charger_info->can_info->receive_init(charger_info->can_info->hcan);
	}

	for(;;) {
		int ret = can_rx_data(charger_info->can_info, 1000);

		if(ret == 0) {
			charger_handle_response(charger_info);
		}
	}
}

static void task_bms_request(void const *argument)
{
	bms_info_t *bms_info = (bms_info_t *)argument;

	if(bms_info == NULL) {
		app_panic();
	}

	for(;;) {
		bms_periodic(bms_info);
		bms_handle_request(bms_info);
		osDelay(10);
	}
}

static void task_bms_response(void const *argument)
{
	bms_info_t *bms_info = (bms_info_t *)argument;

	if(bms_info == NULL) {
		app_panic();
	}

	if(bms_info->settings == NULL) {
		app_panic();
	}

	if(bms_info->can_info->receive_init) {
		bms_info->can_info->receive_init(bms_info->can_info->hcan);
	}

	for(;;) {
		int ret = can_rx_data(bms_info->can_info, 1000);

		if(ret == 0) {
			bms_handle_response(bms_info);
		}
	}
}

void test_charger_bms(void)
{
	{
		channel_info_config_t *channel_info_config = get_channel_info_config(0);
		osThreadDef(charger_request, task_charger_request, osPriorityNormal, 0, 256);
		osThreadDef(charger_response, task_charger_response, osPriorityNormal, 0, 256);
		charger_info_t *charger_info;

		if(channel_info_config == NULL) {
			app_panic();
		}

		charger_info = get_or_alloc_charger_info(channel_info_config);

		if(charger_info == NULL) {
			app_panic();
		}

		osThreadCreate(osThread(charger_request), charger_info);
		osThreadCreate(osThread(charger_response), charger_info);
	}

	{
		bms_info_config_t *bms_info_config = get_bms_info_config(0);
		bms_info_t *bms_info;

		osThreadDef(bms_request, task_bms_request, osPriorityNormal, 0, 256);
		osThreadDef(bms_response, task_bms_response, osPriorityNormal, 0, 256);

		if(bms_info_config == NULL) {
			app_panic();
		}

		bms_info = get_or_alloc_bms_info(bms_info_config);

		if(bms_info == NULL) {
			app_panic();
		}

		bms_restore_data(bms_info);

		osThreadCreate(osThread(bms_request), bms_info);
		osThreadCreate(osThread(bms_response), bms_info);
	}
}
