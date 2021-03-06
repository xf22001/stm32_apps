

/*================================================================
 *
 *
 *   文件名称：eeprom.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月14日 星期四 09时01分36秒
 *   修改日期：2021年05月11日 星期二 14时14分53秒
 *   描    述：
 *
 *================================================================*/
#include "eeprom.h"

#include <string.h>

#include "os_utils.h"
#include "object_class.h"
#include "log.h"

static object_class_t *eeprom_class = NULL;

static void free_eeprom_info(eeprom_info_t *eeprom_info)
{
	if(eeprom_info == NULL) {
		return;
	}

	mutex_delete(eeprom_info->mutex);

	if(eeprom_info->spi_info) {
		eeprom_info->spi_info = NULL;
	}

	os_free(eeprom_info);
}

static eeprom_info_t *alloc_eeprom_info(spi_info_t *spi_info)
{
	eeprom_info_t *eeprom_info = NULL;

	if(spi_info == NULL) {
		return eeprom_info;
	}

	eeprom_info = (eeprom_info_t *)os_alloc(sizeof(eeprom_info_t));

	if(eeprom_info == NULL) {
		return eeprom_info;
	}

	memset(eeprom_info, 0, sizeof(eeprom_info_t));

	eeprom_info->spi_info = spi_info;

	eeprom_info->mutex = mutex_create();

	if(eeprom_info->mutex == NULL) {
		goto failed;
	}

	return eeprom_info;
failed:
	free_eeprom_info(eeprom_info);
	eeprom_info = NULL;
	return eeprom_info;
}

static void set_eeprom_gpio_pins(eeprom_info_t *eeprom_info, GPIO_TypeDef *gpio_port_spi_cs, uint16_t gpio_pin_spi_cs, GPIO_TypeDef *gpio_port_spi_wp, uint16_t gpio_pin_spi_wp)
{
	if(eeprom_info == NULL) {
		return;
	}

	eeprom_info->gpio_port_spi_cs = gpio_port_spi_cs;
	eeprom_info->gpio_pin_spi_cs = gpio_pin_spi_cs;
	eeprom_info->gpio_port_spi_wp = gpio_port_spi_wp;
	eeprom_info->gpio_pin_spi_wp = gpio_pin_spi_wp;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	eeprom_info_t *eeprom_info = (eeprom_info_t *)o;
	spi_info_t *spi_info = (spi_info_t *)ctx;

	if(eeprom_info->spi_info == spi_info) {
		ret = 0;
	}

	return ret;
}

eeprom_info_t *get_or_alloc_eeprom_info(spi_info_t *spi_info, GPIO_TypeDef *gpio_port_spi_cs, uint16_t gpio_pin_spi_cs, GPIO_TypeDef *gpio_port_spi_wp, uint16_t gpio_pin_spi_wp)
{
	eeprom_info_t *eeprom_info = NULL;

	os_enter_critical();

	if(eeprom_class == NULL) {
		eeprom_class = object_class_alloc();
	}

	os_leave_critical();

	eeprom_info = (eeprom_info_t *)object_class_get_or_alloc_object(eeprom_class, object_filter, spi_info, (object_alloc_t)alloc_eeprom_info, (object_free_t)free_eeprom_info);

	if(eeprom_info != NULL) {
		set_eeprom_gpio_pins(eeprom_info, gpio_port_spi_cs, gpio_pin_spi_cs, gpio_port_spi_wp, gpio_pin_spi_wp);
	}

	return eeprom_info;
}


static inline void eeprom_enable_cs(eeprom_info_t *eeprom_info)
{
	if(eeprom_info->gpio_port_spi_cs != NULL) {
		HAL_GPIO_WritePin(eeprom_info->gpio_port_spi_cs, eeprom_info->gpio_pin_spi_cs, GPIO_PIN_RESET);
	}
}

static inline void eeprom_disable_cs(eeprom_info_t *eeprom_info)
{
	if(eeprom_info->gpio_port_spi_cs != NULL) {
		HAL_GPIO_WritePin(eeprom_info->gpio_port_spi_cs, eeprom_info->gpio_pin_spi_cs, GPIO_PIN_SET);
	}
}

static inline void eeprom_enable_wp(eeprom_info_t *eeprom_info)
{
	if(eeprom_info->gpio_port_spi_wp != NULL) {
		HAL_GPIO_WritePin(eeprom_info->gpio_port_spi_wp, eeprom_info->gpio_pin_spi_wp, GPIO_PIN_RESET);
	}
}

static inline void eeprom_disable_wp(eeprom_info_t *eeprom_info)
{
	if(eeprom_info->gpio_port_spi_wp != NULL) {
		HAL_GPIO_WritePin(eeprom_info->gpio_port_spi_wp, eeprom_info->gpio_pin_spi_wp, GPIO_PIN_SET);
	}
}

static inline void set_eeprom_addr(eeprom_addr_t *eeprom_addr, uint32_t addr)
{
	u_uint32_bytes_t u_uint32_bytes;
	u_uint32_bytes.v = addr;
	eeprom_addr->b0 = u_uint32_bytes.s.byte0;
	eeprom_addr->b1 = u_uint32_bytes.s.byte1;
	eeprom_addr->b2 = u_uint32_bytes.s.byte2;
}

