

/*================================================================
 *
 *
 *   文件名称：channels_power_module.c
 *   创 建 者：肖飞
 *   创建日期：2021年03月26日 星期五 17时18分33秒
 *   修改日期：2021年07月02日 星期五 10时36分47秒
 *   描    述：
 *
 *================================================================*/
#include "channels_power_module.h"

#include "log.h"

extern channels_power_module_callback_t channels_power_module_callback_native;

static channels_power_module_callback_t *channels_power_module_callback_sz[] = {
	&channels_power_module_callback_native,
};

static channels_power_module_callback_t *get_channels_power_module_callback(channels_power_module_type_t type)
{
	channels_power_module_callback_t *channels_power_module_callback = NULL;
	int i;

	for(i = 0; i < ARRAY_SIZE(channels_power_module_callback_sz); i++) {
		channels_power_module_callback_t *channels_power_module_callback_item = channels_power_module_callback_sz[i];

		if(channels_power_module_callback_item->type == type) {
			channels_power_module_callback = channels_power_module_callback_item;
		}
	}

	return channels_power_module_callback;
}

power_module_item_info_t *get_power_module_item_info(channels_power_module_t *channels_power_module, uint8_t module_id)
{
	channels_power_module_cmd_ctx_t channels_power_module_cmd_ctx;
	get_power_module_item_info_args_t get_power_module_item_info_args;

	channels_power_module_cmd_ctx.cmd = CHANNELS_POWER_MODULE_CMD_GET_POWER_MODULE_ITEM_INFO;

	get_power_module_item_info_args.module_id = module_id;
	get_power_module_item_info_args.power_module_item_info = NULL;

	channels_power_module_cmd_ctx.args = &get_power_module_item_info_args;

	do_callback_chain(channels_power_module->power_module_cmd_callback_chain, &channels_power_module_cmd_ctx);
	return get_power_module_item_info_args.power_module_item_info;
}

static int channels_power_module_init(channels_power_module_t *channels_power_module)
{
	int ret = 0;
	channels_info_t *channels_info = (channels_info_t *)channels_power_module->channels_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	channels_power_module_type_t type = channels_settings->channels_power_module_settings.channels_power_module_type;

	channels_power_module->power_module_cmd_callback_chain = alloc_callback_chain();
	OS_ASSERT(channels_power_module->power_module_cmd_callback_chain != NULL);

	channels_power_module->channels_power_module_callback = get_channels_power_module_callback(type);

	if((channels_power_module->channels_power_module_callback != NULL) && (channels_power_module->channels_power_module_callback->init != NULL)) {
		channels_power_module->channels_power_module_callback->init(channels_power_module);
	}

	return ret;
}

channels_power_module_t *alloc_channels_power_module(channels_info_t *channels_info)
{
	channels_power_module_t *channels_power_module = os_calloc(1, sizeof(channels_power_module_t));

	OS_ASSERT(channels_power_module != NULL);

	channels_power_module->channels_info = channels_info;

	channels_power_module_init(channels_power_module);

	return channels_power_module;
}
