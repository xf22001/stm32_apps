

/*================================================================
 *
 *
 *   文件名称：channel_handler_proxy.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月05日 星期六 22时43分22秒
 *   修改日期：2021年06月07日 星期一 09时30分51秒
 *   描    述：
 *
 *================================================================*/
#include "channel_handler_proxy.h"

#include "log.h"

typedef struct {
	callback_item_t handler_periodic_callback_item;
	callback_item_t handler_event_callback_item;

	callback_item_t idle_callback_item;
	callback_item_t start_callback_item;
	callback_item_t starting_callback_item;
	callback_item_t charging_callback_item;
	callback_item_t stopping_callback_item;
	callback_item_t stop_callback_item;
	callback_item_t state_changed_callback_item;

	uint8_t state;
	uint32_t state_stamps;
} channel_handler_ctx_t;

static void idle(void *_channel_info, void *__channel_info)
{
}

static void start(void *_channel_info, void *__channel_info)
{
}

static void starting(void *_channel_info, void *__channel_info)
{
}

static void charging(void *_channel_info, void *__channel_info)
{
}

static void stopping(void *_channel_info, void *__channel_info)
{
}

static void stop(void *_channel_info, void *__channel_info)
{
}

static void state_changed(void *_channel_info, void *_pre_state)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channel_handler_ctx_t *channel_handler_ctx = (channel_handler_ctx_t *)channel_info->channel_handler_ctx;

	debug("channel state:%s -> %s!", get_channel_state_des(channel_info->state), get_channel_state_des(channel_info->request_state));
	channel_info->state = channel_info->request_state;
	channel_handler_ctx->state = 0;
}

static void handle_channel_handler_periodic(void *_channel_info, void *_channels_info)
{
	//debug("channel_id %d handler periodic!", ((channel_info_t *)_channel_info)->channel_id);
}

static int _handle_channel_handler_event(channel_info_t *channel_info, channel_event_t *channel_event)
{
	int ret = -1;

	debug("channel_id %d process handler event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

	switch(channel_event->type) {
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

	channel_handler_ctx->idle_callback_item.fn = idle;
	channel_handler_ctx->idle_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->idle_chain, &channel_handler_ctx->idle_callback_item) == 0);

	channel_handler_ctx->start_callback_item.fn = start;
	channel_handler_ctx->start_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->start_chain, &channel_handler_ctx->start_callback_item) == 0);

	channel_handler_ctx->starting_callback_item.fn = starting;
	channel_handler_ctx->starting_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->starting_chain, &channel_handler_ctx->starting_callback_item) == 0);

	channel_handler_ctx->charging_callback_item.fn = charging;
	channel_handler_ctx->charging_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->charging_chain, &channel_handler_ctx->charging_callback_item) == 0);

	channel_handler_ctx->stopping_callback_item.fn = stopping;
	channel_handler_ctx->stopping_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->stopping_chain, &channel_handler_ctx->stopping_callback_item) == 0);

	channel_handler_ctx->stop_callback_item.fn = stop;
	channel_handler_ctx->stop_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->stop_chain, &channel_handler_ctx->stop_callback_item) == 0);

	channel_handler_ctx->state_changed_callback_item.fn = state_changed;
	channel_handler_ctx->state_changed_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->state_changed_chain, &channel_handler_ctx->state_changed_callback_item) == 0);

	channel_handler_ctx->handler_periodic_callback_item.fn = handle_channel_handler_periodic;
	channel_handler_ctx->handler_periodic_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channel_handler_ctx->handler_periodic_callback_item) == 0);

	channel_handler_ctx->handler_event_callback_item.fn = handle_channel_handler_event;
	channel_handler_ctx->handler_event_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channel_handler_ctx->handler_event_callback_item) == 0);

	channel_info->channel_handler_ctx = channel_handler_ctx;

	OS_ASSERT(start_channel_comm_channel(channel_info) == 0);

	return ret;
}

static int channel_start(void *_channel_info)
{
	int ret = -1;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	switch(channel_info->state) {
		case CHANNEL_STATE_IDLE: {
			set_channel_request_state(channel_info, CHANNEL_STATE_START);
			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static int channel_stop(void *_channel_info)
{
	int ret = -1;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	switch(channel_info->state) {
		case CHANNEL_STATE_START:
		case CHANNEL_STATE_STARTING:
		case CHANNEL_STATE_CHARGING: {
			set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}
channel_handler_t channel_handler_proxy = {
	.channel_type = CHANNEL_TYPE_PROXY,
	.init = init,
	.channel_start = channel_start,
	.channel_stop = channel_stop,
};
