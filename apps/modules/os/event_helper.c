

/*================================================================
 *
 *
 *   文件名称：event_helper.c
 *   创 建 者：肖飞
 *   创建日期：2020年01月07日 星期二 09时56分01秒
 *   修改日期：2021年02月04日 星期四 11时52分28秒
 *   描    述：
 *
 *================================================================*/
#include "event_helper.h"
#include "os_utils.h"
#include "log.h"

void free_event_pool(event_pool_t *event_pool)
{
	if(event_pool == NULL) {
		return;
	}

	signal_delete(event_pool->queue);

	mutex_lock(event_pool->mutex);

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

	mutex_unlock(event_pool->mutex);

	mutex_delete(event_pool->mutex);

	os_free(event_pool);
}

event_pool_t *alloc_event_pool(void)
{
	event_pool_t *event_pool = NULL;

	event_pool = (event_pool_t *)os_alloc(sizeof(event_pool_t));

	if(event_pool == NULL) {
		return event_pool;
	}

	event_pool->queue = signal_create(1);

	if(event_pool->queue == NULL) {
		goto failed;
	}

	event_pool->mutex = mutex_create();

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

	mutex_lock(event_pool->mutex);

	list_add_tail(&event_item->list_head, &event_pool->list_event);

	mutex_unlock(event_pool->mutex);

	signal_send(event_pool->queue, 0, timeout);

	return ret;
}

int event_pool_wait_event(event_pool_t *event_pool, uint32_t timeout)
{
	int ret = -1;

	if(event_pool == NULL) {
		return ret;
	}

	ret = signal_wait(event_pool->queue, NULL, timeout);

	return ret;
}

void *event_pool_get_event(event_pool_t *event_pool)
{
	event_item_t *event_item = NULL;
	void *event = NULL;

	if(event_pool == NULL) {
		return event;
	}

	mutex_lock(event_pool->mutex);

	if(!list_empty(&event_pool->list_event)) {
		event_item = list_first_entry(&event_pool->list_event, event_item_t, list_head);
		list_del(&event_item->list_head);

		event = event_item->event;
	}

	mutex_unlock(event_pool->mutex);

	if(event_item != NULL) {
		os_free(event_item);
	}

	return event;
}
