

/*================================================================
 *   
 *   
 *   文件名称：modbus_master_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 15时28分52秒
 *   修改日期：2020年04月20日 星期一 17时30分30秒
 *   描    述：
 *
 *================================================================*/
#include "modbus_master_txrx.h"
#include "os_utils.h"
//#define UDP_LOG
#include "task_probe_tool.h"

static LIST_HEAD(modbus_master_info_list);
static osMutexId modbus_master_info_list_mutex = NULL;

static modbus_master_info_t *get_modbus_master_info(uart_info_t *uart_info)
{
	modbus_master_info_t *modbus_master_info = NULL;
	modbus_master_info_t *modbus_master_info_item = NULL;
	osStatus os_status;

	if(modbus_master_info_list_mutex == NULL) {
		return modbus_master_info;
	}

	os_status = osMutexWait(modbus_master_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(modbus_master_info_item, &modbus_master_info_list, modbus_master_info_t, list) {
		if(modbus_master_info_item->uart_info == uart_info) {
			modbus_master_info = modbus_master_info_item;
			break;
		}
	}

	os_status = osMutexRelease(modbus_master_info_list_mutex);

	if(os_status != osOK) {
	}

	return modbus_master_info;
}

void free_modbus_master_info(modbus_master_info_t *modbus_master_info)
{
	osStatus os_status;

	if(modbus_master_info == NULL) {
		return;
	}

	if(modbus_master_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(modbus_master_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&modbus_master_info->list);

	os_status = osMutexRelease(modbus_master_info_list_mutex);

	if(os_status != osOK) {
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
	osStatus os_status;

	modbus_master_info = get_modbus_master_info(uart_info);

	if(modbus_master_info != NULL) {
		return modbus_master_info;
	}

	if(modbus_master_info_list_mutex == NULL) {
		osMutexDef(modbus_master_info_list_mutex);
		modbus_master_info_list_mutex = osMutexCreate(osMutex(modbus_master_info_list_mutex));

		if(modbus_master_info_list_mutex == NULL) {
			return modbus_master_info;
		}
	}

	modbus_master_info = (modbus_master_info_t *)os_alloc(sizeof(modbus_master_info_t));

	if(modbus_master_info == NULL) {
		return modbus_master_info;
	}

	modbus_master_info->uart_info = uart_info;
	modbus_master_info->rx_timeout = 3000;
	modbus_master_info->tx_timeout = 1000;
	modbus_master_info->modbus_master_data_info = NULL;

	os_status = osMutexWait(modbus_master_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&modbus_master_info->list, &modbus_master_info_list);

	os_status = osMutexRelease(modbus_master_info_list_mutex);

	if(os_status != osOK) {
	}

	return modbus_master_info;
}

static void modbus_master_read(modbus_master_info_t *modbus_master_info)
{
	modbus_master_info->rx_size = uart_rx_data(modbus_master_info->uart_info, modbus_master_info->rx_buffer, MODBUS_BUFFER_SIZE, modbus_master_info->rx_timeout);
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

int modbus_master_read_items(modbus_master_info_t *modbus_master_info, uint16_t addr, uint16_t *values, uint16_t number)//read some number
{
	int ret = -1;
	modbus_master_request_0x03_t *request_0x03 = (modbus_master_request_0x03_t *)modbus_master_info->tx_buffer;
	modbus_master_response_0x03_head_t *modbus_master_response_0x03_head = (modbus_master_response_0x03_head_t *)modbus_master_info->rx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	u_uint16_bytes_t u_uint16_bytes;
	int i;
	uint16_t rx_size;

	if(modbus_master_info->modbus_master_data_info == NULL) {
		return ret;
	}

	if(number == 0) {
		return ret;
	}

	request_0x03->head.station = MODBUS_SLAVE_ID;
	request_0x03->head.fn = 0x03;

	u_uint16_bytes.v = addr;
	request_0x03->addr.addr_l = u_uint16_bytes.s.byte0;
	request_0x03->addr.addr_h = u_uint16_bytes.s.byte1;

	u_uint16_bytes.v = number;
	request_0x03->number.number_l = u_uint16_bytes.s.byte0;
	request_0x03->number.number_h = u_uint16_bytes.s.byte1;

	modbus_crc = &request_0x03->crc;

	u_uint16_bytes.v = modbus_master_info->modbus_master_data_info->crc(modbus_master_info->modbus_master_data_info->ctx, (uint8_t *)request_0x03, (uint8_t *)modbus_crc - (uint8_t *)request_0x03);

	modbus_crc->crc_l = u_uint16_bytes.s.byte0;
	modbus_crc->crc_h = u_uint16_bytes.s.byte1;

	rx_size = sizeof(modbus_master_response_0x03_head_t) + number * sizeof(modbus_data_item_t) + sizeof(modbus_crc_t);

	if(rx_size <= MODBUS_BUFFER_SIZE) {
		return ret;
	}

	ret = uart_tx_rx_data(modbus_master_info->uart_info, (uint8_t *)request_0x03, sizeof(modbus_master_request_0x03_t), (uint8_t *)modbus_master_response_0x03_head, MODBUS_BUFFER_SIZE, modbus_master_info->rx_timeout);

	if(ret != rx_size) {
		return ret;
	}

	if(modbus_master_response_0x03_head->head.station != MODBUS_SLAVE_ID) {
		return ret;
	}

	if(modbus_master_response_0x03_head->head.fn != 0x03) {
		return ret;
	}

	if(modbus_master_response_0x03_head->bytes_size != number * sizeof(modbus_data_item_t)) {
		return ret;
	}

	u_uint16_bytes.v = modbus_master_info->modbus_master_data_info->crc(modbus_master_info->modbus_master_data_info->ctx, (uint8_t *)modbus_master_response_0x03_head, rx_size - sizeof(modbus_crc_t));

	modbus_crc = (modbus_crc_t *)((uint8_t *)modbus_master_response_0x03_head + number * sizeof(modbus_data_item_t));

	if(u_uint16_bytes.s.byte0 != modbus_crc->crc_l) {
		return ret;
	}

	if(u_uint16_bytes.s.byte1 != modbus_crc->crc_h) {
		return ret;
	}

	ret = 0;
	data_item = (modbus_data_item_t *)(modbus_master_response_0x03_head + 1);

	for(i = 0; i < number; i++) {
		u_uint16_bytes.s.byte0 = data_item[i].data_l;
		u_uint16_bytes.s.byte1 = data_item[i].data_h;

		values[i] = u_uint16_bytes.v;
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

int modbus_master_write_one_item(modbus_master_info_t *modbus_master_info, uint16_t addr, uint16_t value)//write one number
{
	int ret = -1;
	modbus_master_request_0x06_t *request_0x06 = (modbus_master_request_0x06_t *)modbus_master_info->rx_buffer;
	modbus_master_response_0x06_t *modbus_master_response_0x06 = (modbus_master_response_0x06_t *)modbus_master_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	u_uint16_bytes_t u_uint16_bytes;
	int i;
	uint16_t rx_size;

	if(modbus_master_info->modbus_master_data_info == NULL) {
		return ret;
	}

	request_0x06->head.station = MODBUS_SLAVE_ID;
	request_0x06->head.fn = 0x06;

	u_uint16_bytes.v = addr;
	request_0x06->addr.addr_l = u_uint16_bytes.s.byte0;
	request_0x06->addr.addr_h = u_uint16_bytes.s.byte1;

	u_uint16_bytes.v = value;
	request_0x06->data.data_l = u_uint16_bytes.s.byte0;
	request_0x06->data.data_h = u_uint16_bytes.s.byte1;

	modbus_crc = &request_0x06->crc;

	u_uint16_bytes.v = modbus_master_info->modbus_master_data_info->crc(modbus_master_info->modbus_master_data_info->ctx, (uint8_t *)request_0x06, (uint8_t *)modbus_crc - (uint8_t *)request_0x06);

	modbus_crc->crc_l = u_uint16_bytes.s.byte0;
	modbus_crc->crc_h = u_uint16_bytes.s.byte1;

	rx_size = sizeof(modbus_master_response_0x06_t);

	if(rx_size <= MODBUS_BUFFER_SIZE) {
		return ret;
	}

	ret = uart_tx_rx_data(modbus_master_info->uart_info, (uint8_t *)request_0x06, sizeof(modbus_master_request_0x06_t), (uint8_t *)modbus_master_response_0x06, MODBUS_BUFFER_SIZE, modbus_master_info->rx_timeout);

	if(ret != rx_size) {
		return ret;
	}

	if(modbus_master_response_0x06->head.station != MODBUS_SLAVE_ID) {
		return ret;
	}

	if(modbus_master_response_0x06->head.fn != 0x03) {
		return ret;
	}

	u_uint16_bytes.v = modbus_master_info->modbus_master_data_info->crc(modbus_master_info->modbus_master_data_info->ctx, (uint8_t *)modbus_master_response_0x06, rx_size - sizeof(modbus_crc_t));

	modbus_crc = (modbus_crc_t *)&modbus_master_response_0x06->crc;

	if(u_uint16_bytes.s.byte0 != modbus_crc->crc_l) {
		return ret;
	}

	if(u_uint16_bytes.s.byte1 != modbus_crc->crc_h) {
		return ret;
	}

	u_uint16_bytes.v = value;
	data_item = (modbus_data_item_t *)&modbus_master_response_0x06->data;

	if(u_uint16_bytes.s.byte0 != data_item->data_l) {
		return ret;
	}

	if(u_uint16_bytes.s.byte1 != data_item->data_h) {
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

int modbus_master_write_items(modbus_master_info_t *modbus_master_info, uint16_t addr, uint16_t *values, uint16_t number)//write more number
{
	int ret = -1;
	modbus_master_request_0x10_head_t *modbus_master_request_0x10_head = (modbus_master_request_0x10_head_t *)modbus_master_info->rx_buffer;
	modbus_master_response_0x10_t *modbus_master_response_0x10 = (modbus_master_response_0x10_t *)modbus_master_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	u_uint16_bytes_t u_uint16_bytes;
	int i;


	ret = 0;
	return ret;
}

int set_modbus_master_data_info(modbus_master_info_t *modbus_master_info, modbus_master_data_info_t *modbus_master_data_info)
{
	int ret = 0;

	modbus_master_info->modbus_master_data_info = modbus_master_data_info;

	return ret;
}
