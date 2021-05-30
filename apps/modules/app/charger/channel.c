

/*================================================================
 *
 *
 *   文件名称：channel.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月08日 星期四 09时51分12秒
 *   修改日期：2021年05月30日 星期日 14时22分58秒
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

char *get_channel_state_des(channel_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHANNEL_STATE_NONE);
			add_des_case(CHANNEL_STATE_IDLE);
			add_des_case(CHANNEL_STATE_START);
			add_des_case(CHANNEL_STATE_STARTING);
			add_des_case(CHANNEL_STATE_CHARGING);
			add_des_case(CHANNEL_STATE_STOPPING);
			add_des_case(CHANNEL_STATE_STOP);

		default: {
		}
		break;
	}

	return des;
}

int set_channel_request_state(channel_info_t *channel_info, channel_state_t state)
{
	int ret = -1;

	if(channel_info->request_state == CHANNEL_STATE_NONE) {
		ret = 0;
	}

	debug("request_state %s -> %s", get_channel_state_des(channel_info->request_state), get_channel_state_des(state));
	channel_info->request_state = state;

	return ret;
}

static void handle_channel_request_state(channel_info_t *channel_info)
{
	channel_state_t state = channel_info->state;

	switch(channel_info->state) {
		case CHANNEL_STATE_IDLE: {
			switch(channel_info->request_state) {
				case CHANNEL_STATE_START: {
					channel_info->state = CHANNEL_STATE_START;
				}
				break;

				default: {
				}
				break;
			}

		}
		break;

		case CHANNEL_STATE_START: {
			switch(channel_info->request_state) {
				case CHANNEL_STATE_STARTING: {
					channel_info->state = CHANNEL_STATE_STARTING;
				}
				break;

				default: {
				}
				break;
			}
		}
		break;

		case CHANNEL_STATE_STARTING: {
			switch(channel_info->request_state) {
				case CHANNEL_STATE_CHARGING: {
					channel_info->state = CHANNEL_STATE_CHARGING;
				}
				break;

				default: {
				}
				break;
			}
		}
		break;

		case CHANNEL_STATE_CHARGING: {
			switch(channel_info->request_state) {
				case CHANNEL_STATE_STOPPING: {
					channel_info->state = CHANNEL_STATE_STOPPING;
				}
				break;

				default: {
				}
				break;
			}
		}
		break;

		case CHANNEL_STATE_STOPPING: {
			switch(channel_info->request_state) {
				case CHANNEL_STATE_STOP: {
					channel_info->state = CHANNEL_STATE_STOP;
				}
				break;

				default: {
				}
				break;
			}
		}
		break;

		case CHANNEL_STATE_STOP: {
			switch(channel_info->request_state) {
				case CHANNEL_STATE_IDLE: {
					channel_info->state = CHANNEL_STATE_IDLE;
				}
				break;

				default: {
				}
				break;
			}
		}
		break;

		default: {
		}
		break;
	}

	if(channel_info->request_state != CHANNEL_STATE_NONE) {
		channel_info->request_state = CHANNEL_STATE_NONE;
	}

	if(state != channel_info->state) {
		debug("channel state:%s -> %s!", get_channel_state_des(state), get_channel_state_des(channel_info->state));
	}

}

static void handle_channel_periodic(void *_channel_info, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	//debug("channel_info %d periodic!", channel_info->channel_id);

	switch(channel_info->state) {
		case CHANNEL_STATE_IDLE: {
			do_callback_chain(channel_info->idle_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_START: {
			do_callback_chain(channel_info->start_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_STARTING: {
			do_callback_chain(channel_info->starting_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_CHARGING: {
			do_callback_chain(channel_info->charging_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_STOPPING: {
			do_callback_chain(channel_info->stopping_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_STOP: {
			do_callback_chain(channel_info->stop_chain, channel_info);
		}
		break;

		default: {
		}
		break;
	}

	handle_channel_request_state(channel_info);
}

static int _handle_channel_event(channel_info_t *channel_info, channel_event_t *channel_event)
{
	int ret = -1;

	debug("channel_id %d process handler event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

	switch(channel_event->type) {
		case CHANNEL_EVENT_TYPE_START_CHANNEL: {
			set_channel_request_state(channel_info, CHANNEL_STATE_START);
		}
		break;

		case CHANNEL_EVENT_TYPE_STOP_CHANNEL: {
			set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static void handle_channel_event(void *_channel_info, void *_channels_event)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;
	channel_event_t *channel_event;
	uint8_t match = 0;

	if(channels_event->type != CHANNELS_EVENT_CHANNEL_EVENT) {
		return;
	}

	channel_event = &channels_event->event.channel_event;

	if(channel_event->channel_id == 0xff) { //broadcast
		match = 1;
	}

	if(channel_event->channel_id == channel_info->channel_id) {
		match = 1;
	}

	if(match == 0) {
		return;
	}

	_handle_channel_event(channel_info, channel_event);
}

static int channel_init(channel_info_t *channel_info)
{
	int ret = 0;
	channel_config_t *channel_config = channel_info->channel_config;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;

	channel_info->idle_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->idle_chain != NULL);
	channel_info->start_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->start_chain != NULL);
	channel_info->starting_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->starting_chain != NULL);
	channel_info->charging_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->charging_chain != NULL);
	channel_info->stopping_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->stopping_chain != NULL);
	channel_info->stop_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->stop_chain != NULL);

	debug("channel %d init charger %s", channel_info->channel_id, get_channel_config_channel_type(channel_config->channel_type));
	channel_info->channel_handler = get_channel_handler(channel_config->channel_type);

	if((channel_info->channel_handler != NULL) && (channel_info->channel_handler->init != NULL)) {
		OS_ASSERT(channel_info->channel_handler->init(channel_info) == 0);
	}

	channel_info->periodic_callback_item.fn = handle_channel_periodic;
	channel_info->periodic_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channel_info->periodic_callback_item) == 0);

	channel_info->event_callback_item.fn = handle_channel_event;
	channel_info->event_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channel_info->event_callback_item) == 0);

	debug("channel %d alloc charger %s", channel_info->channel_id, get_channel_config_charger_type(channel_config->charger_config.charger_type));
	channel_info->charger_info = alloc_charger_info(channel_info);
	debug("channel %d alloc energy_meter %s", channel_info->channel_id, get_channel_config_energy_meter_type(channel_config->energy_meter_config.energy_meter_type));
	channel_info->energy_meter_info = alloc_energy_meter_info(channel_info);

	channel_info->request_state = CHANNEL_STATE_NONE;
	channel_info->state = CHANNEL_STATE_IDLE;

	return ret;
}

static void handle_channels_common_periodic(void *_channels_info, void *chain_ctx)
{
	//debug("channels_info common periodic!");
}

static void handle_channels_common_event(void *_channels_info, void *_channels_event)
{
	//channels_info_t *channels_info = (channels_info_t *)_channels_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;

	debug("channels_info common process event %s!", get_channel_event_type_des(channels_event->type));

	switch(channels_event->type) {
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

	channels_info->periodic_callback_item.fn = handle_channels_common_periodic;
	channels_info->periodic_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channels_info->periodic_callback_item) == 0);

	channels_info->event_callback_item.fn = handle_channels_common_event;
	channels_info->event_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channels_info->event_callback_item) == 0);

	return channel_info;
}
