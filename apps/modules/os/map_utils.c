

/*================================================================
 *
 *
 *   文件名称：map_utils.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 11时40分50秒
 *   修改日期：2020年12月29日 星期二 13时40分48秒
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

map_utils_t *map_utils_alloc(void)
{
	map_utils_t *map_utils = NULL;

	osMutexDef(mutex);

	map_utils = (map_utils_t *)os_alloc(sizeof(map_utils_t));

	if(map_utils == NULL) {
		return map_utils;
	}

	map_utils->mutex = osMutexCreate(osMutex(mutex));

	if(map_utils->mutex == NULL) {
		goto failed;
	}

	INIT_LIST_HEAD(&map_utils->list);

	return map_utils;

failed:
	os_free(map_utils);
	map_utils = NULL;

	return map_utils;
}

int map_utils_add_key_value(map_utils_t *map_utils, void *key, void *value)
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

			if(item->key == key) {
				found = 1;
			}
		}
	}

	if(found == 0) {
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

void *map_utils_get_value(map_utils_t *map_utils, void *key)
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

			if(map_utils_item->key == key) {
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

int map_utils_remove_value(map_utils_t *map_utils, void *key)
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

			if(map_utils_item->key == key) {
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

