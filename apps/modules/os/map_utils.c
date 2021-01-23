

/*================================================================
 *
 *
 *   文件名称：map_utils.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 11时40分50秒
 *   修改日期：2021年01月23日 星期六 21时29分44秒
 *   描    述：
 *
 *================================================================*/
#include "map_utils.h"

#include "os_utils.h"
#include "log.h"

void map_utils_free(map_utils_t *map_utils)
{
	osStatus os_status;

	if(map_utils == NULL) {
		return;
	}


	if(map_utils->mutex != NULL) {
		os_status = osMutexWait(map_utils->mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	if(!list_empty(&map_utils->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &map_utils->list) {
			map_utils_item_t *map_utils_item = list_entry(pos, map_utils_item_t, list);
			list_del(pos);
			os_free(map_utils_item);
		}
	}

	if(map_utils->sync_mutex != NULL) {
		os_status = osMutexDelete(map_utils->sync_mutex);

		if(osOK != os_status) {
		}
	}

	if(map_utils->mutex != NULL) {
		os_status = osMutexRelease(map_utils->mutex);

		if(os_status != osOK) {
		}
	}


	if(map_utils->mutex != NULL) {
		os_status = osMutexDelete(map_utils->mutex);

		if(osOK != os_status) {
		}
	}

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

	osMutexDef(mutex);
	osMutexDef(sync_mutex);

	if(__get_IPSR() != 0) {
		return map_utils;
	}

	map_utils = (map_utils_t *)os_alloc(sizeof(map_utils_t));

	if(map_utils == NULL) {
		return map_utils;
	}

	map_utils->mutex = osMutexCreate(osMutex(mutex));

	if(map_utils->mutex == NULL) {
		goto failed;
	}

	map_utils->sync_mutex = osMutexCreate(osMutex(sync_mutex));

	if(map_utils->sync_mutex == NULL) {
		goto failed;
	}

	INIT_LIST_HEAD(&map_utils->list);

	if(match == NULL) {
		match = map_utils_default_match;
	}

	map_utils->match = match;

	return map_utils;

failed:
	os_free(map_utils);
	map_utils = NULL;

	return map_utils;
}

static int map_utils_add_key_value(map_utils_t *map_utils, void *key, void *value)
{
	int ret = -1;
	osStatus os_status;
	map_utils_item_t *map_utils_item = NULL;
	uint8_t found = 0;

	if(map_utils == NULL) {
		return ret;
	}

	map_utils_item = (map_utils_item_t *)os_alloc(sizeof(map_utils_item_t));

	if(map_utils_item == NULL) {
		return ret;
	}

	if(map_utils->mutex != NULL) {
		os_status = osMutexWait(map_utils->mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

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

	if(map_utils->mutex != NULL) {
		os_status = osMutexRelease(map_utils->mutex);

		if(os_status != osOK) {
		}
	}

	return ret;
}

static void *map_utils_get_value(map_utils_t *map_utils, void *key)
{
	osStatus os_status;
	void *value = NULL;

	if(map_utils == NULL) {
		return value;
	}

	if(map_utils->mutex != NULL) {
		os_status = osMutexWait(map_utils->mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

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

	if(map_utils->mutex != NULL) {
		os_status = osMutexRelease(map_utils->mutex);

		if(os_status != osOK) {
		}
	}

	return value;
}

static int map_utils_remove_value(map_utils_t *map_utils, void *key)
{
	int ret = -1;
	osStatus os_status;

	if(map_utils == NULL) {
		return ret;
	}

	if(map_utils->mutex != NULL) {
		os_status = osMutexWait(map_utils->mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

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

	if(map_utils->mutex != NULL) {
		os_status = osMutexRelease(map_utils->mutex);

		if(os_status != osOK) {
		}
	}

	return ret;
}

void *map_utils_get_or_alloc_value(map_utils_t *map_utils, void *key, map_utils_value_alloc_t map_utils_value_alloc, map_utils_value_free_t map_utils_value_free)
{
	void *value = NULL;
	osStatus os_status;

	if(map_utils == NULL) {
		return value;
	}

	if(map_utils_value_alloc == NULL) {
		return value;
	}

	if(map_utils_value_free == NULL) {
		return value;
	}

	if(map_utils->sync_mutex != NULL) {
		os_status = osMutexWait(map_utils->sync_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	value = map_utils_get_value(map_utils, key);

	if(value == NULL) {
		value = map_utils_value_alloc(key);

		if(value != NULL) {
			int ret = map_utils_add_key_value(map_utils, key, value);

			if(ret != 0) {
				map_utils_value_free(value);
				value = NULL;
			}
		}
	}

	if(map_utils->sync_mutex != NULL) {
		os_status = osMutexRelease(map_utils->sync_mutex);

		if(os_status != osOK) {
		}
	}

	return value;
}
