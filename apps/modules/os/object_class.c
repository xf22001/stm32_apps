

/*================================================================
 *
 *
 *   文件名称：object_class.c
 *   创 建 者：肖飞
 *   创建日期：2021年02月02日 星期二 08时56分26秒
 *   修改日期：2021年03月26日 星期五 10时40分34秒
 *   描    述：
 *
 *================================================================*/
#include "object_class.h"

void object_class_free(object_class_t *object_class)
{
	if(object_class == NULL) {
		return;
	}

	os_enter_critical();

	mutex_lock(object_class->mutex);

	if(!list_empty(&object_class->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &object_class->list) {
			object_class_item_t *object_class_item = list_entry(pos, object_class_item_t, list);
			list_del(pos);

			if(object_class_item != NULL) {
				if((object_class_item->o != NULL) && (object_class_item->free != NULL)) {
					object_class_item->free(object_class_item->o);
				}

				os_free(object_class_item);
			}
		}
	}

	mutex_unlock(object_class->mutex);

	os_leave_critical();

	mutex_delete(object_class->mutex);

	os_free(object_class);
}

object_class_t *object_class_alloc(void)
{
	object_class_t *object_class = NULL;

	if(__get_IPSR() != 0) {
		return object_class;
	}

	object_class = (object_class_t *)os_alloc(sizeof(object_class_t));

	if(object_class == NULL) {
		return object_class;
	}

	object_class->mutex = mutex_create();

	if(object_class->mutex == NULL) {
		goto failed;
	}

	INIT_LIST_HEAD(&object_class->list);

	return object_class;

failed:
	object_class_free(object_class);
	object_class = NULL;

	return object_class;
}

int object_class_add_object(object_class_t *object_class, object_filter_t filter, void *ctx, void *o, object_free_t object_free)
{
	int ret = -1;
	object_class_item_t *object_class_item = NULL;
	uint8_t found = 0;

	if(object_class == NULL) {
		return ret;
	}

	if(filter == NULL) {
		return ret;
	}

	if(o == NULL) {
		return ret;
	}

	object_class_item = (object_class_item_t *)os_alloc(sizeof(object_class_item_t));

	if(object_class_item == NULL) {
		return ret;
	}

	os_enter_critical();

	mutex_lock(object_class->mutex);

	if(!list_empty(&object_class->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &object_class->list) {
			object_class_item_t *entry = list_entry(pos, object_class_item_t, list);

			if(filter(entry->o, ctx) == 0) {
				found = 1;
			}
		}
	}

	if(found == 0) {
		object_class_item->o = o;
		object_class_item->free = object_free;
		list_add_tail(&object_class_item->list, &object_class->list);
		ret = 0;
	}

	mutex_unlock(object_class->mutex);

	os_leave_critical();

	if(ret != 0) {
		os_free(object_class_item);
	}

	return ret;
}

void *object_class_get_object(object_class_t *object_class, object_filter_t filter, void *ctx)
{
	void *o = NULL;

	if(object_class == NULL) {
		return o;
	}

	if(filter == NULL) {
		return o;
	}

	os_enter_critical();

	mutex_lock(object_class->mutex);

	if(!list_empty(&object_class->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &object_class->list) {
			object_class_item_t *entry = list_entry(pos, object_class_item_t, list);

			if(filter(entry->o, ctx) == 0) {
				o = entry->o;
				break;
			}
		}
	}

	mutex_unlock(object_class->mutex);

	os_leave_critical();

	return o;
}

int object_class_get_objects(object_class_t *object_class, object_filter_t filter, void *ctx, void **po, size_t *size)
{
	int ret = -1;
	size_t index = 0;

	if(object_class == NULL) {
		return ret;
	}

	if(filter == NULL) {
		return ret;
	}

	if(po == NULL) {
		return ret;
	}

	if(size == NULL) {
		return ret;
	}

	os_enter_critical();

	mutex_lock(object_class->mutex);

	if(!list_empty(&object_class->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &object_class->list) {
			object_class_item_t *entry = list_entry(pos, object_class_item_t, list);

			if(index >= *size) {
				break;
			}

			if(filter(entry->o, ctx) == 0) {
				po[index++] = entry->o;
				ret = 0;
			}
		}
	}

	*size = index;

	mutex_unlock(object_class->mutex);

	os_leave_critical();

	return ret;
}

int object_class_remove_object(object_class_t *object_class, void *o)
{
	int ret = -1;

	if(object_class == NULL) {
		return ret;
	}

	if(o == NULL) {
		return ret;
	}

	os_enter_critical();

	mutex_lock(object_class->mutex);

	if(!list_empty(&object_class->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &object_class->list) {
			object_class_item_t *object_class_item = list_entry(pos, object_class_item_t, list);

			if(object_class_item->o == o) {
				list_del(pos);

				if(object_class_item != NULL) {
					if((object_class_item->o != NULL) && (object_class_item->free != NULL)) {
						object_class_item->free(object_class_item->o);
					}

					os_free(object_class_item);
					ret = 0;
				}
			}
		}
	}

	mutex_unlock(object_class->mutex);

	os_leave_critical();

	return ret;
}

size_t object_class_size(object_class_t *object_class)
{
	size_t size = 0;

	if(object_class == NULL) {
		return size;
	}

	os_enter_critical();

	mutex_lock(object_class->mutex);

	if(!list_empty(&object_class->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &object_class->list) {
			size++;
		}
	}

	mutex_unlock(object_class->mutex);

	os_leave_critical();

	return size;
}

void *object_class_get_or_alloc_object(object_class_t *object_class, object_filter_t filter, void *ctx, object_alloc_t object_alloc, object_free_t object_free)
{
	void *o = NULL;
	int ret;

	if(object_class == NULL) {
		return o;
	}

	if(filter == NULL) {
		return o;
	}

	if(object_alloc == NULL) {
		return o;
	}

	if(object_free == NULL) {
		return o;
	}

	o = object_class_get_object(object_class, filter, ctx);

	if(o != NULL) {
		return o;
	}

	if(__get_IPSR() != 0) {
		return o;
	}

	o = object_alloc(ctx);

	if(o == NULL) {
		return o;
	}

	ret = object_class_add_object(object_class, filter, ctx, o, object_free);

	if(ret != 0) {
		object_free(o);
		o = NULL;
	}

	return o;
}
