

/*================================================================
 *
 *
 *   文件名称：channels.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 09时26分31秒
 *   修改日期：2021年07月04日 星期日 22时10分04秒
 *   描    述：
 *
 *================================================================*/
#include "channels.h"

#include <string.h>
#include <stdlib.h>

#include "object_class.h"
#include "os_utils.h"

#include "channel.h"
#include "channels_power_module.h"
#include "card_reader.h"
#include "eeprom_layout.h"

#include "log.h"

static object_class_t *channels_class = NULL;

char *get_channel_event_type_des(channel_event_type_t type)
{
	char *des = "unknow";

	switch(type) {
			add_des_case(CHANNEL_EVENT_TYPE_UNKNOW);
			add_des_case(CHANNEL_EVENT_TYPE_START_CHANNEL);
			add_des_case(CHANNEL_EVENT_TYPE_STOP_CHANNEL);

		default: {
		}
		break;
	}

	return des;
}

int set_fault(bitmap_t *faults, int fault, uint8_t v)
{
	return set_bitmap_value(faults, fault, v);
}

int get_fault(bitmap_t *faults, int fault)
{
	return get_bitmap_value(faults, fault);
}

int get_first_fault(bitmap_t *faults)
{
	int ret = get_first_value_index(faults, 1);

	if(ret >= faults->size) {
		ret = -1;
	}

	return ret;
}

static void free_channels_info(channels_info_t *channels_info)
{
	app_panic();
}

static void channels_info_process_event(channels_info_t *channels_info)
{
	channels_event_t *channels_event = NULL;
	int ret = -1;

	if(channels_info == NULL) {
		return;
	}

	ret = event_pool_wait_event(channels_info->event_pool, CHANNEL_TASK_PERIODIC);

	if(ret != 0) {
		return;
	}

	for(channels_event = event_pool_get_event(channels_info->event_pool);
	    channels_event != NULL;
	    channels_event = event_pool_get_event(channels_info->event_pool)) {
		do_callback_chain(channels_info->common_event_chain, channels_event);
		os_free(channels_event);
	}
}

static void channels_info_handle_common_periodic(channels_info_t *channels_info)
{
	do_callback_chain(channels_info->common_periodic_chain, channels_info);
}

static void channels_periodic(channels_info_t *channels_info)
{
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, channels_info->periodic_stamp) >= CHANNEL_TASK_PERIODIC) {
		channels_info->periodic_stamp = ticks;
		channels_info_handle_common_periodic(channels_info);
	}
}

static void task_channels(void const *argument)
{
	channels_info_t *channels_info = (channels_info_t *)argument;

	OS_ASSERT(channels_info != NULL);

	while(1) {
		if(channels_info->configed == 0) {
			osDelay(10);
			continue;
		}

		channels_info_process_event(channels_info);

		//处理周期性事件
		channels_periodic(channels_info);
	}
}

int send_channels_event(channels_info_t *channels_info, channels_event_t *channels_event, uint32_t timeout)
{
	return event_pool_put_event(channels_info->event_pool, channels_event, timeout);
}

static int channels_info_set_channels_config(channels_info_t *channels_info, channels_config_t *channels_config)
{
	int ret = 0;
	debug("use channels channels_config %d!", channels_config->id);

	channels_info->event_pool = alloc_event_pool();

	OS_ASSERT(channels_info->event_pool != NULL);

	channels_info->channel_info = alloc_channels_channel_info(channels_info);

	channels_info->channels_power_module = alloc_channels_power_module(channels_info);

	channels_info->card_reader_info = alloc_card_reader_info(channels_info);

	osThreadDef(channels, task_channels, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(channels), channels_info);

	channels_info->configed = 1;

	return ret;
}

static int channels_info_load_config(channels_info_t *channels_info)
{
	eeprom_layout_t *eeprom_layout = get_eeprom_layout();
	size_t offset = (size_t)&eeprom_layout->channels_settings_seg.settings.eeprom_channels_settings.channels_settings;
	debug("offset:%d", offset);
	return eeprom_load_config_item(channels_info->eeprom_info, "channels_info->channels_settings", &channels_info->channels_settings, sizeof(channels_settings_t), offset);
}

