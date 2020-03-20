

/*================================================================
 *   
 *   
 *   文件名称：modbus_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月26日 星期二 14时25分28秒
 *   修改日期：2020年03月20日 星期五 14时25分19秒
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

typedef struct {
	struct list_head list;
	uart_info_t *uart_info;
	uint8_t rx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t rx_size;
	uint8_t tx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t tx_size;
	uint16_t *modbus_data;
	uint16_t start_addr;
	uint16_t end_addr;
	uint32_t rx_timeout;
	uint32_t tx_timeout;
	callback_chain_t *data_changed_chain;
} modbus_info_t;

modbus_info_t *get_modbus_info(uart_info_t *uart_info);
void free_modbus_info(modbus_info_t *modbus_info);
modbus_info_t *alloc_modbus_info(uart_info_t *uart_info);
int modbus_process_request(modbus_info_t *modbus_info);
int set_modbus_data(modbus_info_t *modbus_info, uint16_t *modbus_data, uint16_t start_addr, uint16_t end_addr);
int add_modbus_data_changed_cb(modbus_info_t *modbus_info, callback_item_t *callback_item);
int remove_modbus_data_changed_cb(modbus_info_t *modbus_info, callback_item_t *callback_item);
#endif //_MODBUS_TXRX_H