uint8_t eeprom_id(eeprom_info_t *eeprom_info)
{
	eeprom_request_t request;
	uint8_t device_id = 0x00;

	if(eeprom_info == NULL) {
		return device_id;
	}

	request.fn.fn = 0xab;
	set_eeprom_addr(&request.addr, 0x00);

	eeprom_enable_cs(eeprom_info);

	spi_tx_data(eeprom_info->spi_info, (uint8_t *)&request, sizeof(request), 5);
	spi_rx_data(eeprom_info->spi_info, (uint8_t *)&device_id, 1, 5);

	eeprom_disable_cs(eeprom_info);

	return device_id;
}

static uint8_t eeprom_wip_status(eeprom_info_t *eeprom_info)
{
	uint8_t wip_info = 0x00;
	eeprom_fn_t fn;

	if(eeprom_info == NULL) {
		return 0x00;
	}

	fn.fn = 0x05;

	eeprom_enable_cs(eeprom_info);

	spi_tx_data(eeprom_info->spi_info, (uint8_t *)&fn, sizeof(fn), 5);
	spi_rx_data(eeprom_info->spi_info, (uint8_t *)&wip_info, 1, 5);

	eeprom_disable_cs(eeprom_info);

	return (wip_info & 0x01);

}

static uint8_t eeprom_read_byte(eeprom_info_t *eeprom_info, uint32_t start, uint8_t *pbyte)     //25LC1024
{

	uint8_t state = 0;

	eeprom_request_t request;

	if(eeprom_info == NULL) {
		return 0x00;
	}

	request.fn.fn = 0x03;
	set_eeprom_addr(&request.addr, start);

	eeprom_enable_cs(eeprom_info);

	spi_tx_data(eeprom_info->spi_info, (uint8_t *)&request, sizeof(request), 5);
	spi_rx_data(eeprom_info->spi_info, (uint8_t *)pbyte, 1, 5);

	eeprom_disable_cs(eeprom_info);

	return state;
}

uint8_t eeprom_read(eeprom_info_t *eeprom_info, uint32_t start, uint8_t *data, uint16_t size)
{
	uint8_t state = 0;
	uint16_t i;

	//debug("start:%d, size:%d", start, size);

	if(eeprom_info == NULL) {
		return state;
	}

	mutex_lock(eeprom_info->mutex);

	for(i = 0; i < size; i++) {
		eeprom_read_byte(eeprom_info, start + i, data + i);
	}

	mutex_unlock(eeprom_info->mutex);

	//_hexdump("read", data, size);

	return state;
}

static uint8_t eeprom_write_bytes_25lc1024(eeprom_info_t *eeprom_info, uint32_t start, uint8_t *data, uint16_t size)
{
	uint8_t state = 0;
	uint8_t i = 0;
	eeprom_request_t request;

	if(eeprom_info == NULL) {
		return state;
	}

	request.fn.fn = 0x06;

	eeprom_enable_cs(eeprom_info);

	spi_tx_data(eeprom_info->spi_info, (uint8_t *)&request.fn, sizeof(request.fn), 5);

	eeprom_disable_cs(eeprom_info);

	request.fn.fn = 0x02;
	set_eeprom_addr(&request.addr, start);

	eeprom_enable_cs(eeprom_info);
	spi_tx_data(eeprom_info->spi_info, (uint8_t *)&request, sizeof(request), 5);
	spi_tx_data(eeprom_info->spi_info, (uint8_t *)data, size, 50);
	eeprom_disable_cs(eeprom_info);

	while(eeprom_wip_status(eeprom_info)) {
		osDelay(1);

		i++;

		if(i > 10) {
			break;
		}
	}

	return state;
}

uint8_t eeprom_write(eeprom_info_t *eeprom_info, uint32_t start, uint8_t *data, uint16_t size)
{
	uint8_t state = 0;
	uint16_t left = size;
	uint32_t addr = start;

	//debug("start:%d, size:%d", start, size);

	if(eeprom_info == NULL) {
		return state;
	}

	mutex_lock(eeprom_info->mutex);

	while(left > 0) {
		uint16_t write_size = EEPROM_1024_PAGE - (addr % EEPROM_1024_PAGE);

		if(write_size > left) {
			write_size = left;
		}

		eeprom_write_bytes_25lc1024(eeprom_info, addr, data + (addr - start), write_size);

		addr += write_size;
		left -= write_size;
	}

	mutex_unlock(eeprom_info->mutex);

	//_hexdump("write", data, size);

	return state;
}

int detect_eeprom(eeprom_info_t *eeprom_info)
{
	int i;
	int ret = -1;
	uint8_t id;

	for(i = 0; i < 10; i++) {
		id = eeprom_id(eeprom_info);

		if(id == 0x29) {
			break;
		}

		osDelay(200);
	}

	if(id == 0x29) {
		ret = 0;
	}

	return ret;
}

