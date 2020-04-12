

/*================================================================
 *   
 *   
 *   文件名称：modbus_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月26日 星期二 14时25分28秒
 *   修改日期：2020年04月12日 星期日 13时57分28秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MODBUS_TXRX_H
#define _MODBUS_TXRX_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "usart_txrx.h"
#include "callback_chain.h"

#define MODBUS_BUFFER_SIZE 192
#define MODBUS_SLAVE_ID  1

typedef uint8_t (*modbus_addr_valid_t)(void *ctx, uint16_t start, uint16_t number);
typedef uint16_t (*modbus_data_get_t)(void *ctx, uint16_t addr);
typedef void (*modbus_data_set_t)(void *ctx, uint16_t addr, uint16_t value);

typedef struct {
	void *ctx;
	modbus_addr_valid_t valid;
	modbus_data_get_t get;
	modbus_data_set_t set;
} modbus_data_info_t;

typedef struct {
	struct list_head list;
	uart_info_t *uart_info;
	uint8_t rx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t rx_size;
	uint8_t tx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t tx_size;
	uint32_t rx_timeout;
	uint32_t tx_timeout;
	modbus_data_info_t *modbus_data_info;
	callback_chain_t *data_changed_chain;
} modbus_info_t;

void free_modbus_info(modbus_info_t *modbus_info);
modbus_info_t *get_or_alloc_modbus_info(uart_info_t *uart_info);
int modbus_process_request(modbus_info_t *modbus_info);
int set_modbus_data_info(modbus_info_t *modbus_info, modbus_data_info_t *modbus_data_info);
int add_modbus_data_changed_cb(modbus_info_t *modbus_info, callback_item_t *callback_item);
int remove_modbus_data_changed_cb(modbus_info_t *modbus_info, callback_item_t *callback_item);
#endif //_MODBUS_TXRX_H
