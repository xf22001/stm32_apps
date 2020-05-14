

/*================================================================
 *   
 *   
 *   文件名称：modbus_spec.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月22日 星期三 11时47分17秒
 *   修改日期：2020年05月14日 星期四 13时12分05秒
 *   描    述：
 *
 *================================================================*/
#include "modbus_spec.h"
#include "os_utils.h"
//#define UDP_LOG
#include "task_probe_tool.h"

#define _printf udp_log_printf

uint16_t modbus_calc_crc(uint8_t *data, uint16_t size)
{
	uint16_t crc = 0xFFFF;
	uint16_t i;

	_printf("modbus_calc_crc size:%d\n", size);

	for(i = 0; i < size; i++) {
		uint16_t loop;

		crc = crc ^ data[i];

		for(loop = 0; loop < 8; loop++) {
			if(crc & 1) {
				crc >>= 1;
				crc ^= 0xa001;
			} else {
				crc >>= 1;
			}
		}
	}

	return (crc);
}
