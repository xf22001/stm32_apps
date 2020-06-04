

/*================================================================
 *
 *
 *   文件名称：channel.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月29日 星期五 16时34分48秒
 *   修改日期：2020年06月04日 星期四 14时03分56秒
 *   描    述：
 *
 *================================================================*/
#include "channel.h"

#include "os_utils.h"
#include <string.h>
#include "log.h"

static int default_handle_channel_event(void *channel_info, channel_event_t *channel_event)
{
	int ret = 0;
	//debug("channel %d process event!\n", channel_event->channel_id);
	return ret;
}

void set_channel_request_state(channel_info_t *channel_info, channel_request_state_t channel_request_state)
{
	channel_info->channel_request_state = channel_request_state;
}

channel_request_state_t get_channel_request_state(channel_info_t *channel_info)
{
	return channel_info->channel_request_state;
}

channel_state_t get_channel_state(channel_info_t *channel_info)
{
	return channel_info->channel_state;
}

static void process_channel_state(channel_info_t *channel_info)
{
	switch(channel_info->channel_state) {
		case CHANNEL_STATE_IDLE: {
			if(channel_info->channel_request_state == CHANNEL_REQUEST_STATE_START) {
				channel_info->channel_request_state = CHANNEL_REQUEST_STATE_NONE;
				channel_info->channel_state = CHANNEL_STATE_START;
			}
		}
		break;

		case CHANNEL_STATE_START: {//发启动指令
		}
		break;

		case CHANNEL_STATE_RUNNING: {
		}
		break;

		case CHANNEL_STATE_STOP: {
		}
		break;

		default: {
		}
		break;
	}
}

static void default_handle_channel_periodic(void *channel_info)
{
	//_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
	process_channel_state((channel_info_t *)channel_info);
}

static channel_callback_t default_channel_callback = {
	.handle_channel_event = default_handle_channel_event,
	.handle_channel_periodic = default_handle_channel_periodic,
};

channel_callback_t *get_channel_callback(uint8_t channel_id)
{
	channel_callback_t *channel_callback = NULL;

	switch(channel_id) {
		default: {
			channel_callback = &default_channel_callback;
		}
		break;
	}

	return channel_callback;
}
