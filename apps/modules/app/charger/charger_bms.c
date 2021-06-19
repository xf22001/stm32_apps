

/*================================================================
 *
 *
 *   文件名称：charger_bms.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月04日 星期五 16时51分31秒
 *   修改日期：2021年06月19日 星期六 23时09分39秒
 *   描    述：
 *
 *================================================================*/
#include "charger_bms.h"
#include "charger_bms_gb.h"
#include "charger_bms_ac.h"

#include "log.h"

static charger_bms_handler_t *charger_bms_handler_sz[] = {
	&charger_bms_handler_gb,
	&charger_bms_handler_ac,
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

static char *get_charger_bms_work_state_des(charger_bms_work_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHARGER_BMS_WORK_STATE_IDLE);
			add_des_case(CHARGER_BMS_WORK_STATE_STARTING);
			add_des_case(CHARGER_BMS_WORK_STATE_RUNNING);
			add_des_case(CHARGER_BMS_WORK_STATE_STOPPING);

		default: {
		}
		break;
	}

	return des;
}

int set_charger_bms_work_state(charger_info_t *charger_info, charger_bms_work_state_t state)
{
	int ret = 0;

	channel_info_t *channel_info = charger_info->channel_info;

	debug("charger %d bms work state %s -> %s", channel_info->channel_id, get_charger_bms_work_state_des(charger_info->charger_bms_work_state), get_charger_bms_work_state_des(state));
	charger_info->charger_bms_work_state = state;

	return ret;
}

static char *get_charger_bms_request_action_des(charger_bms_request_action_t action)
{
	char *des = "unknow";

	switch(action) {
			add_des_case(CHARGER_BMS_REQUEST_ACTION_NONE);
			add_des_case(CHARGER_BMS_REQUEST_ACTION_START);
			add_des_case(CHARGER_BMS_REQUEST_ACTION_STOP);

		default: {
		}
		break;
	}

	return des;
}

int set_charger_bms_request_action(charger_info_t *charger_info, charger_bms_request_action_t action)
{
	int ret = -1;

	if(charger_info->charger_bms_request_action == CHARGER_BMS_REQUEST_ACTION_NONE) {
		ret = 0;
	}

	debug("charger bms request action %s -> %s", get_charger_bms_request_action_des(charger_info->charger_bms_request_action), get_charger_bms_request_action_des(action));
	charger_info->charger_bms_request_action = action;

	return ret;
}

void set_charger_bms_request_state(charger_info_t *charger_info, uint8_t request_state)
{
	charger_info->request_state = request_state;
}

int charger_bms_init(charger_info_t *charger_info)
{
	int ret = 0;
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
