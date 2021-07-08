

/*================================================================
 *   
 *   
 *   文件名称：energy_meter.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月07日 星期三 15时56分19秒
 *   修改日期：2021年07月08日 星期四 11时37分16秒
 *   描    述：
 *
 *================================================================*/
#include "energy_meter.h"
#include "channels.h"
#include "uart_data_task.h"
#include "energy_meter_handler_ac.h"
#include "energy_meter_handler_ac_hlw8032.h"
#include "energy_meter_handler_dc.h"

#include "log.h"

energy_meter_handler_t *energy_meter_handler_sz[] = {
	&energy_meter_handler_ac,
	&energy_meter_handler_ac_hlw8032,
	&energy_meter_handler_dc,
};

static energy_meter_handler_t *get_energy_meter_handler(channel_energy_meter_type_t energy_meter_type)
{
	int i;
	energy_meter_handler_t *energy_meter_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(energy_meter_handler_sz); i++) {
		energy_meter_handler_t *energy_meter_handler_item = energy_meter_handler_sz[i];

		if(energy_meter_handler_item->energy_meter_type == energy_meter_type) {
			energy_meter_handler = energy_meter_handler_item;
		}
	}

	return energy_meter_handler;
}

static void energy_meter_periodic(void *fn_ctx, void *chain_ctx)
{
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)fn_ctx;
	channel_info_t *channel_info = (channel_info_t *)chain_ctx;
	uint32_t ticks = osKernelSysTick();
	uint8_t fault = 0;

	if(ticks_duration(ticks, energy_meter_info->alive_stamps) >= 5000) {
		fault = 1;
	}

	if(get_fault(channel_info->faults, CHANNEL_FAULT_ENERGYMETER) != fault) {
		set_fault(channel_info->faults, CHANNEL_FAULT_ENERGYMETER, fault);
	}
}

energy_meter_info_t *alloc_energy_meter_info(channel_info_t *channel_info)
{
	channel_settings_t *channel_settings = &channel_info->channel_settings;
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)os_calloc(1, sizeof(energy_meter_info_t));

	OS_ASSERT(energy_meter_info != NULL);
	energy_meter_info->channel_info = channel_info;

	energy_meter_info->energy_meter_periodic_cb.fn = energy_meter_periodic;
	energy_meter_info->energy_meter_periodic_cb.fn_ctx = energy_meter_info;
	OS_ASSERT(register_callback(channel_info->channel_periodic_chain, &energy_meter_info->energy_meter_periodic_cb) == 0);

	energy_meter_info->energy_meter_handler = get_energy_meter_handler(channel_settings->energy_meter_settings.type);

	if((energy_meter_info->energy_meter_handler != NULL) && (energy_meter_info->energy_meter_handler->init != NULL)) {
		energy_meter_info->energy_meter_handler->init(energy_meter_info);
	}

	return energy_meter_info;
}
