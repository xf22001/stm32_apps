

/*================================================================
 *   
 *   
 *   文件名称：task_modbus.c
 *   创 建 者：肖飞
 *   创建日期：2020年03月20日 星期五 10时41分24秒
 *   修改日期：2020年03月20日 星期五 10时41分45秒
 *   描    述：
 *
 *================================================================*/
#include "task_modbus.h"
#include "modbus_txrx.h"
#include "os_utils.h"

void task_modbus(void const *argument)
{
	uart_info_t *uart_info = (uart_info_t *)argument;

	if(uart_info == NULL) {
		app_panic();
	}

	modbus_info_t *modbus_info = alloc_modbus_info(uart_info);;

	if(modbus_info == NULL) {
		app_panic();
	}

	while(1) {
		int ret = modbus_process_request(modbus_info);

		if(ret != 0) {
		}
	}
}
