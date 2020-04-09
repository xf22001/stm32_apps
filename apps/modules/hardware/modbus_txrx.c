

/*================================================================
 *
 *
 *   文件名称：modbus_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月26日 星期二 14时24分54秒
 *   修改日期：2020年04月09日 星期四 17时26分07秒
 *   描    述：
 *
 *================================================================*/
#include "modbus_txrx.h"
#include "modbus_spec.h"
#include "os_utils.h"
//#define UDP_LOG
#include "task_probe_tool.h"

static LIST_HEAD(modbus_info_list);
static osMutexId modbus_info_list_mutex = NULL;

static modbus_info_t *get_modbus_info(uart_info_t *uart_info)
{
	modbus_info_t *modbus_info = NULL;
	modbus_info_t *modbus_info_item = NULL;
	osStatus os_status;

	if(modbus_info_list_mutex == NULL) {
		return modbus_info;
	}

	os_status = osMutexWait(modbus_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(modbus_info_item, &modbus_info_list, modbus_info_t, list) {
		if(modbus_info_item->uart_info == uart_info) {
			modbus_info = modbus_info_item;
			break;
		}
	}

	os_status = osMutexRelease(modbus_info_list_mutex);

	if(os_status != osOK) {
	}

	return modbus_info;
}

void free_modbus_info(modbus_info_t *modbus_info)
{
	osStatus os_status;

	if(modbus_info == NULL) {
		return;
	}

	if(modbus_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(modbus_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&modbus_info->list);

	os_status = osMutexRelease(modbus_info_list_mutex);

	if(os_status != osOK) {
	}

	if(modbus_info->uart_info) {
		modbus_info->uart_info = NULL;
		modbus_info->rx_size = 0;
		modbus_info->tx_size = 0;
	}

	free_callback_chain(modbus_info->data_changed_chain);
	modbus_info->data_changed_chain = NULL;

	os_free(modbus_info);
}

modbus_info_t *get_or_alloc_modbus_info(uart_info_t *uart_info)
{
	modbus_info_t *modbus_info = NULL;
	osStatus os_status;

	modbus_info = get_modbus_info(uart_info);

	if(modbus_info != NULL) {
		return modbus_info;
	}

	if(modbus_info_list_mutex == NULL) {
		osMutexDef(modbus_info_list_mutex);
		modbus_info_list_mutex = osMutexCreate(osMutex(modbus_info_list_mutex));

		if(modbus_info_list_mutex == NULL) {
			return modbus_info;
		}
	}

	modbus_info = (modbus_info_t *)os_alloc(sizeof(modbus_info_t));

	if(modbus_info == NULL) {
		return modbus_info;
	}

	modbus_info->data_changed_chain = alloc_callback_chain();

	if(modbus_info->data_changed_chain == NULL) {
		goto failed;
	}

	modbus_info->uart_info = uart_info;
	modbus_info->modbus_data = NULL;
	modbus_info->start_addr = 0;
	modbus_info->end_addr = 0;
	modbus_info->rx_timeout = 3000;
	modbus_info->tx_timeout = 1000;

	os_status = osMutexWait(modbus_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&modbus_info->list, &modbus_info_list);

	os_status = osMutexRelease(modbus_info_list_mutex);

	if(os_status != osOK) {
	}

	return modbus_info;

failed:

	if(modbus_info != NULL) {
		os_free(modbus_info);
		modbus_info = NULL;
	}

	return modbus_info;
}

int add_modbus_data_changed_cb(modbus_info_t *modbus_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = register_callback(modbus_info->data_changed_chain, callback_item);
	return ret;
}

int remove_modbus_data_changed_cb(modbus_info_t *modbus_info, callback_item_t *callback_item)
{
	int ret = -1;
	ret = remove_callback(modbus_info->data_changed_chain, callback_item);
	return ret;
}

static void modbus_data_changed(modbus_info_t *modbus_info)
{
	do_callback_chain(modbus_info->data_changed_chain, modbus_info);
}

static void modbus_read(modbus_info_t *modbus_info)
{
	modbus_info->rx_size = uart_rx_data(modbus_info->uart_info, modbus_info->rx_buffer, MODBUS_BUFFER_SIZE, modbus_info->rx_timeout);
}

static uint8_t is_valid_modbus_addr(modbus_info_t *modbus_info, uint16_t start, uint16_t number)
{
	uint8_t valid = 0;
	uint16_t start_addr = start * sizeof(uint16_t);
	uint16_t end_addr = start_addr + number * sizeof(uint16_t);

	if(end_addr <= start_addr) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	if(modbus_info->modbus_data == NULL) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	if(start_addr < modbus_info->start_addr) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	if(end_addr > modbus_info->end_addr) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	valid = 1;

	return valid;
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	modbus_crc_t crc;
} modbus_request_0x03_t;

typedef struct {
	modbus_head_t head;
	uint8_t bytes_size;
} modbus_response_0x03_head_t;

static int fn_0x03(modbus_info_t *modbus_info)//read some number
{
	int ret = -1;
	uint16_t crc;
	modbus_request_0x03_t *request_0x03 = (modbus_request_0x03_t *)modbus_info->rx_buffer;
	modbus_response_0x03_head_t *modbus_response_0x03_head = (modbus_response_0x03_head_t *)modbus_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	u_uint16_bytes_t u_uint16_bytes;
	uint16_t addr;
	uint16_t number;
	int i;

	if(modbus_info->rx_size < sizeof(modbus_request_0x03_t)) {
		return ret;
	}

	modbus_crc = &request_0x03->crc;
	u_uint16_bytes.s.byte0 = modbus_crc->crc_l;
	u_uint16_bytes.s.byte1 = modbus_crc->crc_h;
	crc = crc_check_for_dcph((uint8_t *)request_0x03, (uint8_t *)modbus_crc - (uint8_t *)request_0x03);

	if(crc != u_uint16_bytes.v) {
		return ret;
	}

	u_uint16_bytes.s.byte0 = request_0x03->addr.addr_l;
	u_uint16_bytes.s.byte1 = request_0x03->addr.addr_h;
	addr = u_uint16_bytes.v;

	u_uint16_bytes.s.byte0 = request_0x03->number.number_l;
	u_uint16_bytes.s.byte1 = request_0x03->number.number_h;
	number = u_uint16_bytes.v;

	if(is_valid_modbus_addr(modbus_info, addr, number) == 0) {
		return ret;
	}

	modbus_response_0x03_head->head.station = MODBUS_SLAVE_ID;
	modbus_response_0x03_head->head.fn = 0x03;

	data_item = (modbus_data_item_t *)(modbus_response_0x03_head + 1);

	for(i = 0; i < number; i++) {
		u_uint16_bytes.v = modbus_info->modbus_data[addr + i];
		udp_log_printf("request read addr:%d, data:%d(%x)\n", addr + i, u_uint16_bytes.v, u_uint16_bytes.v);

		data_item->data_l = u_uint16_bytes.s.byte0;
		data_item->data_h = u_uint16_bytes.s.byte1;
		data_item++;
	}

	modbus_response_0x03_head->bytes_size = (uint8_t *)data_item - (uint8_t *)(modbus_response_0x03_head + 1);

	modbus_crc = (modbus_crc_t *)data_item;

	u_uint16_bytes.v = crc_check_for_dcph((uint8_t *)modbus_response_0x03_head, (uint8_t *)modbus_crc - (uint8_t *)modbus_response_0x03_head);
	modbus_crc->crc_l = u_uint16_bytes.s.byte0;
	modbus_crc->crc_h = u_uint16_bytes.s.byte1;
	modbus_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_response_0x03_head;

	uart_tx_data(modbus_info->uart_info, modbus_info->tx_buffer, modbus_info->tx_size, modbus_info->tx_timeout);
	//udp_log_hexdump("modbus_info->tx_buffer", (const char *)modbus_info->tx_buffer, modbus_info->tx_size);

	ret = 0;
	return ret;
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_data_item_t data;
	modbus_crc_t crc;
} modbus_request_0x06_t;

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_data_item_t data;
	modbus_crc_t crc;
} modbus_response_0x06_t;

static int fn_0x06(modbus_info_t *modbus_info)//write one number
{
	int ret = -1;
	uint16_t crc;
	modbus_request_0x06_t *request_0x06 = (modbus_request_0x06_t *)modbus_info->rx_buffer;
	modbus_response_0x06_t *modbus_response_0x06 = (modbus_response_0x06_t *)modbus_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	u_uint16_bytes_t u_uint16_bytes;
	uint16_t addr;
	int i;

	if(modbus_info->rx_size < sizeof(modbus_request_0x06_t)) {
		return ret;
	}

	modbus_crc = &request_0x06->crc;
	u_uint16_bytes.s.byte0 = modbus_crc->crc_l;
	u_uint16_bytes.s.byte1 = modbus_crc->crc_h;
	crc = crc_check_for_dcph((uint8_t *)request_0x06, (uint8_t *)&request_0x06->crc - (uint8_t *)request_0x06);

	if(crc != u_uint16_bytes.v) {
		return ret;
	}

	u_uint16_bytes.s.byte0 = request_0x06->addr.addr_l;
	u_uint16_bytes.s.byte1 = request_0x06->addr.addr_h;
	addr = u_uint16_bytes.v;

	data_item = &request_0x06->data;

	if(is_valid_modbus_addr(modbus_info, addr, 1) == 0) {
		return ret;
	}

	//wite one number
	for(i = 0; i < 1; i++) {
		u_uint16_bytes.s.byte0 = data_item->data_l;
		u_uint16_bytes.s.byte1 = data_item->data_h;
		udp_log_printf("request write addr:%d, data:%d(%x)\n", addr + i, u_uint16_bytes.v, u_uint16_bytes.v);
		modbus_info->modbus_data[addr + i] = u_uint16_bytes.v;
		data_item++;
	}

	modbus_response_0x06->head.station = MODBUS_SLAVE_ID;
	modbus_response_0x06->head.fn = 0x06;

	u_uint16_bytes.v = addr;
	modbus_response_0x06->addr.addr_l = u_uint16_bytes.s.byte0;
	modbus_response_0x06->addr.addr_h = u_uint16_bytes.s.byte1;

	data_item = &modbus_response_0x06->data;

	for(i = 0; i < 1; i++) {
		u_uint16_bytes.v = modbus_info->modbus_data[addr + i];
		data_item->data_l = u_uint16_bytes.s.byte0;
		data_item->data_h = u_uint16_bytes.s.byte1;
		data_item++;
	}

	modbus_crc = &modbus_response_0x06->crc;

	u_uint16_bytes.v = crc_check_for_dcph((uint8_t *)modbus_response_0x06, (uint8_t *)modbus_crc - (uint8_t *)modbus_response_0x06);
	modbus_crc->crc_l = u_uint16_bytes.s.byte0;
	modbus_crc->crc_h = u_uint16_bytes.s.byte1;
	modbus_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_response_0x06;

	uart_tx_data(modbus_info->uart_info, modbus_info->tx_buffer, modbus_info->tx_size, modbus_info->tx_timeout);
	//udp_log_hexdump("modbus_info->tx_buffer", (const char *)modbus_info->tx_buffer, modbus_info->tx_size);

	modbus_data_changed(modbus_info);

	ret = 0;
	return ret;
}

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	uint8_t bytes_size;
} modbus_request_0x10_head_t;

