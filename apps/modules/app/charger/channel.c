

/*================================================================
 *
 *
 *   文件名称：channel.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月08日 星期四 09时51分12秒
 *   修改日期：2021年05月26日 星期三 15时23分55秒
 *   描    述：
 *
 *================================================================*/
#include "channel.h"

#include "channel_handler_dc.h"
#include "channel_handler_ac.h"
#include "charger.h"
#include "energy_meter.h"

#include "log.h"

static channel_handler_t *channel_handler_sz[] = {
	&channel_handler_dc,
	&channel_handler_ac,
};

static channel_handler_t *get_channel_handler(channel_type_t channel_type)
{
	int i;
	channel_handler_t *channel_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(channel_handler_sz); i++) {
		channel_handler_t *channel_handler_item = channel_handler_sz[i];

		if(channel_handler_item->channel_type == channel_type) {
			channel_handler = channel_handler_item;
		}
	}

	return channel_handler;
}

static int channel_init(channel_info_t *channel_info)
{
	int ret = 0;
	channel_config_t *channel_config = channel_info->channel_config;

	debug("channel %d init charger %s", channel_info->channel_id, get_channel_config_channel_type(channel_config->channel_type));
	channel_info->channel_handler = get_channel_handler(channel_config->channel_type);

	if((channel_info->channel_handler != NULL) && (channel_info->channel_handler->init != NULL)) {
		OS_ASSERT(channel_info->channel_handler->init(channel_info) == 0);
	}

	debug("channel %d alloc charger %s", channel_info->channel_id, get_channel_config_charger_type(channel_config->charger_config.charger_type));
	channel_info->charger_info = alloc_charger_info(channel_info);
	debug("channel %d alloc energy_meter %s", channel_info->channel_id, get_channel_config_energy_meter_type(channel_config->energy_meter_config.energy_meter_type));
	channel_info->energy_meter_info = alloc_energy_meter_info(channel_info);

	return ret;
}

static void handle_channels_periodic(void *_channels_info, void *chain_ctx)
{
	//debug("channels_info periodic!");
}

static int handle_channel_event(channels_info_t *channels_info, channel_event_t *channel_event)
{
	int ret = -1;

	switch(channel_event->type) {
		case CHANNEL_EVENT_TYPE_START_CHANNEL: {
		}
		break;

		case CHANNEL_EVENT_TYPE_STOP_CHANNEL: {
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static void handle_channels_event(void *_channels_info, void *_channels_event)
{
	channels_info_t *channels_info = (channels_info_t *)_channels_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;

	debug("channels_info process event %s!", get_channel_event_type_des(channels_event->type));

	switch(channels_event->type) {
		case CHANNELS_EVENT_CHANNEL_EVENT: {
			channel_event_t *channel_event = &channels_event->event.channel_event;
			handle_channel_event(channels_info, channel_event);
		}
		break;

		default: {
		}
		break;
	}
}


channel_info_t *alloc_channels_channel_info(channels_info_t *channels_info)
{
	channels_config_t *channels_config = channels_info->channels_config;
	channel_info_t *channel_info = NULL;
	int i;

	channels_info->channel_number = channels_config->channels_config.channels_number;
	channel_info = (channel_info_t *)os_calloc(channels_info->channel_number, sizeof(channel_info_t));
	OS_ASSERT(channel_info != NULL);

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info_item = channel_info + i;

		channel_info_item->channels_info = channels_info;
		channel_info_item->channel_config = channels_config->channels_config.channel_config[i];
		channel_info_item->channel_id = i;

		channel_init(channel_info_item);
	}

	channels_info->periodic_callback_item.fn = handle_channels_periodic;
	channels_info->periodic_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channels_info->periodic_callback_item) == 0);

	channels_info->event_callback_item.fn = handle_channels_event;
	channels_info->event_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channels_info->event_callback_item) == 0);

	return channel_info;
}
