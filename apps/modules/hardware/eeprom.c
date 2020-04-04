

/*================================================================
 *
 *
 *   文件名称：eeprom.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月14日 星期四 09时01分36秒
 *   修改日期：2020年04月04日 星期六 18时36分50秒
 *   描    述：
 *
 *================================================================*/
#include "eeprom.h"

#include "cmsis_os.h"

#include "os_utils.h"

extern SPI_HandleTypeDef hspi3;

static LIST_HEAD(eeprom_info_list);
static osMutexId eeprom_info_list_mutex = NULL;

typedef struct {
	SPI_HandleTypeDef *hspi;

	GPIO_TypeDef *cs_gpio;
	uint16_t cs_pin;
	GPIO_TypeDef *wp_gpio;
	uint16_t wp_pin;
} eeprom_info_config_t;

eeprom_info_config_t eeprom_info_config_sz[] = {
	{
		.hspi = &hspi3,
		.cs_gpio = spi3_cs_GPIO_Port,
		.cs_pin = spi3_cs_Pin,
		.wp_gpio = spi3_wp_GPIO_Port,
		.wp_pin = spi3_wp_Pin,
	},
};

static eeprom_info_config_t *get_eeprom_info_config(spi_info_t *spi_info)
{
	int i;
	eeprom_info_config_t *eeprom_info_config = NULL;
	eeprom_info_config_t *eeprom_info_config_item = NULL;

	for(i = 0; i < sizeof(eeprom_info_config_sz) / sizeof(eeprom_info_config_t); i++) {
		eeprom_info_config_item = eeprom_info_config_sz + i;

		if(spi_info->hspi == eeprom_info_config_item->hspi) {
			eeprom_info_config = eeprom_info_config_item;
			break;
		}
	}

	return eeprom_info_config;
}

static eeprom_info_t *get_eeprom_info(spi_info_t *spi_info)
{
	eeprom_info_t *eeprom_info = NULL;
	eeprom_info_t *eeprom_info_item = NULL;
	osStatus os_status;

	if(eeprom_info_list_mutex == NULL) {
		return eeprom_info;
	}

	os_status = osMutexWait(eeprom_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}


	list_for_each_entry(eeprom_info_item, &eeprom_info_list, eeprom_info_t, list) {
		if(eeprom_info_item->spi_info == spi_info) {
			eeprom_info = eeprom_info_item;
			break;
		}
	}

	os_status = osMutexRelease(eeprom_info_list_mutex);

	if(os_status != osOK) {
	}

	return eeprom_info;
}

void free_eeprom_info(eeprom_info_t *eeprom_info)
{
	osStatus os_status;

	if(eeprom_info == NULL) {
		return;
	}

	if(eeprom_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(eeprom_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&eeprom_info->list);

	os_status = osMutexRelease(eeprom_info_list_mutex);

	if(os_status != osOK) {
	}

	os_status = osMutexDelete(eeprom_info->mutex);

	if(os_status != osOK) {
	}

	os_free(eeprom_info);
}

eeprom_info_t *get_or_alloc_eeprom_info(spi_info_t *spi_info)
{
	eeprom_info_t *eeprom_info = NULL;
	eeprom_info_config_t *eeprom_info_config = get_eeprom_info_config(spi_info);
	osMutexDef(eeprom_mutex);
	osStatus os_status;

	eeprom_info = get_eeprom_info(spi_info);

	if(eeprom_info != NULL) {
		return eeprom_info;
	}

	if(eeprom_info_list_mutex == NULL) {
		osMutexDef(eeprom_info_list_mutex);
		eeprom_info_list_mutex = osMutexCreate(osMutex(eeprom_info_list_mutex));

		if(eeprom_info_list_mutex == NULL) {
			return eeprom_info;
		}
	}

	if(eeprom_info_config == NULL) {
		return eeprom_info;
	}

	eeprom_info = (eeprom_info_t *)os_alloc(sizeof(eeprom_info_t));

	if(eeprom_info == NULL) {
		return eeprom_info;
	}

	eeprom_info->spi_info = spi_info;

	eeprom_info->cs_gpio = eeprom_info_config->cs_gpio;
	eeprom_info->cs_pin = eeprom_info_config->cs_pin;
	eeprom_info->wp_gpio = eeprom_info_config->wp_gpio;
	eeprom_info->wp_pin = eeprom_info_config->wp_pin;

	eeprom_info->mutex = osMutexCreate(osMutex(eeprom_mutex));

	os_status = osMutexWait(eeprom_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&eeprom_info->list, &eeprom_info_list);

	os_status = osMutexRelease(eeprom_info_list_mutex);

	if(os_status != osOK) {
	}

	return eeprom_info;
}


static inline void eeprom_enable_cs(eeprom_info_t *eeprom_info)
{
	HAL_GPIO_WritePin(eeprom_info->cs_gpio, eeprom_info->cs_pin, GPIO_PIN_RESET);
}

static inline void eeprom_disable_cs(eeprom_info_t *eeprom_info)
{
	HAL_GPIO_WritePin(eeprom_info->cs_gpio, eeprom_info->cs_pin, GPIO_PIN_SET);
}

static inline void eeprom_enable_wp(eeprom_info_t *eeprom_info)
{
	HAL_GPIO_WritePin(eeprom_info->wp_gpio, eeprom_info->wp_pin, GPIO_PIN_RESET);
}

static inline void eeprom_disable_wp(eeprom_info_t *eeprom_info)
{
	HAL_GPIO_WritePin(eeprom_info->wp_gpio, eeprom_info->wp_pin, GPIO_PIN_SET);
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
	osStatus os_status;
	uint16_t i;

	if(eeprom_info == NULL) {
		return state;
	}

	os_status = osMutexWait(eeprom_info->mutex, osWaitForever);

	if(os_status != osOK) {
	}

	for(i = 0; i < size; i++) {
		eeprom_read_byte(eeprom_info, start + i, data + i);
	}

	os_status = osMutexRelease(eeprom_info->mutex);

	if(os_status != osOK) {
	}

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

	osStatus os_status;

	if(eeprom_info == NULL) {
		return state;
	}

	os_status = osMutexWait(eeprom_info->mutex, osWaitForever);

	if(os_status != osOK) {
	}

	while(left > 0) {
		uint16_t write_size = EEPROM_1024_PAGE - (addr % EEPROM_1024_PAGE);

		if(write_size > left) {
			write_size = left;
		}

		eeprom_write_bytes_25lc1024(eeprom_info, addr, data + (addr - start), write_size);

		addr += write_size;
		left -= write_size;
	}

	os_status = osMutexRelease(eeprom_info->mutex);

	if(os_status != osOK) {
	}

	return state;
}
