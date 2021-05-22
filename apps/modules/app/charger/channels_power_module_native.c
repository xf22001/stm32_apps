

/*================================================================
 *
 *
 *   文件名称：channels_power_module_native.c
 *   创 建 者：肖飞
 *   创建日期：2021年03月26日 星期五 15时31分18秒
 *   修改日期：2021年05月11日 星期二 11时37分24秒
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

	callback_item_t periodic_callback_item;
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

	power_modules_info = (power_modules_info_t *)get_or_alloc_power_modules_info(channels_info->channels_config);
	OS_ASSERT(power_modules_info != NULL);
	channels_power_module_ctx->power_modules_info = power_modules_info;

	if(channels_info->channels_settings.power_module_type == POWER_MODULE_TYPE_UNKNOW) {
		channels_info->channels_settings.power_module_type = POWER_MODULE_TYPE_PSEUDO;
	}

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

	channels_power_module_ctx->periodic_callback_item.fn = periodic;
	channels_power_module_ctx->periodic_callback_item.fn_ctx = channels_power_module_ctx;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channels_power_module_ctx->periodic_callback_item) == 0);

	return ret;
}

static int set_output(channel_info_t *channel_info, uint32_t voltage, uint32_t current)
{
	return 0;
}

static power_module_item_info_t *get_power_module_item_info(channels_info_t *channels_info, uint8_t module_id)
{
	channels_power_module_t *channels_power_module = (channels_power_module_t *)channels_info->channels_power_module;
	channels_power_module_ctx_t *channels_power_module_ctx = (channels_power_module_ctx_t *)channels_power_module->ctx;
	power_modules_info_t *power_modules_info = channels_power_module_ctx->power_modules_info;

	if(module_id >= power_modules_info->power_module_number) {
		return NULL;
	}

	return &channels_power_module_ctx->power_module_item_info_ctx[module_id].power_module_item_info;
}

channels_power_module_callback_t channels_power_module_callback_native = {
	.type = CHANNELS_POWER_MODULE_TYPE_NATIVE,
	.init = init,
	.set_output = set_output,
	.get_power_module_item_info = get_power_module_item_info,
};
