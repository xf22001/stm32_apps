

/*================================================================
 *   
 *   
 *   文件名称：channel_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分44秒
 *   修改日期：2020年04月30日 星期四 10时54分10秒
 *   描    述：
 *
 *================================================================*/
#include "channel_communication.h"
#include <string.h>
#include "os_utils.h"

static LIST_HEAD(channel_com_info_list);
static osMutexId channel_com_info_list_mutex = NULL;

static channel_com_info_t *get_channel_com_info(can_info_t *can_info)
{
	channel_com_info_t *channel_com_info = NULL;
	channel_com_info_t *channel_com_info_item = NULL;
	osStatus os_status;

	if(channel_com_info_list_mutex == NULL) {
		return channel_com_info;
	}

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(channel_com_info_item, &channel_com_info_list, channel_com_info_t, list) {
		if(channel_com_info_item->can_info == can_info) {
			channel_com_info = channel_com_info_item;
			break;
		}
	}

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	return channel_com_info;
}

void free_channel_com_info(channel_com_info_t *channel_com_info)
{
	osStatus os_status;

	if(channel_com_info == NULL) {
		return;
	}

	if(channel_com_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&channel_com_info->list);

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	os_free(channel_com_info);
}

channel_com_info_t *get_or_alloc_channel_com_info(can_info_t *can_info)
{
	channel_com_info_t *channel_com_info = NULL;
	osStatus os_status;

	channel_com_info = get_channel_com_info(can_info);

	if(channel_com_info != NULL) {
		return channel_com_info;
	}

	if(channel_com_info_list_mutex == NULL) {
		osMutexDef(channel_com_info_list_mutex);
		channel_com_info_list_mutex = osMutexCreate(osMutex(channel_com_info_list_mutex));

		if(channel_com_info_list_mutex == NULL) {
			return channel_com_info;
		}
	}

	channel_com_info = (channel_com_info_t *)os_alloc(sizeof(channel_com_info_t));

	if(channel_com_info == NULL) {
		return channel_com_info;
	}

	memset(channel_com_info, 0, sizeof(channel_com_info_t));

	channel_com_info->can_info = can_info;

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&channel_com_info->list, &channel_com_info_list);

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	return channel_com_info;
}

int channel_com_info_set_channel_config(channel_com_info_t *channel_com_info, channel_info_config_t *channel_info_config)
{
	int ret = 0;
	channel_com_info->channel_info_config = channel_info_config;
	return ret;
}