typedef struct {
	modbus_head_t head;
	modbus_addr_t addr;
	modbus_number_t number;
	modbus_crc_t crc;
} modbus_response_0x10_t;

static int fn_0x10(modbus_info_t *modbus_info)//write more number
{
	int ret = -1;
	uint16_t crc;
	modbus_request_0x10_head_t *modbus_request_0x10_head = (modbus_request_0x10_head_t *)modbus_info->rx_buffer;
	modbus_response_0x10_t *modbus_response_0x10 = (modbus_response_0x10_t *)modbus_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	u_uint16_bytes_t u_uint16_bytes;
	uint16_t addr;
	uint16_t number;
	int i;

	if(modbus_info->rx_size < sizeof(modbus_request_0x10_head_t)) {
		return ret;
	}

	u_uint16_bytes.s.byte0 = modbus_request_0x10_head->addr.addr_l;
	u_uint16_bytes.s.byte1 = modbus_request_0x10_head->addr.addr_h;
	addr = u_uint16_bytes.v;

	u_uint16_bytes.s.byte0 = modbus_request_0x10_head->number.number_l;
	u_uint16_bytes.s.byte1 = modbus_request_0x10_head->number.number_h;
	number = u_uint16_bytes.v;

	if(is_valid_modbus_addr(modbus_info, addr, number) == 0) {
		return ret;
	}

	data_item = (modbus_data_item_t *)(modbus_request_0x10_head + 1);

	modbus_crc = (modbus_crc_t *)(data_item + number);

	u_uint16_bytes.s.byte0 = modbus_crc->crc_l;
	u_uint16_bytes.s.byte1 = modbus_crc->crc_h;
	crc = crc_check_for_dcph((uint8_t *)modbus_request_0x10_head, (uint8_t *)modbus_crc - (uint8_t *)modbus_request_0x10_head);

	if(crc != u_uint16_bytes.v) {
		return ret;
	}

	//wite more number
	for(i = 0; i < number; i++) {
		u_uint16_bytes.s.byte0 = data_item->data_l;
		u_uint16_bytes.s.byte1 = data_item->data_h;
		udp_log_printf("request multi-write addr:%d, data:%d(%x)\n", addr + i, u_uint16_bytes.v, u_uint16_bytes.v);
		modbus_info->modbus_data[addr + i] = u_uint16_bytes.v;
		data_item++;
	}

	modbus_response_0x10->head.station = MODBUS_SLAVE_ID;
	modbus_response_0x10->head.fn = 0x10;

	u_uint16_bytes.v = addr;
	modbus_response_0x10->addr.addr_l = u_uint16_bytes.s.byte0;
	modbus_response_0x10->addr.addr_h = u_uint16_bytes.s.byte1;

	u_uint16_bytes.v = number;
	modbus_response_0x10->number.number_l = u_uint16_bytes.s.byte0;
	modbus_response_0x10->number.number_h = u_uint16_bytes.s.byte1;

	modbus_crc = &modbus_response_0x10->crc;

	u_uint16_bytes.v = crc_check_for_dcph((uint8_t *)modbus_response_0x10, (uint8_t *)modbus_crc - (uint8_t *)modbus_response_0x10);
	modbus_crc->crc_l = u_uint16_bytes.s.byte0;
	modbus_crc->crc_h = u_uint16_bytes.s.byte1;
	modbus_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_response_0x10;

	uart_tx_data(modbus_info->uart_info, modbus_info->tx_buffer, modbus_info->tx_size, modbus_info->tx_timeout);
	//udp_log_hexdump("modbus_info->tx_buffer", (const char *)modbus_info->tx_buffer, modbus_info->tx_size);

	modbus_data_changed(modbus_info);

	ret = 0;
	return ret;
}

