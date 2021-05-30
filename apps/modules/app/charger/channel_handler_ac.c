

/*================================================================
 *
 *
 *   文件名称：channel_handler_ac.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月11日 星期二 09时20分53秒
 *   修改日期：2021年05月30日 星期日 11时59分20秒
 *   描    述：
 *
 *================================================================*/
#include "channel_handler_ac.h"

#include "log.h"

typedef struct {
	callback_item_t handler_periodic_callback_item;
	callback_item_t handler_event_callback_item;
} channel_handler_ctx_t;

static void handle_channel_handler_periodic(void *_channel_info, void *_channels_info)
{
	//debug("channel_id %d handler periodic!", ((channel_info_t *)_channel_info)->channel_id);
}

static int _handle_channel_handler_event(channel_info_t *channel_info, channel_event_t *channel_event)
{
	int ret = -1;

	debug("channel_id %d process handler event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

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

static void handle_channel_handler_event(void *_channel_info, void *_channels_event)
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

	_handle_channel_handler_event(channel_info, channel_event);
}

static int init(void *_channel_info)
{
	int ret = 0;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	channel_handler_ctx_t *channel_handler_ctx = os_calloc(1, sizeof(channel_handler_ctx_t));

	OS_ASSERT(channel_handler_ctx != NULL);

	channel_handler_ctx->handler_periodic_callback_item.fn = handle_channel_handler_periodic;
	channel_handler_ctx->handler_periodic_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channel_handler_ctx->handler_periodic_callback_item) == 0);

	channel_handler_ctx->handler_event_callback_item.fn = handle_channel_handler_event;
	channel_handler_ctx->handler_event_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channel_handler_ctx->handler_event_callback_item) == 0);

	channel_info->channel_handler_ctx = channel_handler_ctx;

	return ret;
}

channel_handler_t channel_handler_ac = {
	.channel_type = CHANNEL_TYPE_AC,
	.init = init,
};
