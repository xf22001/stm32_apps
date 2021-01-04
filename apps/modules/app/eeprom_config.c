

/*================================================================
 *
 *
 *   文件名称：eeprom_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月17日 星期四 14时02分18秒
 *   修改日期：2021年01月04日 星期一 14时13分49秒
 *   描    述：
 *
 *================================================================*/
#include "eeprom_config.h"
#include <string.h>
#include "log.h"

int eeprom_load_config_item(eeprom_info_t *eeprom_info, const char *label, void *config, size_t size, size_t offset)
{
	int ret = -1;
	unsigned char crc;
	eeprom_config_item_head_t eeprom_config_item_head;

	///debug("offset:%d, size:%d\n", offset, size);

	if(detect_eeprom(eeprom_info) != 0) {
		debug("\n");
		return ret;
	}

	eeprom_read(eeprom_info, offset, (uint8_t *)&eeprom_config_item_head, sizeof(eeprom_config_item_head_t));
	offset += sizeof(eeprom_config_item_head_t);

	if(eeprom_config_item_head.payload_size != size) {
		debug("\n");
		return ret;
	}

	crc = size;

	if(label != NULL) {
		crc += calc_crc8(label, strlen(label));
	}

	eeprom_read(eeprom_info, offset, (uint8_t *)config, size);

	crc += calc_crc8(config, size);

	if(crc != eeprom_config_item_head.crc) {
		debug("\n");
		return ret;
	}

	ret = 0;

	return ret;
}

int eeprom_save_config_item(eeprom_info_t *eeprom_info, const char *label, void *config, size_t size, size_t offset)
{
	int ret = -1;
	unsigned char crc;
	eeprom_config_item_head_t eeprom_config_item_head;

	//debug("offset:%d, size:%d\n", offset, size);

	if(detect_eeprom(eeprom_info) != 0) {
		debug("\n");
		return ret;
	}

	eeprom_config_item_head.payload_size = size;

	crc = size;

	if(label != NULL) {
		crc += calc_crc8(label, strlen(label));
	}

	crc += calc_crc8(config, size);

	eeprom_config_item_head.crc = crc;

	eeprom_write(eeprom_info, offset, (uint8_t *)&eeprom_config_item_head, sizeof(eeprom_config_item_head_t));

	offset += sizeof(eeprom_config_item_head_t);

	eeprom_write(eeprom_info, offset, (uint8_t *)config, size);

	ret = 0;

	return ret;
}

