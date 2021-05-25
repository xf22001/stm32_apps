

/*================================================================
 *
 *
 *   文件名称：channel_record.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分21秒
 *   修改日期：2021年05月25日 星期二 09时44分01秒
 *   描    述：
 *
 *================================================================*/
#include "channel_record.h"

#include "object_class.h"
#include "eeprom_layout.h"
#include "app.h"
#include "log.h"

static object_class_t *channel_record_task_class = NULL;

static int channel_record_info_load(channel_record_task_info_t *channel_record_task_info, channel_record_info_t *channel_record_info)
{
	eeprom_layout_t *eeprom_layout = get_eeprom_layout();
	size_t offset = (size_t)&eeprom_layout->channel_record_seg.channel_record.eeprom_channel_record_info.channel_record_info;
	debug("offset:%d", offset);
	return eeprom_load_config_item(channel_record_task_info->eeprom_info, "record_info", channel_record_info, sizeof(channel_record_info_t), offset);
}

static int channel_record_info_save(channel_record_task_info_t *channel_record_task_info, channel_record_info_t *channel_record_info)
{
	eeprom_layout_t *eeprom_layout = get_eeprom_layout();
	size_t offset = (size_t)&eeprom_layout->channel_record_seg.channel_record.eeprom_channel_record_info.channel_record_info;
	debug("offset:%d", offset);
	return eeprom_save_config_item(channel_record_task_info->eeprom_info, "record_info", &channel_record_info, sizeof(channel_record_info_t), offset);
}

static int channel_record_item_load(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item, uint16_t id)
{
	eeprom_layout_t *eeprom_layout = get_eeprom_layout();
	size_t offset = (size_t)&eeprom_layout->channel_record_seg.channel_record.eeprom_channel_record_item[id].channel_record_item;
	debug("offset:%d", offset);
	return eeprom_load_config_item(channel_record_task_info->eeprom_info, "record_item", channel_record_item, sizeof(channel_record_item_t), offset);
}

static int channel_record_item_save(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item)
{
	eeprom_layout_t *eeprom_layout = get_eeprom_layout();
	uint16_t id = channel_record_item->id;
	size_t offset = (size_t)&eeprom_layout->channel_record_seg.channel_record.eeprom_channel_record_item[id].channel_record_item;
	debug("offset:%d", offset);
	return eeprom_save_config_item(channel_record_task_info->eeprom_info, "record_item", channel_record_item, sizeof(channel_record_item_t), offset);
}

uint16_t alloc_channel_record_item_id(channel_record_task_info_t *channel_record_task_info)
{
	uint16_t id = -1;
	channel_record_info_t *channel_record_info = &channel_record_task_info->channel_record_info;
	channel_record_item_t *channel_record_item = (channel_record_item_t *)os_calloc(1, sizeof(channel_record_item_t));

	OS_ASSERT(channel_record_item != NULL);

	channel_record_item->state = CHANNEL_RECORD_ITEM_STATE_INIT;

	mutex_lock(channel_record_task_info->mutex);

	id = channel_record_info->end;

	channel_record_info->end += 1;

	if(channel_record_info->end >= CHANNEL_RECORD_NUMBER) {
		channel_record_info->end = 0;
	}

	if(channel_record_info->start == channel_record_info->end) {
		channel_record_info->start += 1;
	}

	if(channel_record_info->start >= CHANNEL_RECORD_NUMBER) {
		channel_record_info->start = 0;
	}

	channel_record_item->id = id;

	OS_ASSERT(channel_record_info_save(channel_record_task_info, channel_record_info));
	OS_ASSERT(channel_record_item_save(channel_record_task_info, channel_record_item));

	mutex_unlock(channel_record_task_info->mutex);

	os_free(channel_record_item);

	return id;
}

channel_record_item_t *get_channel_record_item_by_id(channel_record_task_info_t *channel_record_task_info, uint16_t id)
{
	channel_record_item_t *channel_record_item = (channel_record_item_t *)os_calloc(1, sizeof(channel_record_item_t));

	if(channel_record_item == NULL) {
		return channel_record_item;
	}

	if(id >= CHANNEL_RECORD_NUMBER) {
		return channel_record_item;
	}

	mutex_lock(channel_record_task_info->mutex);

	if(channel_record_item_load(channel_record_task_info, channel_record_item, id) != 0) {
		os_free(channel_record_item);
		channel_record_item = NULL;
	}

	mutex_unlock(channel_record_task_info->mutex);

	return channel_record_item;
}

