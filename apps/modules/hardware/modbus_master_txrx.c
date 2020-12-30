

/*================================================================
 *
 *
 *   文件名称：modbus_master_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 15时28分52秒
 *   修改日期：2020年12月30日 星期三 15时34分22秒
 *   描    述：
 *
 *================================================================*/
#include "modbus_master_txrx.h"
#include "os_utils.h"
#include "map_utils.h"

#define LOG_NONE
#include "log.h"

static map_utils_t *modbus_master_map = NULL;

static modbus_master_info_t *get_modbus_master_info(uart_info_t *uart_info)
{
	modbus_master_info_t *modbus_master_info = NULL;

	modbus_master_info = (modbus_master_info_t *)map_utils_get_value(modbus_master_map, uart_info);

	return modbus_master_info;
}

static void free_modbus_master_info(modbus_master_info_t *modbus_master_info)
{
	int ret;

	if(modbus_master_info == NULL) {
		return;
	}

	ret = map_utils_remove_value(modbus_master_map, modbus_master_info->uart_info);

	if(ret != 0) {
	}

	if(modbus_master_info->uart_info) {
		modbus_master_info->uart_info = NULL;
		modbus_master_info->rx_size = 0;
		modbus_master_info->tx_size = 0;
	}

	os_free(modbus_master_info);
}

modbus_master_info_t *get_or_alloc_modbus_master_info(uart_info_t *uart_info)
{
	modbus_master_info_t *modbus_master_info = NULL;
	int ret;

	if(modbus_master_map == NULL) {
		modbus_master_map = map_utils_alloc(NULL);
	}

	if(uart_info == NULL) {
		return modbus_master_info;
	}

	modbus_master_info = get_modbus_master_info(uart_info);

	if(modbus_master_info != NULL) {
		return modbus_master_info;
	}

	modbus_master_info = (modbus_master_info_t *)os_alloc(sizeof(modbus_master_info_t));

	if(modbus_master_info == NULL) {
		return modbus_master_info;
	}

	modbus_master_info->uart_info = uart_info;
	modbus_master_info->rx_timeout = 100;
	modbus_master_info->tx_timeout = 100;

	ret = map_utils_add_key_value(modbus_master_map, uart_info, modbus_master_info);

	if(ret != 0) {
		free_modbus_master_info(modbus_master_info);
		modbus_master_info = NULL;
	}

	return modbus_master_info;
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	modbus_crc_t crc;
} modbus_master_request_0x03_t;

typedef struct {
	modbus_head_t head;
	uint8_t bytes_size;
} modbus_master_response_0x03_head_t;

