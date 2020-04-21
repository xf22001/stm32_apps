

/*================================================================
 *   
 *   
 *   文件名称：modbus_master_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 15时28分59秒
 *   修改日期：2020年04月21日 星期二 10时14分11秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MODBUS_MASTER_TXRX_H
#define _MODBUS_MASTER_TXRX_H
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
#define MODBUS_SLAVE_ID  1

typedef uint16_t (*modbus_master_data_crc_t)(void *ctx, uint8_t *buffer, uint16_t size);

typedef struct {
	void *ctx;
	modbus_master_data_crc_t crc;
} modbus_master_data_info_t;

typedef struct {
	struct list_head list;
	uart_info_t *uart_info;
	uint8_t rx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t rx_size;
	uint8_t tx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t tx_size;
	uint32_t rx_timeout;
	uint32_t tx_timeout;
	modbus_master_data_info_t *modbus_master_data_info;
} modbus_master_info_t;

void free_modbus_master_info(modbus_master_info_t *modbus_master_info);
modbus_master_info_t *get_or_alloc_modbus_master_info(uart_info_t *uart_info);
int modbus_master_read_items(modbus_master_info_t *modbus_master_info, uint16_t addr, uint16_t number, uint16_t *values);
int modbus_master_write_one_item(modbus_master_info_t *modbus_master_info, uint16_t addr, uint16_t value);
int modbus_master_write_items(modbus_master_info_t *modbus_master_info, uint16_t addr, uint16_t number, uint16_t *values);
int set_modbus_master_data_info(modbus_master_info_t *modbus_master_info, modbus_master_data_info_t *modbus_master_data_info);
#endif //_MODBUS_MASTER_TXRX_H
