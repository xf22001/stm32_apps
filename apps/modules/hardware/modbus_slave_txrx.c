

/*================================================================
 *   
 *   
 *   文件名称：modbus_slave_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 14时54分12秒
 *   修改日期：2020年04月20日 星期一 15时21分29秒
 *   描    述：
 *
 *================================================================*/
#include "modbus_slave_txrx.h"
#include "modbus_spec.h"
#include "os_utils.h"
//#define UDP_LOG
#include "task_probe_tool.h"

static LIST_HEAD(modbus_slave_info_list);
static osMutexId modbus_slave_info_list_mutex = NULL;

static modbus_slave_info_t *get_modbus_slave_info(uart_info_t *uart_info)
{
	modbus_slave_info_t *modbus_slave_info = NULL;
	modbus_slave_info_t *modbus_slave_info_item = NULL;
	osStatus os_status;

	if(modbus_slave_info_list_mutex == NULL) {
		return modbus_slave_info;
	}

	os_status = osMutexWait(modbus_slave_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(modbus_slave_info_item, &modbus_slave_info_list, modbus_slave_info_t, list) {
		if(modbus_slave_info_item->uart_info == uart_info) {
			modbus_slave_info = modbus_slave_info_item;
			break;
		}
	}

	os_status = osMutexRelease(modbus_slave_info_list_mutex);

	if(os_status != osOK) {
	}

	return modbus_slave_info;
}

void free_modbus_slave_info(modbus_slave_info_t *modbus_slave_info)
{
	osStatus os_status;

	if(modbus_slave_info == NULL) {
		return;
	}

	if(modbus_slave_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(modbus_slave_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&modbus_slave_info->list);

	os_status = osMutexRelease(modbus_slave_info_list_mutex);

	if(os_status != osOK) {
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

modbus_slave_info_t *get_or_alloc_modbus_slave_info(uart_info_t *uart_info)
{
	modbus_slave_info_t *modbus_slave_info = NULL;
	osStatus os_status;

	modbus_slave_info = get_modbus_slave_info(uart_info);

	if(modbus_slave_info != NULL) {
		return modbus_slave_info;
	}

	if(modbus_slave_info_list_mutex == NULL) {
		osMutexDef(modbus_slave_info_list_mutex);
		modbus_slave_info_list_mutex = osMutexCreate(osMutex(modbus_slave_info_list_mutex));

		if(modbus_slave_info_list_mutex == NULL) {
			return modbus_slave_info;
		}
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
	modbus_slave_info->rx_timeout = 3000;
	modbus_slave_info->tx_timeout = 1000;
	modbus_slave_info->modbus_slave_data_info = NULL;

	os_status = osMutexWait(modbus_slave_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&modbus_slave_info->list, &modbus_slave_info_list);

	os_status = osMutexRelease(modbus_slave_info_list_mutex);

	if(os_status != osOK) {
	}

	return modbus_slave_info;

failed:

	if(modbus_slave_info != NULL) {
		os_free(modbus_slave_info);
		modbus_slave_info = NULL;
	}

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
	u_uint16_bytes_t u_uint16_bytes;
	uint16_t addr;
	uint16_t number;
	int i;

	if(modbus_slave_info->rx_size < sizeof(modbus_slave_request_0x03_t)) {
		return ret;
	}

	modbus_crc = &request_0x03->crc;
	u_uint16_bytes.s.byte0 = modbus_crc->crc_l;
	u_uint16_bytes.s.byte1 = modbus_crc->crc_h;
	crc = modbus_slave_info->modbus_slave_data_info->crc(modbus_slave_info->modbus_slave_data_info->ctx, (uint8_t *)request_0x03, (uint8_t *)modbus_crc - (uint8_t *)request_0x03);

	if(crc != u_uint16_bytes.v) {
		return ret;
	}

	u_uint16_bytes.s.byte0 = request_0x03->addr.addr_l;
	u_uint16_bytes.s.byte1 = request_0x03->addr.addr_h;
	addr = u_uint16_bytes.v;

	u_uint16_bytes.s.byte0 = request_0x03->number.number_l;
	u_uint16_bytes.s.byte1 = request_0x03->number.number_h;
	number = u_uint16_bytes.v;

	if(modbus_slave_info->modbus_slave_data_info->valid(modbus_slave_info->modbus_slave_data_info->ctx, addr, number) == 0) {
		return ret;
	}

	modbus_slave_response_0x03_head->head.station = MODBUS_SLAVE_ID;
	modbus_slave_response_0x03_head->head.fn = 0x03;

	data_item = (modbus_data_item_t *)(modbus_slave_response_0x03_head + 1);

	for(i = 0; i < number; i++) {
		u_uint16_bytes.v = modbus_slave_info->modbus_slave_data_info->get(modbus_slave_info->modbus_slave_data_info->ctx, addr + i);
		udp_log_printf("request read addr:%d, data:%d(%x)\n", addr + i, u_uint16_bytes.v, u_uint16_bytes.v);

		data_item->data_l = u_uint16_bytes.s.byte0;
		data_item->data_h = u_uint16_bytes.s.byte1;
		data_item++;
	}

	modbus_slave_response_0x03_head->bytes_size = (uint8_t *)data_item - (uint8_t *)(modbus_slave_response_0x03_head + 1);

	modbus_crc = (modbus_crc_t *)data_item;

	u_uint16_bytes.v = modbus_slave_info->modbus_slave_data_info->crc(modbus_slave_info->modbus_slave_data_info->ctx, (uint8_t *)modbus_slave_response_0x03_head, (uint8_t *)modbus_crc - (uint8_t *)modbus_slave_response_0x03_head);
	modbus_crc->crc_l = u_uint16_bytes.s.byte0;
	modbus_crc->crc_h = u_uint16_bytes.s.byte1;
	modbus_slave_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_slave_response_0x03_head;

	uart_tx_data(modbus_slave_info->uart_info, modbus_slave_info->tx_buffer, modbus_slave_info->tx_size, modbus_slave_info->tx_timeout);
	//udp_log_hexdump("modbus_slave_info->tx_buffer", (const char *)modbus_slave_info->tx_buffer, modbus_slave_info->tx_size);

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
	u_uint16_bytes_t u_uint16_bytes;
	uint16_t addr;
	int i;

	if(modbus_slave_info->rx_size < sizeof(modbus_slave_request_0x06_t)) {
		return ret;
	}

	modbus_crc = &request_0x06->crc;
	u_uint16_bytes.s.byte0 = modbus_crc->crc_l;
	u_uint16_bytes.s.byte1 = modbus_crc->crc_h;
	crc = modbus_slave_info->modbus_slave_data_info->crc(modbus_slave_info->modbus_slave_data_info->ctx, (uint8_t *)request_0x06, (uint8_t *)&request_0x06->crc - (uint8_t *)request_0x06);

	if(crc != u_uint16_bytes.v) {
		return ret;
	}

	u_uint16_bytes.s.byte0 = request_0x06->addr.addr_l;
	u_uint16_bytes.s.byte1 = request_0x06->addr.addr_h;
	addr = u_uint16_bytes.v;

	data_item = &request_0x06->data;

	if(modbus_slave_info->modbus_slave_data_info->valid(modbus_slave_info->modbus_slave_data_info->ctx, addr, 1) == 0) {
		return ret;
	}

	//wite one number
	for(i = 0; i < 1; i++) {
		u_uint16_bytes.s.byte0 = data_item->data_l;
		u_uint16_bytes.s.byte1 = data_item->data_h;
		udp_log_printf("request write addr:%d, data:%d(%x)\n", addr + i, u_uint16_bytes.v, u_uint16_bytes.v);
		modbus_slave_info->modbus_slave_data_info->set(modbus_slave_info->modbus_slave_data_info->ctx, addr + i, u_uint16_bytes.v);
		data_item++;
	}

	modbus_slave_response_0x06->head.station = MODBUS_SLAVE_ID;
	modbus_slave_response_0x06->head.fn = 0x06;

	u_uint16_bytes.v = addr;
	modbus_slave_response_0x06->addr.addr_l = u_uint16_bytes.s.byte0;
	modbus_slave_response_0x06->addr.addr_h = u_uint16_bytes.s.byte1;

	data_item = &modbus_slave_response_0x06->data;

	for(i = 0; i < 1; i++) {
		u_uint16_bytes.v = modbus_slave_info->modbus_slave_data_info->get(modbus_slave_info->modbus_slave_data_info->ctx, addr + i);
		data_item->data_l = u_uint16_bytes.s.byte0;
		data_item->data_h = u_uint16_bytes.s.byte1;
		data_item++;
	}

	modbus_crc = &modbus_slave_response_0x06->crc;

	u_uint16_bytes.v = modbus_slave_info->modbus_slave_data_info->crc(modbus_slave_info->modbus_slave_data_info->ctx, (uint8_t *)modbus_slave_response_0x06, (uint8_t *)modbus_crc - (uint8_t *)modbus_slave_response_0x06);
	modbus_crc->crc_l = u_uint16_bytes.s.byte0;
	modbus_crc->crc_h = u_uint16_bytes.s.byte1;
	modbus_slave_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_slave_response_0x06;

	uart_tx_data(modbus_slave_info->uart_info, modbus_slave_info->tx_buffer, modbus_slave_info->tx_size, modbus_slave_info->tx_timeout);
	//udp_log_hexdump("modbus_slave_info->tx_buffer", (const char *)modbus_slave_info->tx_buffer, modbus_slave_info->tx_size);

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
	u_uint16_bytes_t u_uint16_bytes;
	uint16_t addr;
	uint16_t number;
	int i;

	if(modbus_slave_info->rx_size < sizeof(modbus_slave_request_0x10_head_t)) {
		return ret;
	}

	u_uint16_bytes.s.byte0 = modbus_slave_request_0x10_head->addr.addr_l;
	u_uint16_bytes.s.byte1 = modbus_slave_request_0x10_head->addr.addr_h;
	addr = u_uint16_bytes.v;

	u_uint16_bytes.s.byte0 = modbus_slave_request_0x10_head->number.number_l;
	u_uint16_bytes.s.byte1 = modbus_slave_request_0x10_head->number.number_h;
	number = u_uint16_bytes.v;

	if(modbus_slave_info->modbus_slave_data_info->valid(modbus_slave_info->modbus_slave_data_info->ctx, addr, number) == 0) {
		return ret;
	}

	data_item = (modbus_data_item_t *)(modbus_slave_request_0x10_head + 1);

	modbus_crc = (modbus_crc_t *)(data_item + number);

	u_uint16_bytes.s.byte0 = modbus_crc->crc_l;
	u_uint16_bytes.s.byte1 = modbus_crc->crc_h;
	crc = modbus_slave_info->modbus_slave_data_info->crc(modbus_slave_info->modbus_slave_data_info->ctx, (uint8_t *)modbus_slave_request_0x10_head, (uint8_t *)modbus_crc - (uint8_t *)modbus_slave_request_0x10_head);

	if(crc != u_uint16_bytes.v) {
		return ret;
	}

	//wite more number
	for(i = 0; i < number; i++) {
		u_uint16_bytes.s.byte0 = data_item->data_l;
		u_uint16_bytes.s.byte1 = data_item->data_h;
		udp_log_printf("request multi-write addr:%d, data:%d(%x)\n", addr + i, u_uint16_bytes.v, u_uint16_bytes.v);
		modbus_slave_info->modbus_slave_data_info->set(modbus_slave_info->modbus_slave_data_info->ctx, addr + i, u_uint16_bytes.v);
		data_item++;
	}

	modbus_slave_response_0x10->head.station = MODBUS_SLAVE_ID;
	modbus_slave_response_0x10->head.fn = 0x10;

	u_uint16_bytes.v = addr;
	modbus_slave_response_0x10->addr.addr_l = u_uint16_bytes.s.byte0;
	modbus_slave_response_0x10->addr.addr_h = u_uint16_bytes.s.byte1;

	u_uint16_bytes.v = number;
	modbus_slave_response_0x10->number.number_l = u_uint16_bytes.s.byte0;
	modbus_slave_response_0x10->number.number_h = u_uint16_bytes.s.byte1;

	modbus_crc = &modbus_slave_response_0x10->crc;

	u_uint16_bytes.v = modbus_slave_info->modbus_slave_data_info->crc(modbus_slave_info->modbus_slave_data_info->ctx, (uint8_t *)modbus_slave_response_0x10, (uint8_t *)modbus_crc - (uint8_t *)modbus_slave_response_0x10);
	modbus_crc->crc_l = u_uint16_bytes.s.byte0;
	modbus_crc->crc_h = u_uint16_bytes.s.byte1;
	modbus_slave_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_slave_response_0x10;

	uart_tx_data(modbus_slave_info->uart_info, modbus_slave_info->tx_buffer, modbus_slave_info->tx_size, modbus_slave_info->tx_timeout);
	//udp_log_hexdump("modbus_slave_info->tx_buffer", (const char *)modbus_slave_info->tx_buffer, modbus_slave_info->tx_size);

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

	modbus_head_t *head = NULL;

	if(modbus_slave_info->rx_size < sizeof(modbus_head_t)) {
		return ret;
	}

	//udp_log_printf("\nmodbus_slave_info->rx_size:%d\n", modbus_slave_info->rx_size);
	//udp_log_hexdump("modbus_slave_info->rx_buffer", (const char *)modbus_slave_info->rx_buffer, modbus_slave_info->rx_size);

	head = (modbus_head_t *)modbus_slave_info->rx_buffer;

	//udp_log_printf("head->station:%02x\n", head->station);
	//udp_log_printf("head->fn:%02x\n", head->fn);

	if(modbus_slave_info->modbus_slave_data_info == NULL) {
		return ret;
	}

	for(i = 0; i < (sizeof(fn_table) / sizeof(modbus_slave_callback_t)); i++) {
		modbus_slave_callback_t *cb = &fn_table[i];

		if(cb->fn == head->fn) {
			ret = cb->callback(modbus_slave_info);
			break;
		}
	}

	return ret;
}

int modbus_slave_process_request(modbus_slave_info_t *modbus_slave_info)
{
	modbus_slave_read(modbus_slave_info);
	return modubs_decode_request(modbus_slave_info);
}

int set_modbus_slave_data_info(modbus_slave_info_t *modbus_slave_info, modbus_slave_data_info_t *modbus_slave_data_info)
{
	int ret = 0;

	modbus_slave_info->modbus_slave_data_info = modbus_slave_data_info;

	return ret;
}