int modbus_master_read_items(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t number, uint16_t *values)/*read some number*/
{
	int ret = -1;
	int rx_size;
	modbus_master_request_0x03_t *request_0x03 = (modbus_master_request_0x03_t *)modbus_master_info->tx_buffer;
	modbus_master_response_0x03_head_t *modbus_master_response_0x03_head = (modbus_master_response_0x03_head_t *)modbus_master_info->rx_buffer;
	modbus_crc_t *modbus_crc = NULL;
	modbus_data_item_t *data_item = NULL;
	uint16_t crc;
	int i;

	debug("station:%d, addr:%d, number:%d\n", station, addr, number);

	if(number == 0) {
		debug("\n");
		return ret;
	}

	request_0x03->head.station = station;
	request_0x03->head.fn = 0x03;
	set_modbus_addr(&request_0x03->addr, addr);
	set_modbus_number(&request_0x03->number, number);
	modbus_crc = &request_0x03->crc;
	crc = modbus_calc_crc((uint8_t *)request_0x03,
	                      (uint8_t *)modbus_crc - (uint8_t *)request_0x03);
	set_modbus_crc(modbus_crc, crc);

	modbus_master_info->tx_size = sizeof(modbus_master_request_0x03_t);
	modbus_master_info->rx_size = sizeof(modbus_master_response_0x03_head_t) + number * sizeof(modbus_data_item_t) + sizeof(modbus_crc_t);

	if(modbus_master_info->rx_size > MODBUS_BUFFER_SIZE) {
		debug("\n");
		return ret;
	}

	rx_size = uart_tx_rx_data(modbus_master_info->uart_info,
	                          (uint8_t *)request_0x03, modbus_master_info->tx_size,
	                          (uint8_t *)modbus_master_response_0x03_head, modbus_master_info->rx_size,
	                          modbus_master_info->rx_timeout);

	if(rx_size != modbus_master_info->rx_size) {
		debug("\n");
		return ret;
	}

	//_hexdump("modbus_master_info->rx_buffer", (const char *)modbus_master_response_0x03_head, modbus_master_info->rx_size);

	if(modbus_master_response_0x03_head->head.station != station) {
		debug("\n");
		return ret;
	}

	if(modbus_master_response_0x03_head->head.fn != 0x03) {
		debug("\n");
		return ret;
	}

	if(modbus_master_response_0x03_head->bytes_size != number * sizeof(modbus_data_item_t)) {
		debug("\n");
		return ret;
	}

	crc = modbus_calc_crc((uint8_t *)modbus_master_response_0x03_head,
	                      modbus_master_info->rx_size - sizeof(modbus_crc_t));

	data_item = (modbus_data_item_t *)(modbus_master_response_0x03_head + 1);

	modbus_crc = (modbus_crc_t *)(data_item + number);

	if(crc != get_modbus_crc(modbus_crc)) {
		debug("\n");
		return ret;
	}

	ret = 0;

	for(i = 0; i < number; i++) {
		uint16_t value = get_modbus_data_item(data_item + i);
		debug("read addr:%d, data:%d(%x)\n", addr + i, value, value);
		values[i] = value;
	}

	return ret;
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_data_item_t data;
	modbus_crc_t crc;
} modbus_master_request_0x06_t;

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_data_item_t data;
	modbus_crc_t crc;
} modbus_master_response_0x06_t;

int modbus_master_write_one_item(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t value)/*write one number*/
{
	int ret = -1;
	int rx_size;
	modbus_master_request_0x06_t *request_0x06 = (modbus_master_request_0x06_t *)modbus_master_info->tx_buffer;
	modbus_master_response_0x06_t *modbus_master_response_0x06 = (modbus_master_response_0x06_t *)modbus_master_info->rx_buffer;
	modbus_crc_t *modbus_crc = NULL;
	uint16_t crc;

	debug("station:%d, addr:%d, value:%d\n", station, addr, value);

	request_0x06->head.station = station;
	request_0x06->head.fn = 0x06;
	set_modbus_addr(&request_0x06->addr, addr);
	set_modbus_data_item(&request_0x06->data, value);
	debug("write addr:%d, data:%d(%x)\n", addr, value, value);
	modbus_crc = &request_0x06->crc;
	crc = modbus_calc_crc((uint8_t *)request_0x06,
	                      (uint8_t *)modbus_crc - (uint8_t *)request_0x06);
	set_modbus_crc(modbus_crc, crc);

	modbus_master_info->tx_size = sizeof(modbus_master_request_0x06_t);
	modbus_master_info->rx_size = sizeof(modbus_master_response_0x06_t);

	if(modbus_master_info->rx_size > MODBUS_BUFFER_SIZE) {
		debug("\n");
		return ret;
	}

	rx_size = uart_tx_rx_data(modbus_master_info->uart_info,
	                          (uint8_t *)request_0x06, modbus_master_info->tx_size,
	                          (uint8_t *)modbus_master_response_0x06, modbus_master_info->rx_size,
	                          modbus_master_info->rx_timeout);

	if(rx_size != modbus_master_info->rx_size) {
		debug("\n");
		return ret;
	}

	//_hexdump("modbus_master_info->tx_buffer", (const char *)request_0x06, modbus_master_info->tx_size);

	if(modbus_master_response_0x06->head.station != station) {
		debug("\n");
		return ret;
	}

	if(modbus_master_response_0x06->head.fn != 0x06) {
		debug("\n");
		return ret;
	}

	if(addr != get_modbus_addr(&modbus_master_response_0x06->addr)) {
		debug("\n");
		return ret;
	}

	if(value != get_modbus_data_item(&modbus_master_response_0x06->data)) {
		debug("\n");
		return ret;
	}

	crc = modbus_calc_crc((uint8_t *)modbus_master_response_0x06,
	                      modbus_master_info->rx_size - sizeof(modbus_crc_t));

	modbus_crc = (modbus_crc_t *)&modbus_master_response_0x06->crc;

	if(crc != get_modbus_crc(modbus_crc)) {
		debug("\n");
		return ret;
	}

	ret = 0;
	return ret;
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	uint8_t bytes_size;
} modbus_master_request_0x10_head_t;

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	modbus_crc_t crc;
} modbus_master_response_0x10_t;

