

/*================================================================
 *
 *
 *   文件名称：channel_record.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分21秒
 *   修改日期：2021年05月23日 星期日 15时00分50秒
 *   描    述：
 *
 *================================================================*/
#include "channel_record.h"

#include "object_class.h"

static object_class_t *channel_record_task_class = NULL;

int channel_record_load(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item, uint16_t index)
{
	int ret = 0;
	return ret;
}

int channel_record_save(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item)
{
	int ret = 0;
	return ret;
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

int channel_record_update(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item)
{
	int ret = 0;
	mutex_lock(channel_record_task_info->mutex);
	do_callback_chain(channel_record_task_info->channel_record_update_chain, channel_record_item);
	channel_record_save(channel_record_task_info, channel_record_item);
	mutex_unlock(channel_record_task_info->mutex);
	return ret;
}

static channel_record_task_info_t *alloc_channel_record_task_info(uint32_t id)
{
	channel_record_task_info_t *channel_record_task_info = NULL;

	channel_record_task_info = (channel_record_task_info_t *)os_calloc(1, sizeof(channel_record_task_info_t));
	OS_ASSERT(channel_record_task_info != NULL);

	channel_record_task_info->id = id;
	//load channel_record_task_info->channel_record_info;

	channel_record_task_info->channel_record_sync_chain = alloc_callback_chain();
	OS_ASSERT(channel_record_task_info->channel_record_sync_chain != NULL);

	channel_record_task_info->channel_record_update_chain = alloc_callback_chain();
	OS_ASSERT(channel_record_task_info->channel_record_update_chain != NULL);

	channel_record_task_info->mutex = mutex_create();
	OS_ASSERT(channel_record_task_info->mutex != NULL);

	channel_record_task_info->sync_signal = signal_create(1);
	OS_ASSERT(channel_record_task_info->sync_signal != NULL);

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
