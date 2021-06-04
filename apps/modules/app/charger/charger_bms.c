

/*================================================================
 *
 *
 *   文件名称：charger_bms.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月04日 星期五 16时51分31秒
 *   修改日期：2021年06月04日 星期五 23时45分49秒
 *   描    述：
 *
 *================================================================*/
#include "charger_bms.h"
#include "charger_bms_gb.h"

#include "log.h"

static charger_bms_handler_t *charger_bms_handler_sz[] = {
	&charger_bms_handler_gb,
};

static charger_bms_handler_t *get_charger_bms_handler(channel_charger_type_t channel_charger_type)
{
	int i;
	charger_bms_handler_t *charger_bms_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(charger_bms_handler_sz); i++) {
		charger_bms_handler_t *charger_bms_handler_item = charger_bms_handler_sz[i];

		if(charger_bms_handler_item->channel_charger_type == channel_charger_type) {
			charger_bms_handler = charger_bms_handler_item;
		}
	}

	return charger_bms_handler;
}

void set_charger_bms_request_action(charger_info_t *charger_info, charger_bms_request_action_t charger_bms_request_action)
{
	charger_info->charger_bms_request_action = charger_bms_request_action;
}

void set_charger_bms_request_state(charger_info_t *charger_info, uint8_t request_state)
{
	charger_info->request_state = request_state;
}

int charger_bms_init(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = (channel_info_t *)charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;

	charger_info->handle_mutex = mutex_create();
	OS_ASSERT(charger_info->handle_mutex);

	charger_info->charger_bms_status_changed = alloc_callback_chain();
	OS_ASSERT(charger_info->charger_bms_status_changed != NULL);

	charger_info->charger_bms_handler = get_charger_bms_handler(channel_config->charger_config.channel_charger_type);

	if((charger_info->charger_bms_handler != NULL) && (charger_info->charger_bms_handler->handle_init != NULL)) {
		OS_ASSERT(charger_info->charger_bms_handler->handle_init(charger_info) == 0);
	}

	return ret;
}
