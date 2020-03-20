

/*================================================================
 *   
 *   
 *   文件名称：eeprom.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月14日 星期四 09时01分48秒
 *   修改日期：2020年01月20日 星期一 11时28分34秒
 *   描    述：
 *
 *================================================================*/
#ifndef _EEPROM_H
#define _EEPROM_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "main.h"
#include "spi_txrx.h"
#include "list_utils.h"

typedef struct {
	struct list_head list;
	spi_info_t *spi_info;
	osMutexId mutex;

	GPIO_TypeDef *cs_gpio;
	uint16_t cs_pin;
	GPIO_TypeDef *wp_gpio;
	uint16_t wp_pin;
} eeprom_info_t;

typedef struct {
	uint8_t fn;
} eeprom_fn_t;

typedef struct {
	uint8_t b2;
	uint8_t b1;
	uint8_t b0;
} eeprom_addr_t;

typedef struct {
	eeprom_fn_t fn;
	eeprom_addr_t addr;
} eeprom_request_t;

#define EEPROM_1024_PAGE 256

eeprom_info_t *get_eeprom_info(spi_info_t *spi_info);
void free_eeprom_info(eeprom_info_t *eeprom_info);
eeprom_info_t *alloc_eeprom_info(spi_info_t *spi_info);
uint8_t eeprom_id(eeprom_info_t *eeprom_info);
uint8_t eeprom_read(eeprom_info_t *eeprom_info, uint32_t start, uint8_t *data, uint16_t size);
uint8_t eeprom_write(eeprom_info_t *eeprom_info, uint32_t start, uint8_t *data, uint16_t size);
#endif //_EEPROM_H
