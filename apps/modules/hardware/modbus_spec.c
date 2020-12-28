

/*================================================================
 *   
 *   
 *   文件名称：modbus_spec.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月22日 星期三 11时47分17秒
 *   修改日期：2020年12月28日 星期一 13时24分31秒
 *   描    述：
 *
 *================================================================*/
#include "modbus_spec.h"
#include "os_utils.h"

#define LOG_NONE
#include "log.h"

uint16_t modbus_calc_crc(uint8_t *data, uint16_t size)
{
	uint16_t crc = 0xFFFF;
	uint16_t i;

	debug("modbus_calc_crc size:%d\n", size);

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
