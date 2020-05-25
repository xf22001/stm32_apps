

/*================================================================
 *   
 *   
 *   文件名称：channels_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 11时13分49秒
 *   修改日期：2020年05月25日 星期一 16时42分55秒
 *   描    述：
 *
 *================================================================*/
#include "channels_config.h"

#include "os_utils.h"
#include "log.h"
#include "main.h"
#include "auxiliary_function_board.h"

extern CAN_HandleTypeDef hcan1;

channels_info_config_t channels_info_config = {
	.id = 0,
	.hcan_com = &hcan1,
};

static channels_info_config_t *channels_info_config_sz[] = {
	&channels_info_config,
};

channels_info_config_t *get_channels_info_config(uint8_t id)
{
	int i;
	channels_info_config_t *channels_info_config = NULL;
	channels_info_config_t *channels_info_config_item = NULL;

	for(i = 0; i < ARRAY_SIZE(channels_info_config_sz); i++) {
		channels_info_config_item = channels_info_config_sz[i];

		if(channels_info_config_item->id == id) {
			channels_info_config = channels_info_config_item;
			break;
		}
	}

	return channels_info_config;
}
