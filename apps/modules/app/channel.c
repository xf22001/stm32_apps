

/*================================================================
 *
 *
 *   文件名称：channel.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月30日 星期四 08时56分05秒
 *   修改日期：2020年05月01日 星期五 21时42分50秒
 *   描    述：
 *
 *================================================================*/
#include "channel.h"
#include <string.h>
#include "os_utils.h"
#define UDP_LOG
#include "task_probe_tool.h"

static LIST_HEAD(channel_info_list);
static osMutexId channel_info_list_mutex = NULL;

static channel_info_t *get_channel_info(channel_info_config_t *channel_info_config)
{
	channel_info_t *channel_info = NULL;
	channel_info_t *channel_info_item = NULL;
	osStatus os_status;

	if(channel_info_list_mutex == NULL) {
		return channel_info;
	}

	os_status = osMutexWait(channel_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(channel_info_item, &channel_info_list, channel_info_t, list) {
		if(channel_info_item->channel_id == channel_info_config->channel_id) {
			channel_info = channel_info_item;
			break;
		}
	}

	os_status = osMutexRelease(channel_info_list_mutex);

	if(os_status != osOK) {
	}

	return channel_info;
}

void free_channel_info(channel_info_t *channel_info)
{
	osStatus os_status;

	if(channel_info == NULL) {
		return;
	}

	if(channel_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(channel_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&channel_info->list);

	os_status = osMutexRelease(channel_info_list_mutex);

	if(os_status != osOK) {
	}

	os_free(channel_info);
}

static int channel_set_channel_config(channel_info_t *channel_info, channel_info_config_t *channel_info_config)
{
	int ret = -1;

	channel_info->channel_info_config = channel_info_config;

	channel_info->channel_id = channel_info_config->channel_id;

	return ret;
}

channel_info_t *get_or_alloc_channel_info(channel_info_config_t *channel_info_config)
{
	channel_info_t *channel_info = NULL;
	osStatus os_status;

	channel_info = get_channel_info(channel_info_config);

	if(channel_info != NULL) {
		return channel_info;
	}

	if(channel_info_list_mutex == NULL) {
		osMutexDef(channel_info_list_mutex);
		channel_info_list_mutex = osMutexCreate(osMutex(channel_info_list_mutex));

		if(channel_info_list_mutex == NULL) {
			return channel_info;
		}
	}

	channel_info = (channel_info_t *)os_alloc(sizeof(channel_info_t));

	if(channel_info == NULL) {
		return channel_info;
	}

	memset(channel_info, 0, sizeof(channel_info_t));

	if(channel_set_channel_config(channel_info, channel_info_config) == 0) {
		goto failed;
	}

	os_status = osMutexWait(channel_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&channel_info->list, &channel_info_list);

	os_status = osMutexRelease(channel_info_list_mutex);

	if(os_status != osOK) {
	}

	return channel_info;
failed:

	if(channel_info != NULL) {
		os_free(channel_info);
	}

	return channel_info;
}

static uint8_t get_gun_connect_state(channel_info_config_t *channel_info_config)
{
	uint8_t state;

	state = 1;
	udp_log_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);

	return state;
}

static void channel_update_gun_state(channel_info_t *channel_info)
{
	uint8_t state;

	if(channel_info->channel_info_config == NULL) {
		app_panic();
	}

	state = get_gun_connect_state(channel_info->channel_info_config);

	if(state != channel_info->gun_connect_state) {
		channel_info->gun_connect_state_debounce_count++;

		if(channel_info->gun_connect_state_debounce_count >= 3) {
			channel_info->gun_connect_state = state;
		}
	} else {
		channel_info->gun_connect_state_debounce_count = 0;
	}
}