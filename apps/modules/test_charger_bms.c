

/*================================================================
 *
 *
 *   文件名称：test_charger_bms.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 14时28分36秒
 *   修改日期：2020年04月01日 星期三 12时56分35秒
 *   描    述：
 *
 *================================================================*/
#include "test_charger_bms.h"

#include "os_utils.h"

#include "charger.h"
#include "can_txrx.h"
#include "can.h"

#include "bms.h"
#include "charger.h"
#include "task_modbus.h"

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
		int ret = can_rx_data(charger_info->can_info, 10);

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
		int ret = can_rx_data(bms_info->can_info, 10);

		if(ret == 0) {
			bms_handle_response(bms_info);
		}
	}
}

void test_charger_bms(void)
{
	{
		can_info_t *can_info = alloc_can_info(&hcan1);
		osThreadDef(charger_request, task_charger_request, osPriorityNormal, 0, 256);
		osThreadDef(charger_response, task_charger_response, osPriorityNormal, 0, 256);
		charger_info_t *charger_info;

		if(can_info == NULL) {
			app_panic();
		}

		set_can_info_hal_init(can_info, MX_CAN1_Init);

		charger_info = alloc_charger_info(can_info);

		if(charger_info == NULL) {
			app_panic();
		}

		osThreadCreate(osThread(charger_request), charger_info);
		osThreadCreate(osThread(charger_response), charger_info);
	}

	{
		uart_info_t *uart_info = alloc_uart_info(&huart1);

		if(uart_info == NULL) {
			app_panic();
		}

		osThreadDef(task_modbus, task_modbus, osPriorityNormal, 0, 128 * 3);
		osThreadCreate(osThread(task_modbus), uart_info);
	}

	{
		can_info_t *can_info = alloc_can_info(&hcan2);
		uart_info_t *uart_info = alloc_uart_info(&huart1);
		spi_info_t *spi_info = alloc_spi_info(&hspi3);
		bms_info_t *bms_info;
		eeprom_info_t *eeprom_info;
		modbus_info_t *modbus_info;
		osThreadDef(bms_request, task_bms_request, osPriorityNormal, 0, 256);
		osThreadDef(bms_response, task_bms_response, osPriorityNormal, 0, 256);

		if(can_info == NULL) {
			app_panic();
		}

		if(uart_info == NULL) {
			app_panic();
		}

		if(spi_info == NULL) {
			app_panic();
		}

		set_can_info_hal_init(can_info, MX_CAN2_Init);

		eeprom_info = alloc_eeprom_info(spi_info);

		if(eeprom_info == NULL) {
			app_panic();
		}

		modbus_info = alloc_modbus_info(uart_info);

		if(modbus_info == NULL) {
			app_panic();
		}

		bms_info = alloc_bms_info(can_info);

		if(bms_info == NULL) {
			app_panic();
		}

		bms_set_modbus_info(bms_info, modbus_info);
		bms_set_eeprom_info(bms_info, eeprom_info);

		bms_restore_data(bms_info);

		osThreadCreate(osThread(bms_request), bms_info);
		osThreadCreate(osThread(bms_response), bms_info);
	}
}
