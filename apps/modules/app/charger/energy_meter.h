

/*================================================================
 *   
 *   
 *   文件名称：energy_meter.h
 *   创 建 者：肖飞
 *   创建日期：2021年04月07日 星期三 15时56分25秒
 *   修改日期：2021年04月13日 星期二 14时51分03秒
 *   描    述：
 *
 *================================================================*/
#ifndef _ENERGY_METER_H
#define _ENERGY_METER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "usart_txrx.h"
#include "callback_chain.h"
#include "channels_config.h"
#include "channels.h"
#include "dlt_645_master_txrx.h"

#ifdef __cplusplus
}
#endif

typedef int (*energy_meter_handler_init_t)(void *_energy_meter_info);

typedef struct {
	channel_energy_meter_type_t energy_meter_type;
	energy_meter_handler_init_t init;
} energy_meter_handler_t;

typedef struct {
	channel_info_t *channel_info;
	energy_meter_handler_t *energy_meter_handler;

	uart_info_t *uart_info;
	dlt_645_master_info_t *dlt_645_master_info;
	dlt_645_addr_t dlt_645_addr;
	callback_item_t uart_data_request_cb;

	uint32_t energy;
	uint32_t voltage;
	uint32_t current;

	uint32_t periodic_stamps;
} energy_meter_info_t;

energy_meter_info_t *alloc_energy_meter_info(channel_info_t *channel_info);

#endif //_ENERGY_METER_H
