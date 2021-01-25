

/*================================================================
 *
 *
 *   文件名称：event_helper.c
 *   创 建 者：肖飞
 *   创建日期：2020年01月07日 星期二 09时56分01秒
 *   修改日期：2021年01月25日 星期一 11时38分58秒
 *   描    述：
 *
 *================================================================*/
#include "event_helper.h"
#include "os_utils.h"
#include "log.h"

void free_event_pool(event_pool_t *event_pool)
{
	osStatus status;

	if(event_pool == NULL) {
		return;
	}

	if(event_pool->queue != NULL) {
		status = osMessageDelete(event_pool->queue);

		if(status != osOK) {
		}
	}

	if(event_pool->mutex != NULL) {
		status = osMutexWait(event_pool->mutex, osWaitForever);

		if(status != osOK) {
		}
	}

	if(!list_empty(&event_pool->list_event)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &event_pool->list_event) {
			event_item_t *event_item = list_entry(pos, event_item_t, list_head);
			list_del(pos);
			os_free(event_item->event);
			os_free(event_item);
		}
	}

	if(event_pool->mutex != NULL) {
		status = osMutexRelease(event_pool->mutex);

		if(status != osOK) {
		}
	}

	if(event_pool->mutex != NULL) {
		status = osMutexDelete(event_pool->mutex);

		if(osOK != status) {
		}
	}

	os_free(event_pool);
}

event_pool_t *alloc_event_pool(void)
{
	event_pool_t *event_pool = NULL;

	osMessageQDef(queue, 1, uint16_t);
	osMutexDef(mutex);

	event_pool = (event_pool_t *)os_alloc(sizeof(event_pool_t));

	if(event_pool == NULL) {
		return event_pool;
	}

	event_pool->queue = osMessageCreate(osMessageQ(queue), NULL);

	if(event_pool->queue == NULL) {
		goto failed;
	}

	event_pool->mutex = osMutexCreate(osMutex(mutex));

	if(event_pool->mutex == NULL) {
		goto failed;
	}

	INIT_LIST_HEAD(&event_pool->list_event);

	return event_pool;

failed:
	free_event_pool(event_pool);
	event_pool = NULL;

	return event_pool;
}

int event_pool_put_event(event_pool_t *event_pool, void *event, uint32_t timeout)
{
	int ret = -1;
	osStatus status;
	event_item_t *event_item = NULL;

	if(event_pool == NULL) {
		return ret;
	}

	if(event == NULL) {
		return ret;
	}

	if(event_pool->queue == NULL) {
		return ret;
	}

	event_item = (event_item_t *)os_alloc(sizeof(event_item_t));

	if(event_item == NULL) {
		return ret;
	}

	ret = 0;

	event_item->event = event;

	status = osMutexWait(event_pool->mutex, osWaitForever);

	if(status != osOK) {
	}

	list_add_tail(&event_item->list_head, &event_pool->list_event);

	status = osMutexRelease(event_pool->mutex);

	if(status != osOK) {
	}

	status = osMessagePut(event_pool->queue, 0, timeout);

	if(status == osOK) {
	}

	return ret;
}

int event_pool_wait_event(event_pool_t *event_pool, uint32_t timeout)
{
	int ret = -1;

	if(event_pool == NULL) {
		return ret;
	}

	osEvent event = osMessageGet(event_pool->queue, timeout);

	if(event.status != osEventTimeout) {
		ret = 0;
	}

	return ret;
}

void *event_pool_get_event(event_pool_t *event_pool)
{
	osStatus status;
	event_item_t *event_item = NULL;
	void *event = NULL;

	if(event_pool == NULL) {
		return event;
	}

	status = osMutexWait(event_pool->mutex, osWaitForever);

	if(status != osOK) {
	}

	if(!list_empty(&event_pool->list_event)) {
		event_item = list_first_entry(&event_pool->list_event, event_item_t, list_head);
		list_del(&event_item->list_head);

		event = event_item->event;
	}

	status = osMutexRelease(event_pool->mutex);

	if(status != osOK) {
	}

	if(event_item != NULL) {
		os_free(event_item);
	}

	return event;
}