int modbus_master_write_items(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t number, uint16_t *values)/*write more number*/
{
	int ret = -1;
	int rx_size;
	modbus_master_request_0x10_head_t *modbus_master_request_0x10_head = (modbus_master_request_0x10_head_t *)modbus_master_info->tx_buffer;
	modbus_master_response_0x10_t *modbus_master_response_0x10 = (modbus_master_response_0x10_t *)modbus_master_info->rx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	uint16_t crc;
	int i;

	debug("station:%d, addr:%d, number:%d\n", station, addr, number);

	if(number == 0) {
		debug("\n");
		return ret;
	}

	modbus_master_request_0x10_head->head.station = station;
	modbus_master_request_0x10_head->head.fn = 0x10;
	set_modbus_addr(&modbus_master_request_0x10_head->addr, addr);
	set_modbus_number(&modbus_master_request_0x10_head->number, number);
	modbus_master_request_0x10_head->bytes_size = number * sizeof(modbus_data_item_t);
	data_item = (modbus_data_item_t *)(modbus_master_request_0x10_head + 1);

	for(i = 0; i < number; i++) {
		uint16_t value = values[i];
		debug("multi-write addr:%d, data:%d(%x)\n", addr + i, value, value);
		set_modbus_data_item(data_item + i, value);
	}

	modbus_crc = (modbus_crc_t *)(data_item + number);
	crc = modbus_calc_crc((uint8_t *)modbus_master_request_0x10_head,
	                      (uint8_t *)modbus_crc - (uint8_t *)modbus_master_request_0x10_head);

	set_modbus_crc(modbus_crc, crc);

	modbus_master_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_master_request_0x10_head;
	modbus_master_info->rx_size = sizeof(modbus_master_response_0x10_t);

	if(modbus_master_info->rx_size > MODBUS_BUFFER_SIZE) {
		debug("\n");
		return ret;
	}

	rx_size = uart_tx_rx_data(modbus_master_info->uart_info,
	                          (uint8_t *)modbus_master_request_0x10_head, modbus_master_info->tx_size,
	                          (uint8_t *)modbus_master_response_0x10, modbus_master_info->rx_size,
	                          modbus_master_info->rx_timeout);

	if(rx_size != modbus_master_info->rx_size) {
		debug("\n");
		return ret;
	}

	//_hexdump("modbus_master_info->rx_buffer", (const char *)modbus_master_response_0x10, modbus_master_info->rx_size);

	if(modbus_master_response_0x10->head.station != station) {
		debug("\n");
		return ret;
	}

	if(modbus_master_response_0x10->head.fn != 0x10) {
		debug("\n");
		return ret;
	}

	if(addr != get_modbus_addr(&modbus_master_response_0x10->addr)) {
		debug("\n");
		return ret;
	}

	if(number != get_modbus_number(&modbus_master_response_0x10->number)) {
		debug("\n");
		return ret;
	}

	crc = modbus_calc_crc((uint8_t *)modbus_master_response_0x10,
	                      modbus_master_info->rx_size - sizeof(modbus_crc_t));

	modbus_crc = &modbus_master_response_0x10->crc;

	if(crc != get_modbus_crc(modbus_crc)) {
		debug("\n");
		return ret;
	}

	ret = 0;
	return ret;
}
