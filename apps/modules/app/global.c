

/*================================================================
 *
 *
 *   文件名称：global.c
 *   创 建 者：肖飞
 *   创建日期：2020年09月09日 星期三 09时05分04秒
 *   修改日期：2020年09月09日 星期三 14时12分33秒
 *   描    述：
 *
 *================================================================*/
#include "global.h"
#include "log.h"

static LIST_HEAD(global_info_list);
static osMutexId global_info_list_mutex = NULL;

void global_init(void)
{
	if(global_info_list_mutex == NULL) {
		osMutexDef(global_info_list_mutex);

		global_info_list_mutex = osMutexCreate(osMutex(global_info_list_mutex));

		if(global_info_list_mutex == NULL) {
			app_panic();
		}
	}
}

void *global_get_class_key_value(uint32_t cls, uint32_t key)
{
	void *ret = NULL;
	osStatus os_status;
	struct list_head *head_cls;
	global_cls_t *global_cls_item;

	os_status = osMutexWait(global_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	head_cls = &global_info_list;
	list_for_each_entry(global_cls_item, head_cls, global_cls_t, list) {
		if(global_cls_item->cls == cls) {
			struct list_head *head_key;
			global_key_t *global_key_item;

			head_key = &global_cls_item->key_list;
			list_for_each_entry(global_key_item, head_key, global_key_t, list) {
				if(global_key_item->key == key) {
					ret = global_key_item->value;
					break;
				}
			}
			break;
		}
	}

	os_status = osMutexRelease(global_info_list_mutex);

	if(os_status != osOK) {
	}

	return ret;
}

int global_set_class_key_value(uint32_t cls, uint32_t key, void *value, uint8_t replace)
{
	int ret = -1;
	osStatus os_status;
	struct list_head *head_cls;
	struct list_head *head_key;
	global_cls_t *global_cls_item;
	global_key_t *global_key_item;
	global_cls_t *global_cls = NULL;
	global_key_t *global_key = NULL;

	os_status = osMutexWait(global_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	head_cls = &global_info_list;
	list_for_each_entry(global_cls_item, head_cls, global_cls_t, list) {
		if(global_cls_item->cls == cls) {
			global_cls = global_cls_item;
			break;
		}
	}

	if(global_cls == NULL) {
		global_cls = (global_cls_t *)os_alloc(sizeof(global_cls_t));

		if(global_cls == NULL) {
			app_panic();
		} else {
			global_cls->cls = cls;
			INIT_LIST_HEAD(&global_cls->key_list);
			list_add_tail(&global_cls->list, head_cls);
			//debug("alloc cls:%u\n", global_cls->cls);
		}
	}

	head_key = &global_cls->key_list;
	list_for_each_entry(global_key_item, head_key, global_key_t, list) {
		if(global_key_item->key == key) {
			if(replace == 0) {
				app_panic();
			} else {
				global_key = global_key_item;
				global_key->value = value;
				ret = 0;
			}

			break;
		}
	}

	if(global_key == NULL) {
		global_key = (global_key_t *)os_alloc(sizeof(global_key_t));

		if(global_key == NULL) {
			app_panic();
		} else {
			global_key->key = key;
			global_key->value = value;
			list_add_tail(&global_key->list, head_key);
			//debug("alloc cls:%u, key:%u, value:%p\n", global_cls->cls, global_key->key, global_key->value);
			ret = 0;
		}
	}

	os_status = osMutexRelease(global_info_list_mutex);

	if(os_status != osOK) {
	}

	return ret;
}

int global_delete_class_key(uint32_t cls, uint32_t key)
{
	int ret = -1;
	osStatus os_status;
	struct list_head *head_cls;
	struct list_head *head_key;
	global_cls_t *global_cls_item;
	global_key_t *global_key_item;
	global_cls_t *global_cls = NULL;
	global_key_t *global_key = NULL;

	os_status = osMutexWait(global_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	head_cls = &global_info_list;
	list_for_each_entry(global_cls_item, head_cls, global_cls_t, list) {
		if(global_cls_item->cls == cls) {
			global_cls = global_cls_item;

			head_key = &global_cls->key_list;
			list_for_each_entry(global_key_item, head_key, global_key_t, list) {
				if(global_key_item->key == key) {
					global_key = global_key_item;
					break;
				}
			}
			break;
		}
	}


	if(global_key != NULL) {
		list_del(&global_key->list);
		//debug("free cls:%u, key:%u, value:%p\n", global_cls->cls, global_key->key, global_key->value);
		os_free(global_key);

		if(list_empty(head_key)) {
			list_del(&global_cls->list);
			//debug("free cls:%u\n", global_cls->cls);
			os_free(global_cls);
		}

		ret = 0;
	}

	os_status = osMutexRelease(global_info_list_mutex);

	if(os_status != osOK) {
	}

	return ret;
}


void global_dump_class_key_value(void)
{
	osStatus os_status;
	struct list_head *head_cls;
	global_cls_t *global_cls_item;

	os_status = osMutexWait(global_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	head_cls = &global_info_list;
	list_for_each_entry(global_cls_item, head_cls, global_cls_t, list) {
		struct list_head *head_key;
		global_key_t *global_key_item;

		debug("cls:%u\n", global_cls_item->cls);
		head_key = &global_cls_item->key_list;
		list_for_each_entry(global_key_item, head_key, global_key_t, list) {
			debug("\tkey:%u, value:%p\n", global_key_item->key, global_key_item->value);
		}
	}

	os_status = osMutexRelease(global_info_list_mutex);

	if(os_status != osOK) {
	}
}

