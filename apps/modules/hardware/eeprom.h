

/*================================================================
 *   
 *   
 *   文件名称：eeprom.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月14日 星期四 09时01分48秒
 *   修改日期：2021年01月30日 星期六 08时04分39秒
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
#include "os_utils.h"

typedef struct {
	spi_info_t *spi_info;
	os_mutex_t mutex;

	GPIO_TypeDef *gpio_port_spi_cs;
	uint16_t gpio_pin_spi_cs;
	GPIO_TypeDef *gpio_port_spi_wp;
	uint16_t gpio_pin_spi_wp;
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

eeprom_info_t *get_or_alloc_eeprom_info(spi_info_t *spi_info, GPIO_TypeDef *gpio_port_spi_cs, uint16_t gpio_pin_spi_cs, GPIO_TypeDef *gpio_port_spi_wp, uint16_t gpio_pin_spi_wp);
uint8_t eeprom_id(eeprom_info_t *eeprom_info);
uint8_t eeprom_read(eeprom_info_t *eeprom_info, uint32_t start, uint8_t *data, uint16_t size);
uint8_t eeprom_write(eeprom_info_t *eeprom_info, uint32_t start, uint8_t *data, uint16_t size);
int detect_eeprom(eeprom_info_t *eeprom_info);
#endif //_EEPROM_H