int channel_record_update(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item)
{
	int ret = -1;

	if(channel_record_item == NULL) {
		return ret;
	}

	if(channel_record_item->id >= CHANNEL_RECORD_NUMBER) {
		return ret;
	}

	mutex_lock(channel_record_task_info->mutex);
	do_callback_chain(channel_record_task_info->channel_record_update_chain, channel_record_item);
	ret = channel_record_item_save(channel_record_task_info, channel_record_item);
	mutex_unlock(channel_record_task_info->mutex);
	return ret;
}

int channel_record_sync(channel_record_task_info_t *channel_record_task_info)
{
	return signal_send(channel_record_task_info->sync_signal, 0, 10);
}

static void free_channel_record_task_info(channel_record_task_info_t *channel_record_task_info)
{
	app_panic();
}

static void channel_record_task(void const *argument)
{
	channel_record_task_info_t *channel_record_task_info = (channel_record_task_info_t *)argument;

	if(channel_record_task_info == NULL) {
		app_panic();
	}

	for(;;) {
		uint32_t event;
		int ret = signal_wait(channel_record_task_info->sync_signal, &event, 60 * 1000);

		if(ret == 0) {
			switch(event) {
				default: {
				}
				break;
			}
		}

		mutex_lock(channel_record_task_info->mutex);
		do_callback_chain(channel_record_task_info->channel_record_sync_chain, channel_record_task_info);
		mutex_unlock(channel_record_task_info->mutex);
	}
}

static channel_record_task_info_t *alloc_channel_record_task_info(uint32_t id)
{
	channel_record_task_info_t *channel_record_task_info = NULL;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);
	OS_ASSERT(app_info->eeprom_info != NULL);

	channel_record_task_info = (channel_record_task_info_t *)os_calloc(1, sizeof(channel_record_task_info_t));
	OS_ASSERT(channel_record_task_info != NULL);

	channel_record_task_info->id = id;

	channel_record_task_info->eeprom_info = app_info->eeprom_info;

	channel_record_task_info->channel_record_sync_chain = alloc_callback_chain();
	OS_ASSERT(channel_record_task_info->channel_record_sync_chain != NULL);

	channel_record_task_info->channel_record_update_chain = alloc_callback_chain();
	OS_ASSERT(channel_record_task_info->channel_record_update_chain != NULL);

	channel_record_task_info->mutex = mutex_create();
	OS_ASSERT(channel_record_task_info->mutex != NULL);

	channel_record_task_info->sync_signal = signal_create(1);
	OS_ASSERT(channel_record_task_info->sync_signal != NULL);

	mutex_lock(channel_record_task_info->mutex);

	if(channel_record_info_load(channel_record_task_info, &channel_record_task_info->channel_record_info) != 0) {
		channel_record_task_info->channel_record_info.start = 0;
		channel_record_task_info->channel_record_info.end = 0;
		OS_ASSERT(channel_record_info_save(channel_record_task_info, &channel_record_task_info->channel_record_info) == 0);
	}

	mutex_unlock(channel_record_task_info->mutex);

	osThreadDef(channel_record_task, channel_record_task, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(channel_record_task), channel_record_task_info);

	return channel_record_task_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	channel_record_task_info_t *channel_record_task_info = (channel_record_task_info_t *)o;
	uint32_t id = (uint32_t)ctx;

	if(channel_record_task_info->id == id) {
		ret = 0;
	}

	return ret;
}

channel_record_task_info_t *get_or_alloc_channel_record_task_info(uint32_t id)
{
	channel_record_task_info_t *channel_record_task_info = NULL;

	os_enter_critical();

	if(channel_record_task_class == NULL) {
		channel_record_task_class = object_class_alloc();
	}

	os_leave_critical();

	channel_record_task_info = (channel_record_task_info_t *)object_class_get_or_alloc_object(channel_record_task_class, object_filter, (void *)id, (object_alloc_t)alloc_channel_record_task_info, (object_free_t)free_channel_record_task_info);

	return channel_record_task_info;
}
