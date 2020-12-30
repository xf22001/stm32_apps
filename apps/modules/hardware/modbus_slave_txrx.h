

/*================================================================
 *   
 *   
 *   文件名称：modbus_slave_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 14时54分35秒
 *   修改日期：2020年12月30日 星期三 15时43分32秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MODBUS_SLAVE_TXRX_H
#define _MODBUS_SLAVE_TXRX_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "usart_txrx.h"
#include "modbus_spec.h"
#include "callback_chain.h"

#ifdef __cplusplus
}
#endif

#define MODBUS_BUFFER_SIZE 192
#define MODBUS_SLAVE_ID  1

typedef uint8_t (*modbus_slave_addr_valid_t)(void *ctx, uint16_t start, uint16_t number);
typedef uint16_t (*modbus_slave_data_get_t)(void *ctx, uint16_t addr);
typedef void (*modbus_slave_data_set_t)(void *ctx, uint16_t addr, uint16_t value);

typedef struct {
	void *ctx;
	modbus_slave_addr_valid_t valid;
	modbus_slave_data_get_t get;
	modbus_slave_data_set_t set;
} modbus_slave_data_info_t;

typedef struct {
	uart_info_t *uart_info;
	uint8_t rx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t rx_size;
	uint8_t tx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t tx_size;
	uint32_t rx_timeout;
	uint32_t tx_timeout;
	modbus_slave_data_info_t *modbus_slave_data_info;
	callback_chain_t *data_changed_chain;
} modbus_slave_info_t;

void free_modbus_slave_info(modbus_slave_info_t *modbus_slave_info);
modbus_slave_info_t *get_or_alloc_modbus_slave_info(uart_info_t *uart_info);
int add_modbus_slave_data_changed_cb(modbus_slave_info_t *modbus_slave_info, callback_item_t *callback_item);
int remove_modbus_slave_data_changed_cb(modbus_slave_info_t *modbus_slave_info, callback_item_t *callback_item);
int modbus_slave_process_request(modbus_slave_info_t *modbus_slave_info);
int set_modbus_slave_data_info(modbus_slave_info_t *modbus_slave_info, modbus_slave_data_info_t *modbus_slave_data_info);
#endif //_MODBUS_SLAVE_TXRX_H