static int channels_info_save_config(channels_info_t *channels_info)
{
	eeprom_layout_t *eeprom_layout = get_eeprom_layout();
	size_t offset = (size_t)&eeprom_layout->channels_settings_seg.settings.eeprom_channels_settings.channels_settings;
	debug("offset:%d", offset);
	return eeprom_save_config_item(channels_info->eeprom_info, "channels_info->channels_settings", &channels_info->channels_settings, sizeof(channels_settings_t), offset);
}

static void channels_info_reset_default_config(channels_info_t *channels_info)
{
	channels_config_t *channels_config = channels_info->channels_config;
	channels_settings_t *channels_settings = &channels_info->channels_settings;

	memset(channels_settings, 0, sizeof(channels_settings_t));

	channels_settings->channel_number = channels_config->channel_number;

	channels_settings->channels_power_module_settings.channels_power_module_number = channels_config->power_module_config.channels_power_module_number;
	channels_settings->channels_power_module_settings.channels_power_module_type = channels_config->power_module_config.channels_power_module_type;
	channels_settings->card_reader_settings.type = channels_config->card_reader_config.card_reader_type;
}

static int channels_info_init_config(channels_info_t *channels_info)
{
	int ret = 0;

	if(channels_info_load_config(channels_info) == 0) {
		debug("channels load config successfully!");
	} else {
		debug("channels load config failed!");
		channels_info_reset_default_config(channels_info);
		ret = channels_info_save_config(channels_info);
	}

	return ret;
}

static channels_info_t *alloc_channels_info(channels_config_t *channels_config)
{
	channels_info_t *channels_info = NULL;

	OS_ASSERT(channels_config != NULL);

	channels_info = (channels_info_t *)os_calloc(1, sizeof(channels_info_t));

	OS_ASSERT(channels_info != NULL);

	channels_info->channels_config = channels_config;
	channels_info->eeprom_info = get_or_alloc_eeprom_info(get_or_alloc_spi_info(channels_config->channels_eeprom_config.hspi_eeprom),
	                             channels_config->channels_eeprom_config.eeprom_cs_port,
	                             channels_config->channels_eeprom_config.eeprom_cs_pin,
	                             NULL,
	                             0);
	OS_ASSERT(channels_info->eeprom_info != NULL);

	OS_ASSERT(channels_info_init_config(channels_info) == 0);

	channels_info->common_periodic_chain = alloc_callback_chain();
	OS_ASSERT(channels_info->common_periodic_chain != NULL);
	channels_info->common_event_chain = alloc_callback_chain();
	OS_ASSERT(channels_info->common_event_chain != NULL);

	channels_info->faults = alloc_bitmap(CHANNELS_FAULT_SIZE);
	OS_ASSERT(channels_info->faults != NULL);

	OS_ASSERT(channels_info_set_channels_config(channels_info, channels_config) == 0);

	return channels_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	channels_info_t *channels_info = (channels_info_t *)o;
	channels_config_t *channels_config = (channels_config_t *)ctx;

	if(channels_info->channels_config == channels_config) {
		ret = 0;
	}

	return ret;
}

static channels_info_t *get_or_alloc_channels_info(channels_config_t *channels_config)
{
	channels_info_t *channels_info = NULL;

	os_enter_critical();

	if(channels_class == NULL) {
		channels_class = object_class_alloc();
	}

	os_leave_critical();

	channels_info = (channels_info_t *)object_class_get_or_alloc_object(channels_class, object_filter, channels_config, (object_alloc_t)alloc_channels_info, (object_free_t)free_channels_info);

	return channels_info;
}

static channels_info_t *_channels_info = NULL;

channels_info_t *start_channels(void)
{
	channels_config_t *channels_config = get_channels_config(0);

	OS_ASSERT(channels_config != NULL);

	_channels_info = get_or_alloc_channels_info(channels_config);
	OS_ASSERT(_channels_info != NULL);

	return _channels_info;
}

channels_info_t *get_channels(void)
{
	return _channels_info;
}
