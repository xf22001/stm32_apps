

/*================================================================
 *
 *
 *   文件名称：modbus_master_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 15时28分52秒
 *   修改日期：2020年05月14日 星期四 08时13分05秒
 *   描    述：
 *
 *================================================================*/
#include "modbus_master_txrx.h"
#include "os_utils.h"
//#define UDP_LOG
#include "task_probe_tool.h"

static LIST_HEAD(modbus_master_info_list);
static osMutexId modbus_master_info_list_mutex = NULL;

static modbus_master_info_t *get_modbus_master_info(UART_HandleTypeDef *huart)
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
		if(modbus_master_info_item->uart_info->huart == huart) {
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

modbus_master_info_t *get_or_alloc_modbus_master_info(UART_HandleTypeDef *huart)
{
	modbus_master_info_t *modbus_master_info = NULL;
	osStatus os_status;
	uart_info_t *uart_info;

	modbus_master_info = get_modbus_master_info(huart);

	if(modbus_master_info != NULL) {
		return modbus_master_info;
	}

	uart_info = get_or_alloc_uart_info(huart);

	if(uart_info == NULL) {
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
	modbus_master_info->rx_timeout = 100;
	modbus_master_info->tx_timeout = 100;

	os_status = osMutexWait(modbus_master_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&modbus_master_info->list, &modbus_master_info_list);

	os_status = osMutexRelease(modbus_master_info_list_mutex);

	if(os_status != osOK) {
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

int modbus_master_read_items(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t number, uint16_t *values)//read some number
{
	int ret = -1;
	int rx_size;
	modbus_master_request_0x03_t *request_0x03 = (modbus_master_request_0x03_t *)modbus_master_info->tx_buffer;
	modbus_master_response_0x03_head_t *modbus_master_response_0x03_head = (modbus_master_response_0x03_head_t *)modbus_master_info->rx_buffer;
	modbus_crc_t *modbus_crc = NULL;
	modbus_data_item_t *data_item = NULL;
	uint16_t crc;
	int i;

	if(number == 0) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
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
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	rx_size = uart_tx_rx_data(modbus_master_info->uart_info,
	                      (uint8_t *)request_0x03, modbus_master_info->tx_size,
	                      (uint8_t *)modbus_master_response_0x03_head, modbus_master_info->rx_size,
	                      modbus_master_info->rx_timeout);

	if(rx_size != modbus_master_info->rx_size) {
		udp_log_printf("rx_size:%d\n", rx_size);
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	//udp_log_hexdump("modbus_master_info->rx_buffer", (const char *)modbus_master_response_0x03_head, modbus_master_info->rx_size);

	if(modbus_master_response_0x03_head->head.station != station) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	if(modbus_master_response_0x03_head->head.fn != 0x03) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	if(modbus_master_response_0x03_head->bytes_size != number * sizeof(modbus_data_item_t)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	crc = modbus_calc_crc((uint8_t *)modbus_master_response_0x03_head,
	                      modbus_master_info->rx_size - sizeof(modbus_crc_t));

	data_item = (modbus_data_item_t *)(modbus_master_response_0x03_head + 1);

	modbus_crc = (modbus_crc_t *)(data_item + number);

	if(crc != get_modbus_crc(modbus_crc)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	ret = 0;

	for(i = 0; i < number; i++) {
		uint16_t value = get_modbus_data_item(data_item + i);
		udp_log_printf("read addr:%d, data:%d(%x)\n", addr + i, value, value);
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

int modbus_master_write_one_item(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t value)//write one number
{
	int ret = -1;
	int rx_size;
	modbus_master_request_0x06_t *request_0x06 = (modbus_master_request_0x06_t *)modbus_master_info->rx_buffer;
	modbus_master_response_0x06_t *modbus_master_response_0x06 = (modbus_master_response_0x06_t *)modbus_master_info->tx_buffer;
	modbus_crc_t *modbus_crc = NULL;
	uint16_t crc;

	request_0x06->head.station = station;
	request_0x06->head.fn = 0x06;
	set_modbus_addr(&request_0x06->addr, addr);
	set_modbus_data_item(&request_0x06->data, value);
	udp_log_printf("write addr:%d, data:%d(%x)\n", addr, value, value);
	modbus_crc = &request_0x06->crc;
	crc = modbus_calc_crc((uint8_t *)request_0x06,
	                      (uint8_t *)modbus_crc - (uint8_t *)request_0x06);
	set_modbus_crc(modbus_crc, crc);

	modbus_master_info->tx_size = sizeof(modbus_master_request_0x06_t);
	modbus_master_info->rx_size = sizeof(modbus_master_response_0x06_t);

	if(modbus_master_info->rx_size > MODBUS_BUFFER_SIZE) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	rx_size = uart_tx_rx_data(modbus_master_info->uart_info,
	                      (uint8_t *)request_0x06, modbus_master_info->tx_size,
	                      (uint8_t *)modbus_master_response_0x06, modbus_master_info->rx_size,
	                      modbus_master_info->rx_timeout);

	if(rx_size != modbus_master_info->rx_size) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	//udp_log_hexdump("modbus_master_info->rx_buffer", (const char *)request_0x06, modbus_master_info->tx_size);

	if(modbus_master_response_0x06->head.station != station) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	if(modbus_master_response_0x06->head.fn != 0x06) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	if(addr != get_modbus_addr(&modbus_master_response_0x06->addr)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	if(value != get_modbus_data_item(&modbus_master_response_0x06->data)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	crc = modbus_calc_crc((uint8_t *)modbus_master_response_0x06,
	                      modbus_master_info->rx_size - sizeof(modbus_crc_t));

	modbus_crc = (modbus_crc_t *)&modbus_master_response_0x06->crc;

	if(crc != get_modbus_crc(modbus_crc)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
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

int modbus_master_write_items(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t number, uint16_t *values)//write more number
{
	int ret = -1;
	int rx_size;
	modbus_master_request_0x10_head_t *modbus_master_request_0x10_head = (modbus_master_request_0x10_head_t *)modbus_master_info->rx_buffer;
	modbus_master_response_0x10_t *modbus_master_response_0x10 = (modbus_master_response_0x10_t *)modbus_master_info->tx_buffer;
	modbus_data_item_t *data_item = NULL;
	modbus_crc_t *modbus_crc = NULL;
	uint16_t crc;
	int i;

	if(number == 0) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
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
		udp_log_printf("multi-write addr:%d, data:%d(%x)\n", addr + i, value, value);
		set_modbus_data_item(data_item + i, value);
	}

	modbus_crc = (modbus_crc_t *)(data_item + number);
	crc = modbus_calc_crc((uint8_t *)modbus_master_request_0x10_head,
	                      (uint8_t *)modbus_crc - (uint8_t *)modbus_master_request_0x10_head);

	set_modbus_crc(modbus_crc, crc);

	modbus_master_info->tx_size = (uint8_t *)(modbus_crc + 1) - (uint8_t *)modbus_master_request_0x10_head;
	modbus_master_info->rx_size = sizeof(modbus_master_response_0x10_t);

	if(modbus_master_info->rx_size > MODBUS_BUFFER_SIZE) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	rx_size = uart_tx_rx_data(modbus_master_info->uart_info,
	                      (uint8_t *)modbus_master_request_0x10_head, modbus_master_info->tx_size,
	                      (uint8_t *)modbus_master_response_0x10, modbus_master_info->rx_size,
	                      modbus_master_info->rx_timeout);

	if(rx_size != modbus_master_info->rx_size) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	//udp_log_hexdump("modbus_master_info->rx_buffer", (const char *)modbus_master_response_0x10, modbus_master_info->rx_size);

	if(modbus_master_response_0x10->head.station != station) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	if(modbus_master_response_0x10->head.fn != 0x10) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	if(addr != get_modbus_addr(&modbus_master_response_0x10->addr)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	if(number != get_modbus_number(&modbus_master_response_0x10->number)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	crc = modbus_calc_crc((uint8_t *)modbus_master_response_0x10,
	                      modbus_master_info->rx_size - sizeof(modbus_crc_t));

	modbus_crc = &modbus_master_response_0x10->crc;

	if(crc != get_modbus_crc(modbus_crc)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	ret = 0;
	return ret;
}
