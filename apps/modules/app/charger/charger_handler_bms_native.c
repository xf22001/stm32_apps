

/*================================================================
 *   
 *   
 *   文件名称：charger_handler_bms_native.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月11日 星期二 11时18分08秒
 *   修改日期：2021年05月11日 星期二 11时18分48秒
 *   描    述：
 *
 *================================================================*/
#include "charger_handler_bms_native.h"

#include "bms_spec.h"
#include "os_utils.h"
#include "can_data_task.h"
#include "charger_bms.h"

static void charger_periodic(charger_info_t *charger_info)
{
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, charger_info->periodic_stamps) >= 50) {
		//channel_info_t *channel_info = (channel_info_t *)charger_info->channel_info;
		charger_info->periodic_stamps = ticks;
		//debug("charger %d state %s periodic", channel_info->channel_id, get_charger_state_des(charger_info->state));
	}
}

static int charger_handle_request(charger_info_t *charger_info)
{
	//channel_info_t *channel_info = (channel_info_t *)charger_info->channel_info;
	int ret = -1;
	uint8_t change_state = 0;

	charger_periodic(charger_info);

	if(charger_info->bms_state_request != 0) {
		charger_info->bms_state = charger_info->bms_state_request;
		charger_info->bms_state_request = 0;
	}

	if(charger_info->charger_bms_state_handler_cache == NULL) {
		change_state = 1;
	} else if(charger_info->charger_bms_state_handler_cache->bms_state != charger_info->bms_state) {
		change_state = 1;
	}

	if(change_state == 1) {
		charger_bms_handler_t *charger_bms_handler = charger_info->charger_bms_handler;
		charger_info->charger_bms_state_handler_cache = charger_bms_handler->get_charger_bms_state_handler(charger_info->bms_state);
	}

	if(charger_info->charger_bms_state_handler_cache == NULL) {
		return ret;
	}

	if(change_state == 1) {
		charger_info->charger_bms_state_handler_cache->prepare(charger_info);
	}

	charger_info->charger_bms_state_handler_cache->handle_request(charger_info);

	return ret;
}

static int charger_handle_response(charger_info_t *charger_info)
{
	int ret = -1;

	if(charger_info->charger_bms_state_handler_cache == NULL) {
		return ret;
	}

	ret = charger_info->charger_bms_state_handler_cache->handle_response(charger_info);

	return ret;
}

static void bms_can_data_request(void *fn_ctx, void *chain_ctx)
{
	charger_info_t *charger_info = (charger_info_t *)fn_ctx;

	if(fn_ctx == NULL) {
		return;
	}

	charger_handle_request(charger_info);
}

static void bms_can_data_response(void *fn_ctx, void *chain_ctx)
{
	charger_info_t *charger_info = (charger_info_t *)fn_ctx;

	if(fn_ctx == NULL) {
		return;
	}

	charger_handle_response(charger_info);
}

static int init(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = (channel_info_t *)charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;
	can_data_task_info_t *bms_can_data_task_info;

	charger_info->charger_bms_handler = get_charger_bms_handler(channel_config->charger_config.charger_bms_type);

	OS_ASSERT(charger_info->charger_bms_handler != NULL);

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

charger_handler_t charger_handler_bms_native = {
	.charger_type = CHANNEL_CHARGER_TYPE_BMS_NATIVE,
	.init = init,
};
