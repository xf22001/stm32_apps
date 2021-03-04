

/*================================================================
 *
 *
 *   文件名称：modbus_slave_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 14时54分12秒
 *   修改日期：2021年02月02日 星期二 13时13分00秒
 *   描    述：
 *
 *================================================================*/
#include "modbus_slave_txrx.h"
#include "modbus_spec.h"
#include "object_class.h"
#include "os_utils.h"

#define LOG_NONE
#include "log.h"

static object_class_t *modbus_slave_class = NULL;

static void free_modbus_slave_info(modbus_slave_info_t *modbus_slave_info)
{
	if(modbus_slave_info == NULL) {
		return;
	}

	if(modbus_slave_info->uart_info) {
		modbus_slave_info->uart_info = NULL;
		modbus_slave_info->rx_size = 0;
		modbus_slave_info->tx_size = 0;
	}

	free_callback_chain(modbus_slave_info->data_changed_chain);
	modbus_slave_info->data_changed_chain = NULL;

	os_free(modbus_slave_info);
}

static modbus_slave_info_t *alloc_modbus_slave_info(uart_info_t *uart_info)
{
	modbus_slave_info_t *modbus_slave_info = NULL;

	if(uart_info == NULL) {
		return modbus_slave_info;
	}

	modbus_slave_info = (modbus_slave_info_t *)os_alloc(sizeof(modbus_slave_info_t));

	if(modbus_slave_info == NULL) {
		return modbus_slave_info;
	}

	modbus_slave_info->data_changed_chain = alloc_callback_chain();

	if(modbus_slave_info->data_changed_chain == NULL) {
		goto failed;
	}

	modbus_slave_info->uart_info = uart_info;
	modbus_slave_info->rx_timeout = 1000;
	modbus_slave_info->tx_timeout = 1000;
	modbus_slave_info->modbus_slave_data_info = NULL;

	return modbus_slave_info;

failed:
	free_modbus_slave_info(modbus_slave_info);
	modbus_slave_info = NULL;

	return modbus_slave_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	modbus_slave_info_t *modbus_slave_info = (modbus_slave_info_t *)o;
	uart_info_t *uart_info = (uart_info_t *)ctx;

	if(modbus_slave_info->uart_info == uart_info) {
		ret = 0;
	}

	return ret;
}

modbus_slave_info_t *get_or_alloc_modbus_slave_info(uart_info_t *uart_info)
{
	modbus_slave_info_t *modbus_slave_info = NULL;

	os_enter_critical();

	if(modbus_slave_class == NULL) {
		modbus_slave_class = object_class_alloc();
	}

	os_leave_critical();

	modbus_slave_info = (modbus_slave_info_t *)object_class_get_or_alloc_object(modbus_slave_class, object_filter, uart_info, (object_alloc_t)alloc_modbus_slave_info, (object_free_t)free_modbus_slave_info);

	return modbus_slave_info;
}

int add_modbus_slave_data_changed_cb(modbus_slave_info_t *modbus_slave_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = register_callback(modbus_slave_info->data_changed_chain, callback_item);
	return ret;
}

int remove_modbus_slave_data_changed_cb(modbus_slave_info_t *modbus_slave_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = remove_callback(modbus_slave_info->data_changed_chain, callback_item);
	return ret;
}

static void modbus_slave_data_changed(modbus_slave_info_t *modbus_slave_info)
{
	do_callback_chain(modbus_slave_info->data_changed_chain, modbus_slave_info);
}

static void modbus_slave_read(modbus_slave_info_t *modbus_slave_info)
{
	modbus_slave_info->rx_size = uart_rx_data(modbus_slave_info->uart_info, modbus_slave_info->rx_buffer, MODBUS_BUFFER_SIZE, modbus_slave_info->rx_timeout);
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	modbus_crc_t crc;
} modbus_slave_request_0x03_t;

typedef struct {
	modbus_head_t head;
	uint8_t bytes_size;
} modbus_slave_response_0x03_head_t;

