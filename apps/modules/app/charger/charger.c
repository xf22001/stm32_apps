

/*================================================================
 *
 *
 *   文件名称：charger.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月19日 星期二 12时32分21秒
 *   修改日期：2021年05月11日 星期二 11时23分35秒
 *   描    述：
 *
 *================================================================*/
#include "charger.h"

#include <string.h>

#include "charger_handler_bms_native.h"
#include "channels.h"
#include "os_utils.h"
#include "can_data_task.h"

#include "log.h"

charger_handler_t *charger_handler_sz[] = {
	&charger_handler_bms_native,
};

static charger_handler_t *get_charger_handler(channel_charger_type_t charger_type)
{
	int i;
	charger_handler_t *charger_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(charger_handler_sz); i++) {
		charger_handler_t *charger_handler_item = charger_handler_sz[i];

		if(charger_handler_item->charger_type == charger_type) {
			charger_handler = charger_handler_item;
		}
	}

	return charger_handler;
}

charger_info_t *alloc_charger_info(channel_info_t *channel_info)
{
	charger_info_t *charger_info = NULL;
	channel_config_t *channel_config = channel_info->channel_config;

	charger_info = (charger_info_t *)os_calloc(1, sizeof(charger_info_t));

	OS_ASSERT(charger_info != NULL);

	charger_info->channel_info = channel_info;

	charger_info->charger_bms_status_changed = alloc_callback_chain();

	OS_ASSERT(charger_info->charger_bms_status_changed != NULL);

	charger_info->charger_handler = get_charger_handler(channel_config->charger_config.charger_type);

	if((charger_info->charger_handler != NULL) && (charger_info->charger_handler->init != NULL)) {
		charger_info->charger_handler->init(charger_info);
	}

	return charger_info;
}

