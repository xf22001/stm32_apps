

/*================================================================
 *   
 *   
 *   文件名称：dlt_645_master_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月21日 星期四 10时20分02秒
 *   修改日期：2020年12月30日 星期三 15时02分20秒
 *   描    述：
 *
 *================================================================*/
#ifndef _DLT_645_MASTER_TXRX_H
#define _DLT_645_MASTER_TXRX_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "usart_txrx.h"
#include "dlt_645_spec.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uart_info_t *uart_info;
	uint8_t rx_buffer[DLT_645_BUFFER_SIZE];
	uint8_t rx_size;
	uint8_t tx_buffer[DLT_645_BUFFER_SIZE];
	uint8_t tx_size;
	uint32_t rx_timeout;
	uint32_t tx_timeout;
} dlt_645_master_info_t;

dlt_645_master_info_t *get_or_alloc_dlt_645_master_info(uart_info_t *uart_info);
int dlt_645_master_get_energy(dlt_645_master_info_t *dlt_645_master_info, dlt_645_addr_t *addr, uint32_t *energy)/*100*/;
int dlt_645_master_get_voltage(dlt_645_master_info_t *dlt_645_master_info, dlt_645_addr_t *addr, uint16_t *va, uint16_t *vb, uint16_t *vc);
int dlt_645_master_get_current(dlt_645_master_info_t *dlt_645_master_info, dlt_645_addr_t *addr, uint16_t *ca, uint16_t *cb, uint16_t *cc);

#endif //_DLT_645_MASTER_TXRX_H