static int fn_0x03(modbus_slave_info_t *modbus_slave_info)//read some number
{
	int ret = -1;
	uint16_t crc;
	modbus_slave_request_0x03_t *request_0x03 = (modbus_slave_request_0x03_t *)modbus_slave_info->rx_buffer;
	modbus_slave_response_0x03_head_t *modbus_slave_response_0x03_head = (modbus_slave_response_0x03_head_t *)modbus_slave_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	uint16_t addr;
	uint16_t number;
	int i;

	if(modbus_slave_info->rx_size < sizeof(modbus_slave_request_0x03_t)) {
		debug("\n");
		return ret;
	}

	modbus_crc = &request_0x03->crc;
	crc = modbus_calc_crc((uint8_t *)request_0x03,
	                      (uint8_t *)modbus_crc - (uint8_t *)request_0x03);

	if(crc != get_modbus_crc(modbus_crc)) {
		debug("\n");
		return ret;
	}

	addr = get_modbus_addr(&request_0x03->addr);
	number = get_modbus_number(&request_0x03->number);

	if(modbus_slave_info->modbus_slave_data_info->valid(modbus_slave_info->modbus_slave_data_info->ctx, addr, number) == 0) {
		debug("\n");
		return ret;
	}

	modbus_slave_response_0x03_head->head.station = MODBUS_SLAVE_ID;
	modbus_slave_response_0x03_head->head.fn = 0x03;

	data_item = (modbus_data_item_t *)(modbus_slave_response_0x03_head + 1);

	for(i = 0; i < number; i++) {
		uint16_t value;
		value = modbus_slave_info->modbus_slave_data_info->get(modbus_slave_info->modbus_slave_data_info->ctx, addr + i);
		//debug("request read addr:%d, data:%d(%x)\n", addr + i, value, value);

		set_modbus_data_item(data_item + i, value);
	}

	modbus_slave_response_0x03_head->bytes_size = number * sizeof(modbus_data_item_t);

	modbus_crc = (modbus_crc_t *)(data_item + number);

	crc = modbus_calc_crc((uint8_t *)modbus_slave_response_0x03_head,
	                      (uint8_t *)modbus_crc - (uint8_t *)modbus_slave_response_0x03_head);
	set_modbus_crc(modbus_crc, crc);

	modbus_slave_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_slave_response_0x03_head;

	//_hexdump("modbus_slave_info->tx_buffer", (const char *)modbus_slave_info->tx_buffer, modbus_slave_info->tx_size);
	uart_tx_data(modbus_slave_info->uart_info, modbus_slave_info->tx_buffer, modbus_slave_info->tx_size, modbus_slave_info->tx_timeout);

	ret = 0;
	return ret;
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_data_item_t data;
	modbus_crc_t crc;
} modbus_slave_request_0x06_t;

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_data_item_t data;
	modbus_crc_t crc;
} modbus_slave_response_0x06_t;

static int fn_0x06(modbus_slave_info_t *modbus_slave_info)//write one number
{
	int ret = -1;
	uint16_t crc;
	modbus_slave_request_0x06_t *request_0x06 = (modbus_slave_request_0x06_t *)modbus_slave_info->rx_buffer;
	modbus_slave_response_0x06_t *modbus_slave_response_0x06 = (modbus_slave_response_0x06_t *)modbus_slave_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	uint16_t addr;
	int i;

	if(modbus_slave_info->rx_size < sizeof(modbus_slave_request_0x06_t)) {
		debug("\n");
		return ret;
	}

	modbus_crc = &request_0x06->crc;
	crc = modbus_calc_crc((uint8_t *)request_0x06,
	                      (uint8_t *)modbus_crc - (uint8_t *)request_0x06);

	if(crc != get_modbus_crc(modbus_crc)) {
		debug("\n");
		return ret;
	}

	addr = get_modbus_addr(&request_0x06->addr);

	data_item = &request_0x06->data;

	if(modbus_slave_info->modbus_slave_data_info->valid(modbus_slave_info->modbus_slave_data_info->ctx, addr, 1) == 0) {
		debug("\n");
		return ret;
	}

	//wite one number
	for(i = 0; i < 1; i++) {
		uint16_t value = get_modbus_data_item(data_item + i);
		//debug("request write addr:%d, data:%d(%x)\n", addr + i, value, value);
		modbus_slave_info->modbus_slave_data_info->set(modbus_slave_info->modbus_slave_data_info->ctx,
		        addr + i,
		        value);
	}

	modbus_slave_response_0x06->head.station = MODBUS_SLAVE_ID;
	modbus_slave_response_0x06->head.fn = 0x06;

	set_modbus_addr(&modbus_slave_response_0x06->addr, addr);

	data_item = &modbus_slave_response_0x06->data;

	for(i = 0; i < 1; i++) {
		uint16_t value = modbus_slave_info->modbus_slave_data_info->get(modbus_slave_info->modbus_slave_data_info->ctx, addr + i);
		set_modbus_data_item(data_item + i, value);
	}

	modbus_crc = &modbus_slave_response_0x06->crc;
	crc = modbus_calc_crc((uint8_t *)modbus_slave_response_0x06,
	                      (uint8_t *)modbus_crc - (uint8_t *)modbus_slave_response_0x06);

	set_modbus_crc(modbus_crc, crc);
	modbus_slave_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_slave_response_0x06;

	//_hexdump("modbus_slave_info->tx_buffer", (const char *)modbus_slave_info->tx_buffer, modbus_slave_info->tx_size);
	uart_tx_data(modbus_slave_info->uart_info, modbus_slave_info->tx_buffer, modbus_slave_info->tx_size, modbus_slave_info->tx_timeout);

	modbus_slave_data_changed(modbus_slave_info);

	ret = 0;
	return ret;
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	uint8_t bytes_size;
} modbus_slave_request_0x10_head_t;

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	modbus_crc_t crc;
} modbus_slave_response_0x10_t;

