

/*================================================================
 *   
 *   
 *   文件名称：energy_meter_handler_dc_native.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月11日 星期二 11时25分44秒
 *   修改日期：2021年05月31日 星期一 15时28分02秒
 *   描    述：
 *
 *================================================================*/
#include "energy_meter_handler_dc_native.h"

#include "uart_data_task.h"
#include "dlt_645_master_txrx.h"

typedef struct {
	uint8_t state;
	uint32_t stamps;
} energy_meter_handler_ctx_t;

static void uart_data_request(void *fn_ctx, void *chain_ctx)
{
	//energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)fn_ctx;
}

static int init(void *_energy_meter_info)
{
	int ret = 0;
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)_energy_meter_info;
	channel_info_t *channel_info = energy_meter_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;
	uart_data_task_info_t *uart_data_task_info;

	energy_meter_handler_ctx_t *energy_meter_handler_ctx = (energy_meter_handler_ctx_t *)os_calloc(1, sizeof(energy_meter_handler_ctx_t));
	OS_ASSERT(energy_meter_handler_ctx != NULL);

	energy_meter_info->ctx = energy_meter_handler_ctx;

	energy_meter_info->uart_info = get_or_alloc_uart_info(channel_config->energy_meter_config.huart_energy_meter);
	energy_meter_info->dlt_645_master_info = get_or_alloc_dlt_645_master_info(energy_meter_info->uart_info);
	OS_ASSERT(energy_meter_info->dlt_645_master_info != NULL);

	uart_data_task_info = get_or_alloc_uart_data_task_info(channel_config->energy_meter_config.huart_energy_meter);
	OS_ASSERT(uart_data_task_info != NULL);

	energy_meter_info->uart_data_request_cb.fn = uart_data_request;
	energy_meter_info->uart_data_request_cb.fn_ctx = energy_meter_info;
	add_uart_data_task_info_cb(uart_data_task_info, &energy_meter_info->uart_data_request_cb);

	return ret;
}

energy_meter_handler_t energy_meter_handler_dc_native = {
	.energy_meter_type = CHANNEL_ENERGY_METER_TYPE_DC_NATIVE,
	.init = init,
};
