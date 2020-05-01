

/*================================================================
 *   
 *   
 *   文件名称：task_modbus_slave.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 15时13分37秒
 *   修改日期：2020年05月01日 星期五 18时30分21秒
 *   描    述：
 *
 *================================================================*/
#include "task_modbus_slave.h"
#include "modbus_slave_txrx.h"
#include "os_utils.h"

void task_modbus_slave(void const *argument)
{
	modbus_slave_info_t *modbus_slave_info = (modbus_slave_info_t *)argument;

	if(modbus_slave_info == NULL) {
		app_panic();
	}

	while(1) {
		int ret = modbus_slave_process_request(modbus_slave_info);

		if(ret != 0) {
		}
	}
}
