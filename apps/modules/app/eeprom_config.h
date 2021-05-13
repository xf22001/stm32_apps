

/*================================================================
 *   
 *   
 *   文件名称：eeprom_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年12月17日 星期四 14时02分23秒
 *   修改日期：2021年05月13日 星期四 15时16分40秒
 *   描    述：
 *
 *================================================================*/
#ifndef _EEPROM_CONFIG_H
#define _EEPROM_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "eeprom.h"

#ifdef __cplusplus
}
#endif

#pragma pack(push, 1)

typedef struct {
	unsigned char crc;
	unsigned short payload_size;
} eeprom_config_item_head_t;

#pragma pack(pop)

int eeprom_load_config_item(eeprom_info_t *eeprom_info, const char *label, void *config, size_t size, size_t offset);
int eeprom_save_config_item(eeprom_info_t *eeprom_info, const char *label, void *config, size_t size, size_t offset);

#endif //_EEPROM_CONFIG_H
