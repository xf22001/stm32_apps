

/*================================================================
 *
 *
 *   文件名称：charger_bms.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月04日 星期五 16时51分31秒
 *   修改日期：2021年06月04日 星期五 17时42分59秒
 *   描    述：
 *
 *================================================================*/
#include "charger.h"
#include "charger_bms.h"
#include "charger_bms_gb.h"
#include "can_data_task.h"

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

static void charger_periodic(charger_info_t *charger_info)
{
	//channel_info_t *channel_info = (channel_info_t *)charger_info->channel_info;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, charger_info->periodic_stamps) < 50) {
		return;
	}

	charger_info->periodic_stamps = ticks;
	//debug("charger %d state %s periodic", channel_info->channel_id, get_charger_state_des(charger_info->state));
}

static void charger_handle_request(charger_info_t *charger_info)
{
	charger_bms_handler_t *charger_bms_handler = charger_info->charger_bms_handler;
	int ret;

	if(charger_bms_handler->handle_request == NULL) {
		debug("");
		return;
	}

	mutex_lock(charger_info->handle_mutex);

	charger_periodic(charger_info);
	ret = charger_bms_handler->handle_request(charger_info);

	if(ret != 0) {
	}

	mutex_unlock(charger_info->handle_mutex);
}

static void charger_handle_response(charger_info_t *charger_info)
{
	charger_bms_handler_t *charger_bms_handler = charger_info->charger_bms_handler;
	int ret;

	if(charger_bms_handler->handle_response == NULL) {
		debug("");
		return;
	}

	mutex_lock(charger_info->handle_mutex);

	ret = charger_bms_handler->handle_response(charger_info);

	if(ret != 0) {
	}

	mutex_unlock(charger_info->handle_mutex);
}

static void bms_can_data_request(void *fn_ctx, void *chain_ctx)
{
	charger_info_t *charger_info = (charger_info_t *)fn_ctx;

	if(charger_info == NULL) {
		return;
	}

	charger_handle_request(charger_info);
}

static void bms_can_data_response(void *fn_ctx, void *chain_ctx)
{
	charger_info_t *charger_info = (charger_info_t *)fn_ctx;

	if(charger_info == NULL) {
		return;
	}

	charger_handle_response(charger_info);
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
	can_data_task_info_t *bms_can_data_task_info;

	charger_info->handle_mutex = mutex_create();
	OS_ASSERT(charger_info->handle_mutex);

	charger_info->charger_bms_handler = get_charger_bms_handler(channel_config->charger_config.channel_charger_type);
	OS_ASSERT(charger_info->charger_bms_handler != NULL);

	if(charger_info->charger_bms_handler->handle_init != NULL) {
		OS_ASSERT(charger_info->charger_bms_handler->handle_init(charger_info) == 0);
	}

	charger_info->bms_can_info = get_or_alloc_can_info(channel_config->charger_config.hcan_bms);
	OS_ASSERT(charger_info->bms_can_info != NULL);

	bms_can_data_task_info = get_or_alloc_can_data_task_info(channel_config->charger_config.hcan_bms);
	OS_ASSERT(bms_can_data_task_info != NULL);

	charger_info->can_data_request_cb.fn = bms_can_data_request;
	charger_info->can_data_request_cb.fn_ctx = charger_info;
	add_can_data_task_info_request_cb(bms_can_data_task_info, &charger_info->can_data_request_cb);

	charger_info->can_data_response_cb.fn = bms_can_data_response;
	charger_info->can_data_response_cb.fn_ctx = charger_info;
	add_can_data_task_info_response_cb(bms_can_data_task_info, &charger_info->can_data_response_cb);

	return ret;
}