static int fn_0x10(modbus_slave_info_t *modbus_slave_info)//write more number
{
	int ret = -1;
	uint16_t crc;
	modbus_slave_request_0x10_head_t *modbus_slave_request_0x10_head = (modbus_slave_request_0x10_head_t *)modbus_slave_info->rx_buffer;
	modbus_slave_response_0x10_t *modbus_slave_response_0x10 = (modbus_slave_response_0x10_t *)modbus_slave_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	uint16_t addr;
	uint16_t number;
	int i;

	if(modbus_slave_info->rx_size < sizeof(modbus_slave_request_0x10_head_t)) {
		debug("\n");
		return ret;
	}

	addr = get_modbus_addr(&modbus_slave_request_0x10_head->addr);
	number = get_modbus_number(&modbus_slave_request_0x10_head->number);

	if(modbus_slave_info->modbus_slave_data_info->valid(modbus_slave_info->modbus_slave_data_info->ctx, addr, number) == 0) {
		debug("\n");
		return ret;
	}

	data_item = (modbus_data_item_t *)(modbus_slave_request_0x10_head + 1);

	modbus_crc = (modbus_crc_t *)(data_item + number);

	crc = modbus_calc_crc((uint8_t *)modbus_slave_request_0x10_head,
	                      (uint8_t *)modbus_crc - (uint8_t *)modbus_slave_request_0x10_head);

	if(crc != get_modbus_crc(modbus_crc)) {
		debug("\n");
		return ret;
	}

	//wite more number
	for(i = 0; i < number; i++) {
		uint16_t value = get_modbus_data_item(data_item + i);

		//debug("request multi-write addr:%d, data:%d(%x)\n", addr + i, value, value);
		modbus_slave_info->modbus_slave_data_info->set(modbus_slave_info->modbus_slave_data_info->ctx, addr + i, value);
	}

	modbus_slave_response_0x10->head.station = MODBUS_SLAVE_ID;
	modbus_slave_response_0x10->head.fn = 0x10;

	set_modbus_addr(&modbus_slave_response_0x10->addr, addr);
	set_modbus_number(&modbus_slave_response_0x10->number, number);

	modbus_crc = &modbus_slave_response_0x10->crc;

	crc = modbus_calc_crc((uint8_t *)modbus_slave_response_0x10,
	                      (uint8_t *)modbus_crc - (uint8_t *)modbus_slave_response_0x10);
	set_modbus_crc(modbus_crc, crc);
	modbus_slave_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_slave_response_0x10;

	//_hexdump("modbus_slave_info->tx_buffer", (const char *)modbus_slave_info->tx_buffer, modbus_slave_info->tx_size);
	uart_tx_data(modbus_slave_info->uart_info, modbus_slave_info->tx_buffer, modbus_slave_info->tx_size, modbus_slave_info->tx_timeout);

	modbus_slave_data_changed(modbus_slave_info);

	ret = 0;
	return ret;
}

typedef int (*modbus_slave_fn_callback_t)(modbus_slave_info_t *modbus_slave_info);

typedef struct {
	uint8_t fn;
	modbus_slave_fn_callback_t callback;
} modbus_slave_callback_t;

static modbus_slave_callback_t fn_table[] = {
	{
		.fn = 0x03,
		.callback = fn_0x03,
	},
	{
		.fn = 0x06,
		.callback = fn_0x06,
	},
	{
		.fn = 0x10,
		.callback = fn_0x10,
	},
};

static int modubs_decode_request(modbus_slave_info_t *modbus_slave_info)
{
	int ret = -1;
	int i;
	//uint32_t ticks = osKernelSysTick();

	modbus_head_t *head = NULL;

	if(modbus_slave_info->rx_size < sizeof(modbus_head_t)) {
		return ret;
	}

	//debug("\nmodbus_slave_info->rx_size:%d\n", modbus_slave_info->rx_size);
	//_hexdump("modbus_slave_info->rx_buffer", (const char *)modbus_slave_info->rx_buffer, modbus_slave_info->rx_size);

	head = (modbus_head_t *)modbus_slave_info->rx_buffer;

	//debug("head->station:%02x\n", head->station);
	//debug("head->fn:%02x\n", head->fn);

	if(modbus_slave_info->modbus_slave_data_info == NULL) {
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(fn_table); i++) {
		modbus_slave_callback_t *cb = &fn_table[i];

		if(cb->fn == head->fn) {
			ret = cb->callback(modbus_slave_info);
			break;
		}
	}

	//debug("modbus decode duration:%0d\n", osKernelSysTick() - ticks);

	return ret;
}

int modbus_slave_process_request(modbus_slave_info_t *modbus_slave_info)
{
	modbus_slave_read(modbus_slave_info);
	return modubs_decode_request(modbus_slave_info);
}

int set_modbus_slave_data_info(modbus_slave_info_t *modbus_slave_info, modbus_slave_data_info_t *modbus_slave_data_info)
{
	int ret = -1;

	if(modbus_slave_data_info == NULL) {
		return ret;
	}

	if(modbus_slave_data_info->valid == NULL) {
		return ret;
	}

	if(modbus_slave_data_info->get == NULL) {
		return ret;
	}

	if(modbus_slave_data_info->set == NULL) {
		return ret;
	}

	ret = 0;
	modbus_slave_info->modbus_slave_data_info = modbus_slave_data_info;

	return ret;
}
