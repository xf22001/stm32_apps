

/*================================================================
 *
 *
 *   文件名称：map_utils.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 11时40分50秒
 *   修改日期：2021年02月01日 星期一 15时52分29秒
 *   描    述：
 *
 *================================================================*/
#include "map_utils.h"

#include "os_utils.h"
#include "log.h"

void map_utils_free(map_utils_t *map_utils)
{

	if(map_utils == NULL) {
		return;
	}

	__disable_irq();

	mutex_lock(map_utils->mutex);

	if(!list_empty(&map_utils->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &map_utils->list) {
			map_utils_item_t *map_utils_item = list_entry(pos, map_utils_item_t, list);
			list_del(pos);
			os_free(map_utils_item);
		}
	}

	mutex_unlock(map_utils->mutex);

	__enable_irq();

	mutex_delete(map_utils->mutex);

	os_free(map_utils);
}

static int map_utils_default_match(void *key1, void *key2)
{
	int ret = -1;

	if(key1 == key2) {
		ret = 0;
	}

	return ret;
}

map_utils_t *map_utils_alloc(key_match_t match)
{
	map_utils_t *map_utils = NULL;

	if(__get_IPSR() != 0) {
		return map_utils;
	}

	map_utils = (map_utils_t *)os_alloc(sizeof(map_utils_t));

	if(map_utils == NULL) {
		return map_utils;
	}

	map_utils->mutex = mutex_create();

	if(map_utils->mutex == NULL) {
		goto failed;
	}

	INIT_LIST_HEAD(&map_utils->list);

	if(match == NULL) {
		match = map_utils_default_match;
	}

	map_utils->match = match;

	return map_utils;

failed:
	map_utils_free(map_utils);
	map_utils = NULL;

	return map_utils;
}

int map_utils_add_key_value(map_utils_t *map_utils, void *key, void *value)
{
	int ret = -1;
	map_utils_item_t *map_utils_item = NULL;
	uint8_t found = 0;

	if(map_utils == NULL) {
		return ret;
	}

	map_utils_item = (map_utils_item_t *)os_alloc(sizeof(map_utils_item_t));

	if(map_utils_item == NULL) {
		return ret;
	}

	__disable_irq();

	mutex_lock(map_utils->mutex);

	if(!list_empty(&map_utils->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &map_utils->list) {
			map_utils_item_t *item = list_entry(pos, map_utils_item_t, list);

			if(map_utils->match(item->key, key) == 0) {
				found = 1;
			}
		}
	}

	if(found == 0) {
		map_utils_item->key = key;
		map_utils_item->value = value;

		list_add_tail(&map_utils_item->list, &map_utils->list);
		ret = 0;
	}

	mutex_unlock(map_utils->mutex);

	__enable_irq();

	if(ret != 0) {
		os_free(map_utils_item);
	}

	return ret;
}

void *map_utils_get_value(map_utils_t *map_utils, void *key)
{
	void *value = NULL;

	if(map_utils == NULL) {
		return value;
	}

	__disable_irq();

	mutex_lock(map_utils->mutex);

	if(!list_empty(&map_utils->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &map_utils->list) {
			map_utils_item_t *map_utils_item = list_entry(pos, map_utils_item_t, list);

			if(map_utils->match(map_utils_item->key, key) == 0) {
				value = map_utils_item->value;
				break;
			}
		}
	}

	mutex_unlock(map_utils->mutex);

	__enable_irq();

	return value;
}

int map_utils_remove_value(map_utils_t *map_utils, void *key)
{
	int ret = -1;

	if(map_utils == NULL) {
		return ret;
	}

	__disable_irq();

	mutex_lock(map_utils->mutex);

	if(!list_empty(&map_utils->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &map_utils->list) {
			map_utils_item_t *map_utils_item = list_entry(pos, map_utils_item_t, list);

			if(map_utils->match(map_utils_item->key, key) == 0) {
				list_del(pos);
				ret = 0;
				break;
			}
		}
	}

	mutex_unlock(map_utils->mutex);

	__enable_irq();

	return ret;
}

int map_utils_get_keys(map_utils_t *map_utils, void **pkey, size_t *size, key_filter_t filter)
{
	int ret = -1;
	size_t index = 0;

	if(map_utils == NULL) {
		return ret;
	}

	if(pkey == NULL) {
		return ret;
	}

	if(size == NULL) {
		return ret;
	}

	if(filter == NULL) {
		return ret;
	}

	__disable_irq();

	mutex_lock(map_utils->mutex);

	if(!list_empty(&map_utils->list)) {
		struct list_head *pos;
		struct list_head *n;

		ret = 0;

		list_for_each_safe(pos, n, &map_utils->list) {
			map_utils_item_t *map_utils_item = list_entry(pos, map_utils_item_t, list);

			if(index < *size) {
				if(filter(map_utils_item->key) == 0) {
					pkey[index++] = map_utils_item->key;
				}
			} else {
				break;
			}
		}
	}

	mutex_unlock(map_utils->mutex);

	__enable_irq();

	*size = index;

	return ret;
}

int map_utils_get_values(map_utils_t *map_utils, void **pvalue, size_t *size, value_filter_t filter)
{
	int ret = -1;
	size_t index = 0;

	if(map_utils == NULL) {
		return ret;
	}

	if(pvalue == NULL) {
		return ret;
	}

	if(size == NULL) {
		return ret;
	}

	if(filter == NULL) {
		return ret;
	}

	__disable_irq();

	mutex_lock(map_utils->mutex);

	if(!list_empty(&map_utils->list)) {
		struct list_head *pos;
		struct list_head *n;

		ret = 0;

		list_for_each_safe(pos, n, &map_utils->list) {
			map_utils_item_t *map_utils_item = list_entry(pos, map_utils_item_t, list);

			if(index < *size) {
				if(filter(map_utils_item->value) == 0) {
					pvalue[index++] = map_utils_item->value;
				}
			} else {
				break;
			}
		}
	}

	mutex_unlock(map_utils->mutex);

	__enable_irq();

	*size = index;

	return ret;
}

size_t map_utils_get_size(map_utils_t *map_utils)
{
	size_t size = 0;

	if(map_utils == NULL) {
		return size;
	}

	__disable_irq();

	mutex_lock(map_utils->mutex);

	if(!list_empty(&map_utils->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &map_utils->list) {
			//map_utils_item_t *map_utils_item = list_entry(pos, map_utils_item_t, list);
			size++;
		}
	}

	mutex_unlock(map_utils->mutex);

	__enable_irq();

	return size;
}

void *map_utils_get_or_alloc_value(map_utils_t *map_utils, void *key, map_utils_value_alloc_t map_utils_value_alloc, map_utils_value_free_t map_utils_value_free)
{
	void *value = NULL;
	int ret;

	if(map_utils == NULL) {
		return value;
	}

	if(map_utils_value_alloc == NULL) {
		return value;
	}

	if(map_utils_value_free == NULL) {
		return value;
	}

	value = map_utils_get_value(map_utils, key);

	if(value != NULL) {
		return value;
	}

	if(__get_IPSR() != 0) {
		return value;
	}

	value = map_utils_value_alloc(key);

	if(value == NULL) {
		return value;
	}

	ret = map_utils_add_key_value(map_utils, key, value);

	if(ret != 0) {
		map_utils_value_free(value);
		value = NULL;
	}

	return value;
}
