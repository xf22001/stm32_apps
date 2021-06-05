

/*================================================================
 *   
 *   
 *   文件名称：energy_meter.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月07日 星期三 15时56分19秒
 *   修改日期：2021年06月05日 星期六 13时07分15秒
 *   描    述：
 *
 *================================================================*/
#include "energy_meter.h"
#include "channels.h"
#include "uart_data_task.h"
#include "energy_meter_handler_ac.h"
#include "energy_meter_handler_dc.h"

static int handle_init_ac(void *_energy_meter_info)
{
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)_energy_meter_info;
	return energy_meter_ac_init(energy_meter_info);
}

static energy_meter_handler_t energy_meter_handler_ac = {
	.energy_meter_type = CHANNEL_ENERGY_METER_TYPE_AC,
	.handle_init = handle_init_ac,
};

static int handle_init_dc(void *_energy_meter_info)
{
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)_energy_meter_info;
	return energy_meter_dc_init(energy_meter_info);
}

static energy_meter_handler_t energy_meter_handler_dc = {
	.energy_meter_type = CHANNEL_ENERGY_METER_TYPE_DC,
	.handle_init = handle_init_dc,
};

energy_meter_handler_t *energy_meter_handler_sz[] = {
	&energy_meter_handler_ac,
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

energy_meter_info_t *alloc_energy_meter_info(channel_info_t *channel_info)
{
	channel_config_t *channel_config = channel_info->channel_config;
	channel_energy_meter_config_t *channel_energy_meter_config = &channel_config->energy_meter_config;
	energy_meter_info_t *energy_meter_info = (energy_meter_info_t *)os_calloc(1, sizeof(energy_meter_info_t));

	OS_ASSERT(energy_meter_info != NULL);
	energy_meter_info->channel_info = channel_info;

	energy_meter_info->energy_meter_handler = get_energy_meter_handler(channel_energy_meter_config->energy_meter_type);

	if((energy_meter_info->energy_meter_handler != NULL) && (energy_meter_info->energy_meter_handler->handle_init != NULL)) {
		energy_meter_info->energy_meter_handler->handle_init(energy_meter_info);
	}

	return energy_meter_info;
}
