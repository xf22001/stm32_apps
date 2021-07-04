

/*================================================================
 *
 *
 *   文件名称：eeprom_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月17日 星期四 14时02分18秒
 *   修改日期：2021年07月04日 星期日 22时05分51秒
 *   描    述：
 *
 *================================================================*/
#include "eeprom_config.h"
#include <string.h>
#include "os_utils.h"
#include "log.h"

int eeprom_load_config_item(eeprom_info_t *eeprom_info, const char *label, void *config, size_t size, size_t offset)
{
	int ret = -1;
	unsigned char crc;
	eeprom_config_item_head_t eeprom_config_item_head;

	debug("offset:%d, size:%d", offset, size);

	if(detect_eeprom(eeprom_info) != 0) {
		debug("");
		return ret;
	}

	OS_ASSERT(offset >= sizeof(eeprom_config_item_head_t));

	eeprom_read(eeprom_info, offset - sizeof(eeprom_config_item_head_t), (uint8_t *)&eeprom_config_item_head, sizeof(eeprom_config_item_head_t));

	if(eeprom_config_item_head.payload_size != size) {
		debug("eeprom_config_item_head.payload_size:%d, size:%d", eeprom_config_item_head.payload_size, size);
		return ret;
	}

	crc = size;

	if(label != NULL) {
		crc += sum_crc8(label, strlen(label));
	}

	eeprom_read(eeprom_info, offset, (uint8_t *)config, size);

	crc += sum_crc8(config, size);

	if(crc != eeprom_config_item_head.crc) {
		debug("");
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

	debug("offset:%d, size:%d", offset, size);

	if(detect_eeprom(eeprom_info) != 0) {
		debug("");
		return ret;
	}

	OS_ASSERT(offset >= sizeof(eeprom_config_item_head_t));

	eeprom_config_item_head.payload_size = size;

	crc = size;

	if(label != NULL) {
		crc += sum_crc8(label, strlen(label));
	}

	crc += sum_crc8(config, size);

	eeprom_config_item_head.crc = crc;

	eeprom_write(eeprom_info, offset - sizeof(eeprom_config_item_head_t), (uint8_t *)&eeprom_config_item_head, sizeof(eeprom_config_item_head_t));

	eeprom_write(eeprom_info, offset, (uint8_t *)config, size);

	ret = 0;

	return ret;
}

