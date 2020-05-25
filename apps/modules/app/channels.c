

/*================================================================
 *
 *
 *   文件名称：channels.c
 *   创 建 者：肖飞
 *   创建日期：2020年01月02日 星期四 08时53分35秒
 *   修改日期：2020年05月25日 星期一 15时10分35秒
 *   描    述：
 *
 *================================================================*/
#include "channels.h"
#include "os_utils.h"
#include <string.h>

#include "log.h"

static LIST_HEAD(channels_info_list);
osMutexId channels_info_list_mutex = NULL;

static void default_periodic(channels_info_t *channels_info)
{
	//_printf("%s\n", __func__);
}

static void channels_periodic(channels_info_t *channels_info)
{
	static uint32_t expire = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks >= expire) {
		expire = ticks + CHANNEL_TASK_PERIODIC;
		default_periodic(channels_info);
	}
}

static int default_handle_channel_event(channel_event_t *channel_event)
{
	int ret = 0;
	_printf("channel %d process event!\n", channel_event->channel_id);
	return ret;
}

static channels_info_t *get_channels_info(channels_info_config_t *channels_info_config)
{
	channels_info_t *channels_info = NULL;
	channels_info_t *channels_info_item = NULL;
	osStatus os_status;

	if(channels_info_list_mutex == NULL) {
		return channels_info;
	}

	os_status = osMutexWait(channels_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}


	list_for_each_entry(channels_info_item, &channels_info_list, channels_info_t, list) {
		if(channels_info_item->channels_info_config == channels_info_config) {
			channels_info = channels_info_item;
			break;
		}
	}

	os_status = osMutexRelease(channels_info_list_mutex);

	if(os_status != osOK) {
	}

	return channels_info;
}

void free_channels_info(channels_info_t *channels_info)
{
	osStatus os_status;

	if(channels_info == NULL) {
		return;
	}

	if(channels_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(channels_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&channels_info->list);

	os_status = osMutexRelease(channels_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channels_info->event_pool != NULL) {
		free_event_pool(channels_info->event_pool);
	}

	os_free(channels_info);

	return;
}

static int channels_set_channels_info_config(channels_info_t *channels_info, channels_info_config_t *channels_info_config)
{
	int ret = -1;
	int i;
	event_pool_t *event_pool;

	event_pool = alloc_event_pool();

	if(event_pool == NULL) {
		return ret;
	}

	channels_info->event_pool = event_pool;

	for(i = 0; i < CHANNEL_INSTANCES_NUMBER; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;
		channel_info->channel_id = i;
		channel_info->state = CHANNEL_STATE_IDLE;
		channel_info->handle_channel_event = default_handle_channel_event;
	}

	ret = 0;

	return ret;
}

channels_info_t *get_or_alloc_channels_info(channels_info_config_t *channels_info_config)
{
	channels_info_t *channels_info = NULL;
	osStatus os_status;

	channels_info = get_channels_info(channels_info_config);

	if(channels_info != NULL) {
		return channels_info;
	}

	if(channels_info_config == NULL) {
		return channels_info;
	}

	if(channels_info_list_mutex == NULL) {
		osMutexDef(channels_info_list_mutex);
		channels_info_list_mutex = osMutexCreate(osMutex(channels_info_list_mutex));

		if(channels_info_list_mutex == NULL) {
			return channels_info;
		}
	}

	channels_info = (channels_info_t *)os_alloc(sizeof(channels_info_t));

	if(channels_info == NULL) {
		return channels_info;
	}

	memset(channels_info, 0, sizeof(channels_info_t));

	channels_info->channels_info_config = channels_info_config;

	os_status = osMutexWait(channels_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&channels_info->list, &channels_info_list);

	os_status = osMutexRelease(channels_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channels_set_channels_info_config(channels_info, channels_info_config) != 0) {
		goto failed;
	}

	return channels_info;

failed:

	free_channels_info(channels_info);
	channels_info = NULL;

	return channels_info;
}

void channels_process_event(channels_info_t *channels_info)
{
	channel_event_t *channel_event = NULL;
	int8_t i;
	channel_info_t *channel_info = NULL;
	int ret = -1;

	if(channels_info == NULL) {
		return;
	}

	ret = event_pool_wait_event(channels_info->event_pool, CHANNEL_TASK_PERIODIC);

	if(ret != 0) {
		return;
	}

	for(channel_event = event_pool_get_event(channels_info->event_pool);
	    channel_event != NULL;
	    channel_event = event_pool_get_event(channels_info->event_pool)) {
		if(channel_event->channel_id == 0xff) { //broadcast
			for(i = 0; i < CHANNEL_INSTANCES_NUMBER; i++) {
				channel_info = channels_info->channel_info + i;
				channel_info->handle_channel_event(channel_event);
			}
		} else {
			if(channel_event->channel_id < CHANNEL_INSTANCES_NUMBER) {
				channel_info = channels_info->channel_info + channel_event->channel_id;
				channel_info->handle_channel_event(channel_event);
			} else { //id error
			}
		}

		os_free(channel_event);
	}
}

void task_channels(void const *argument)
{
	channels_info_t *channels_info = (channels_info_t *)argument;

	if(channels_info == NULL) {
		app_panic();
	}

	while(1) {
		channels_process_event(channels_info);

		//处理周期性事件
		channels_periodic(channels_info);
	}
}

int send_channel_event(channels_info_t *channels_info, channel_event_t *channel_event, uint32_t timeout)
{
	return event_pool_put_event(channels_info->event_pool, channel_event, timeout);
}

void task_channel_event(void const *argument)
{
	channels_info_t *channels_info = (channels_info_t *)argument;
	uint8_t id = 0;

	if(channels_info == NULL) {
		app_panic();
	}

	while(1) {
		channel_event_t *channel_event = (channel_event_t *)os_alloc(sizeof(channel_event_t));
		static uint32_t error_count = 0;

		if(channel_event != NULL) {
			int ret;

			channel_event->channel_id = id;
			ret = send_channel_event(channels_info, channel_event, 0);

			if(ret != 0) {
				error_count++;
				_printf("error_count:%u\n", error_count);
				os_free(channel_event);
			}
		}

		id++;

		if(id >= CHANNEL_INSTANCES_NUMBER) {
			id = 0;
		}

		osDelay(1000);
	}
}
