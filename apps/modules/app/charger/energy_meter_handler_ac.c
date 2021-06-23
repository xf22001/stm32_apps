

/*================================================================
 *   
 *   
 *   文件名称：energy_meter_handler_ac.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月05日 星期六 12时56分40秒
 *   修改日期：2021年06月23日 星期三 10时57分46秒
 *   描    述：
 *
 *================================================================*/
#include "energy_meter_handler_ac.h"

#include "uart_data_task.h"
#include "dlt_645_master_txrx.h"

typedef struct {
	uint8_t state;
} energy_meter_handler_ctx_t;

static void uart_data_request(void *fn_ctx, void *chain_ctx)
{
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)fn_ctx;
	energy_meter_handler_ctx_t *energy_meter_handler_ctx = (energy_meter_handler_ctx_t *)energy_meter_info->ctx;
	channel_info_t *channel_info = energy_meter_info->channel_info;
	int ret;

	switch(energy_meter_handler_ctx->state) {
		case 0: {
			dlt_645_addr_t dlt_645_addr;
			ret = dlt_645_master_get_energy_get_addr(energy_meter_info->dlt_645_master_info, &dlt_645_addr);

			if(ret == 0) {
				uint8_t *data;
				uint32_t addr;

				data = dlt_645_addr.data;
				addr = get_u64_from_bcd_b01234567(data[0], data[1], data[2], data[3], data[4], data[5], 0, 0);

				data = energy_meter_info->dlt_645_addr.data;
				get_bcd_b0123456789_from_u64(&data[0], &data[1], &data[2], &data[3], &data[4], &data[5], NULL, NULL, NULL, NULL, addr + channel_info->channel_id);
				energy_meter_handler_ctx->state = 1;
			}
		}
		break;

		default: {
			dlt_645_master_get_energy(energy_meter_info->dlt_645_master_info, &energy_meter_info->dlt_645_addr, &channel_info->total_energy);
			dlt_645_master_get_voltage(energy_meter_info->dlt_645_master_info, &energy_meter_info->dlt_645_addr, &channel_info->va, &channel_info->vb, &channel_info->vc);
			dlt_645_master_get_current(energy_meter_info->dlt_645_master_info, &energy_meter_info->dlt_645_addr, &channel_info->ca, &channel_info->cb, &channel_info->cc);
		}
		break;
	}
}

static int init_ac(void *_energy_meter_info)
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

	set_uart_data_task_request_delay(uart_data_task_info, 1000);
	energy_meter_info->uart_data_request_cb.fn = uart_data_request;
	energy_meter_info->uart_data_request_cb.fn_ctx = energy_meter_info;
	add_uart_data_task_info_cb(uart_data_task_info, &energy_meter_info->uart_data_request_cb);

	return ret;
}

energy_meter_handler_t energy_meter_handler_ac = {
	.energy_meter_type = CHANNEL_ENERGY_METER_TYPE_AC,
	.init = init_ac,
};
