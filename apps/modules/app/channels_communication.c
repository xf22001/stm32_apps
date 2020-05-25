

/*================================================================
 *   
 *   
 *   文件名称：channels_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月25日 星期一 14时24分07秒
 *   修改日期：2020年05月25日 星期一 15时58分58秒
 *   描    述：
 *
 *================================================================*/
#include "channels_communication.h"
#include <string.h>

static LIST_HEAD(channels_com_info_list);
static osMutexId channels_com_info_list_mutex = NULL;

typedef enum {
	CHANNELS_COM_CMD_1_101 = 0,
	CHANNELS_COM_CMD_2_102,
	CHANNELS_COM_CMD_13_113,
	CHANNELS_COM_CMD_3_103,
	CHANNELS_COM_CMD_4_104,
	CHANNELS_COM_CMD_5_105,
	CHANNELS_COM_CMD_6_106,
	CHANNELS_COM_CMD_7_107,
	CHANNELS_COM_CMD_8_108,
	CHANNELS_COM_CMD_9_109,
	CHANNELS_COM_CMD_10_110,
	CHANNELS_COM_CMD_11_111,
	CHANNELS_COM_CMD_20_120,
	CHANNELS_COM_CMD_21_121,
	CHANNELS_COM_CMD_22_122,
	CHANNELS_COM_CMD_25_125,
	CHANNELS_COM_CMD_30_130,
	CHANNELS_COM_CMD_50_150,
	CHANNELS_COM_CMD_51_151,
	CHANNELS_COM_CMD_60_160,
	CHANNELS_COM_CMD_61_161,
	CHANNELS_COM_CMD_62_162,
	CHANNELS_COM_CMD_63_163,
	CHANNELS_COM_CMD_64_164,
	CHANNELS_COM_CMD_65_165,
	CHANNELS_COM_CMD_66_166,
	CHANNELS_COM_CMD_67_167,
	CHANNELS_COM_CMD_68_168,
	CHANNELS_COM_CMD_69_169,
	CHANNELS_COM_CMD_70_170,
	CHANNELS_COM_CMD_71_171,
	CHANNELS_COM_CMD_72_172,
	CHANNELS_COM_CMD_73_173,
	CHANNELS_COM_CMD_TOTAL,
} channels_com_cmd_t;

static channels_com_info_t *get_channels_com_info(channels_info_config_t *channels_info_config)
{
	channels_com_info_t *channels_com_info = NULL;
	channels_com_info_t *channels_com_info_item = NULL;
	osStatus os_status;

	if(channels_com_info_list_mutex == NULL) {
		return channels_com_info;
	}

	os_status = osMutexWait(channels_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(channels_com_info_item, &channels_com_info_list, channels_com_info_t, list) {
		if(channels_com_info_item->channels_info_config == channels_info_config) {
			channels_com_info = channels_com_info_item;
			break;
		}
	}

	os_status = osMutexRelease(channels_com_info_list_mutex);

	if(os_status != osOK) {
	}

	return channels_com_info;
}

void free_channels_com_info(channels_com_info_t *channels_com_info)
{
	osStatus os_status;

	if(channels_com_info == NULL) {
		return;
	}

	if(channels_com_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(channels_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&channels_com_info->list);

	os_status = osMutexRelease(channels_com_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channels_com_info->cmd_ctx != NULL) {
		os_free(channels_com_info->cmd_ctx);
	}

	os_free(channels_com_info);
}

static int channels_com_info_set_channel_config(channels_com_info_t *channels_com_info, channels_info_config_t *channels_info_config)
{
	int ret = -1;
	can_info_t *can_info;

	can_info = get_or_alloc_can_info(channels_info_config->hcan_com);

	if(can_info == NULL) {
		return ret;
	}

	channels_com_info->can_info = can_info;

	ret = 0;
	return ret;
}

channels_com_info_t *get_or_alloc_channels_com_info(channels_info_config_t *channels_info_config)
{
	channels_com_info_t *channels_com_info = NULL;
	osStatus os_status;

	channels_com_info = get_channels_com_info(channels_info_config);

	if(channels_com_info != NULL) {
		return channels_com_info;
	}

	if(channels_com_info_list_mutex == NULL) {
		osMutexDef(channels_com_info_list_mutex);
		channels_com_info_list_mutex = osMutexCreate(osMutex(channels_com_info_list_mutex));

		if(channels_com_info_list_mutex == NULL) {
			return channels_com_info;
		}
	}

	channels_com_info = (channels_com_info_t *)os_alloc(sizeof(channels_com_info_t));

	if(channels_com_info == NULL) {
		return channels_com_info;
	}

	memset(channels_com_info, 0, sizeof(channels_com_info_t));

	channels_com_info->cmd_ctx = (channels_com_cmd_ctx_t *)os_alloc(sizeof(channels_com_cmd_ctx_t) * CHANNELS_COM_CMD_TOTAL);

	if(channels_com_info->cmd_ctx == NULL) {
		goto failed;
	}

	memset(channels_com_info->cmd_ctx, 0, sizeof(channels_com_cmd_t) * CHANNELS_COM_CMD_TOTAL);

	channels_com_info->channels_info_config = channels_info_config;

	os_status = osMutexWait(channels_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&channels_com_info->list, &channels_com_info_list);

	os_status = osMutexRelease(channels_com_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channels_com_info_set_channel_config(channels_com_info, channels_info_config) != 0) {
		goto failed;
	}

	return channels_com_info;
failed:

	free_channels_com_info(channels_com_info);

	channels_com_info = NULL;

	return channels_com_info;
}
