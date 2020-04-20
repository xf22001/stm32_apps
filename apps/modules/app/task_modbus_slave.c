

/*================================================================
 *   
 *   
 *   文件名称：task_modbus_slave.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 15时13分37秒
 *   修改日期：2020年04月20日 星期一 15时13分57秒
 *   描    述：
 *
 *================================================================*/
#include "task_modbus_slave.h"
#include "modbus_slave_txrx.h"
#include "os_utils.h"

void task_modbus_slave(void const *argument)
{
	uart_info_t *uart_info = (uart_info_t *)argument;

	if(uart_info == NULL) {
		app_panic();
	}

	modbus_slave_info_t *modbus_slave_info = get_or_alloc_modbus_slave_info(uart_info);;

	if(modbus_slave_info == NULL) {
		app_panic();
	}

	while(1) {
		int ret = modbus_slave_process_request(modbus_slave_info);

		if(ret != 0) {
		}
	}
}
