

/*================================================================
 *
 *
 *   文件名称：channels_power_module_native.c
 *   创 建 者：肖飞
 *   创建日期：2021年03月26日 星期五 15时31分18秒
 *   修改日期：2021年07月01日 星期四 16时31分40秒
 *   描    述：
 *
 *================================================================*/
#include "channels_power_module.h"

#include "power_modules.h"

#include "log.h"

typedef struct {
	struct list_head list;
	power_module_item_info_t power_module_item_info;

	void *power_modules_info;
	void *channel_info;

	uint32_t query_stamp;
	uint32_t setting_stamp;
	uint32_t test_stamp;
} power_module_item_info_ctx_t;

typedef struct {
	struct list_head power_module_idle_list;

	power_modules_info_t *power_modules_info;
	power_module_item_info_ctx_t *power_module_item_info_ctx;

	uint32_t periodic_stamp;
} channels_power_module_ctx_t;


#define POWER_MODULES_QUERY_PERIODIC (1000)

static void power_modules_periodic(channels_power_module_ctx_t *channels_power_module_ctx)
{
	int i;
	uint8_t do_init = 1;
	power_modules_info_t *power_modules_info = channels_power_module_ctx->power_modules_info;

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + i;
		uint8_t state = get_power_module_connect_state(power_module_info);

		if(state != 0) {
			do_init = 0;
		}
	}

	if(do_init == 1) {
		power_modules_init(power_modules_info);
		return;
	}

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_modules_query_status(power_modules_info, i);
	}
}

static void periodic(void *_channels_power_module_ctx, void *_channels_info)
{
	channels_power_module_ctx_t *channels_power_module_ctx = (channels_power_module_ctx_t *)_channels_power_module_ctx;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, channels_power_module_ctx->periodic_stamp) < POWER_MODULES_QUERY_PERIODIC) {
		return;
	}

	channels_power_module_ctx->periodic_stamp = ticks;

	//debug("");
	power_modules_periodic(channels_power_module_ctx);
}

static void power_module_item_info_cb(void *_channels_power_module_ctx, void *_power_module_item_info_args)
{
	channels_power_module_ctx_t *channels_power_module_ctx = (channels_power_module_ctx_t *)_channels_power_module_ctx;
	power_modules_info_t *power_modules_info = channels_power_module_ctx->power_modules_info;
	power_module_item_info_args_t *power_module_item_info_args = (power_module_item_info_args_t *)_power_module_item_info_args;
	uint8_t module_id = power_module_item_info_args->module_id;

	if(module_id >= power_modules_info->power_module_number) {
		power_module_item_info_args->power_module_item_info = NULL;
		return;
	}

	power_module_item_info_args->power_module_item_info = &channels_power_module_ctx->power_module_item_info_ctx[module_id].power_module_item_info;
}

static int init(void *_channels_power_module)
{
	int ret = 0;
	channels_power_module_t *channels_power_module = (channels_power_module_t *)_channels_power_module;
	channels_info_t *channels_info = channels_power_module->channels_info;
	power_modules_info_t *power_modules_info;
	power_module_item_info_ctx_t *power_module_item_info_ctx;
	channels_power_module_ctx_t *channels_power_module_ctx;
	int i;

	channels_power_module_ctx = (channels_power_module_ctx_t *)os_calloc(1, sizeof(channels_power_module_ctx_t));
	OS_ASSERT(channels_power_module_ctx != NULL);
	INIT_LIST_HEAD(&channels_power_module_ctx->power_module_idle_list);
	channels_power_module->ctx = channels_power_module_ctx;

	channels_info->channels_config->power_module_config.channels_power_module_number = channels_info->channels_settings.channels_power_module_settings.channels_power_module_number;

	power_modules_info = (power_modules_info_t *)get_or_alloc_power_modules_info(channels_info->channels_config);
	OS_ASSERT(power_modules_info != NULL);
	channels_power_module_ctx->power_modules_info = power_modules_info;

	power_modules_set_type(power_modules_info, channels_info->channels_settings.power_module_type);

	power_module_item_info_ctx = (power_module_item_info_ctx_t *)os_calloc(power_modules_info->power_module_number, sizeof(power_module_item_info_ctx_t));
	OS_ASSERT(power_module_item_info_ctx != NULL);

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_item_info_ctx_t *info = power_module_item_info_ctx + i;

		info->power_module_item_info.module_id = i;
		info->power_modules_info = power_modules_info;
		list_add(&info->list, &channels_power_module_ctx->power_module_idle_list);
	}

	channels_power_module_ctx->power_module_item_info_ctx = power_module_item_info_ctx;

	remove_callback(channels_info->common_periodic_chain, &channels_power_module->periodic_callback_item);
	channels_power_module->periodic_callback_item.fn = periodic;
	channels_power_module->periodic_callback_item.fn_ctx = channels_power_module_ctx;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channels_power_module->periodic_callback_item) == 0);

	remove_callback(channels_power_module->power_module_item_info_callback_chain, &channels_power_module->power_module_item_info_callback_item);
	channels_power_module->power_module_item_info_callback_item.fn = power_module_item_info_cb;
	channels_power_module->power_module_item_info_callback_item.fn_ctx = channels_power_module_ctx;
	OS_ASSERT(register_callback(channels_power_module->power_module_item_info_callback_chain, &channels_power_module->power_module_item_info_callback_item) == 0);

	return ret;
}

channels_power_module_callback_t channels_power_module_callback_native = {
	.type = CHANNELS_POWER_MODULE_TYPE_NATIVE,
	.init = init,
};
