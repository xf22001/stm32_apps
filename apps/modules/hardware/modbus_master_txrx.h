

/*================================================================
 *   
 *   
 *   文件名称：modbus_master_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月20日 星期一 15时28分59秒
 *   修改日期：2020年12月30日 星期三 15时33分31秒
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

typedef struct {
	uart_info_t *uart_info;
	uint8_t rx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t rx_size;
	uint8_t tx_buffer[MODBUS_BUFFER_SIZE];
	uint8_t tx_size;
	uint32_t rx_timeout;
	uint32_t tx_timeout;
} modbus_master_info_t;

modbus_master_info_t *get_or_alloc_modbus_master_info(uart_info_t *uart_info);
int modbus_master_read_items(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t number, uint16_t *values)/*read some number*/;
int modbus_master_write_one_item(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t value)/*write one number*/;
int modbus_master_write_items(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t number, uint16_t *values)/*write more number*/;

static inline int modbus_master_read_items_retry(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t number, uint16_t *values, uint8_t retry)
{
	int ret = -1;

	while(retry > 0) {
		ret = modbus_master_read_items(modbus_master_info, station, addr, number, values);

		if(ret == 0) {
			break;
		}

		retry--;
	}

	return ret;
}

static inline int modbus_master_write_one_item_retry(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t value, uint8_t retry)
{
	int ret = -1;

	while(retry > 0) {
		ret = modbus_master_write_one_item(modbus_master_info, station, addr, value);

		if(ret == 0) {
			break;
		}

		retry--;
	}

	return ret;
}

static inline int modbus_master_write_items_retry(modbus_master_info_t *modbus_master_info, uint8_t station, uint16_t addr, uint16_t number, uint16_t *values, int retry)
{
	int ret = -1;

	while(retry > 0) {
		ret = modbus_master_write_items(modbus_master_info, station, addr, number, values);

		if(ret == 0) {
			break;
		}

		retry--;
	}

	return ret;
}
#endif //_MODBUS_MASTER_TXRX_H