typedef int (*modbus_fn_callback_t)(modbus_info_t *modbus_info);

typedef struct {
	uint8_t fn;
	modbus_fn_callback_t callback;
} modbus_callback_t;

static modbus_callback_t fn_table[] = {
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

static int modubs_decode_request(modbus_info_t *modbus_info)
{
	int ret = -1;
	int i;

	modbus_head_t *head = NULL;

	if(modbus_info->rx_size < sizeof(modbus_head_t)) {
		return ret;
	}

	//udp_log_printf("\nmodbus_info->rx_size:%d\n", modbus_info->rx_size);
	//udp_log_hexdump("modbus_info->rx_buffer", (const char *)modbus_info->rx_buffer, modbus_info->rx_size);

	head = (modbus_head_t *)modbus_info->rx_buffer;

	//udp_log_printf("head->station:%02x\n", head->station);
	//udp_log_printf("head->fn:%02x\n", head->fn);

	if(modbus_info->modbus_data == NULL) {
		return ret;
	}

	for(i = 0; i < (sizeof(fn_table) / sizeof(modbus_callback_t)); i++) {
		modbus_callback_t *cb = &fn_table[i];

		if(cb->fn == head->fn) {
			ret = cb->callback(modbus_info);
			break;
		}
	}

	return ret;
}

int modbus_process_request(modbus_info_t *modbus_info)
{
	modbus_read(modbus_info);
	return modubs_decode_request(modbus_info);
}

int set_modbus_data(modbus_info_t *modbus_info, uint16_t *modbus_data, uint16_t start_addr, uint16_t end_addr)
{
	int ret = 0;
	modbus_info->modbus_data = modbus_data;
	modbus_info->start_addr = start_addr;
	modbus_info->end_addr = end_addr;

	return ret;
}
