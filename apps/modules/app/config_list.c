

/*================================================================
 *   
 *   
 *   文件名称：config_list.c
 *   创 建 者：肖飞
 *   创建日期：2020年09月09日 星期三 16时47分04秒
 *   修改日期：2020年09月09日 星期三 16时47分25秒
 *   描    述：
 *
 *================================================================*/
#include "config_list.h"
#include "log.h"

static LIST_HEAD(config_info_list);
static osMutexId config_info_list_mutex = NULL;

void config_init(void)
{
	if(config_info_list_mutex == NULL) {
		osMutexDef(config_info_list_mutex);

		config_info_list_mutex = osMutexCreate(osMutex(config_info_list_mutex));

		if(config_info_list_mutex == NULL) {
			app_panic();
		}
	}
}

void *config_get_class_key_value(uint32_t cls, uint32_t key)
{
	void *ret = NULL;
	osStatus os_status;
	struct list_head *head_cls;
	config_cls_t *config_cls_item;

	os_status = osMutexWait(config_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	head_cls = &config_info_list;
	list_for_each_entry(config_cls_item, head_cls, config_cls_t, list) {
		if(config_cls_item->cls == cls) {
			struct list_head *head_key;
			config_key_t *config_key_item;

			head_key = &config_cls_item->key_list;
			list_for_each_entry(config_key_item, head_key, config_key_t, list) {
				if(config_key_item->key == key) {
					ret = config_key_item->value;
					break;
				}
			}
			break;
		}
	}

	os_status = osMutexRelease(config_info_list_mutex);

	if(os_status != osOK) {
	}

	return ret;
}

int config_set_class_key_value(uint32_t cls, uint32_t key, void *value)
{
	int ret = -1;
	osStatus os_status;
	struct list_head *head_cls;
	struct list_head *head_key;
	config_cls_t *config_cls_item;
	config_key_t *config_key_item;
	config_cls_t *config_cls = NULL;
	config_key_t *config_key = NULL;

	os_status = osMutexWait(config_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	head_cls = &config_info_list;
	list_for_each_entry(config_cls_item, head_cls, config_cls_t, list) {
		if(config_cls_item->cls == cls) {
			config_cls = config_cls_item;
			break;
		}
	}

	if(config_cls == NULL) {
		config_cls = (config_cls_t *)os_alloc(sizeof(config_cls_t));

		if(config_cls == NULL) {
			app_panic();
		} else {
			config_cls->cls = cls;
			INIT_LIST_HEAD(&config_cls->key_list);
			list_add_tail(&config_cls->list, head_cls);
			//debug("alloc cls:%u\n", config_cls->cls);
		}
	}

	head_key = &config_cls->key_list;
	list_for_each_entry(config_key_item, head_key, config_key_t, list) {
		if(config_key_item->key == key) {
			config_key = config_key_item;
			config_key->value = value;
			ret = 0;

			break;
		}
	}

	if(config_key == NULL) {
		config_key = (config_key_t *)os_alloc(sizeof(config_key_t));

		if(config_key == NULL) {
			app_panic();
		} else {
			config_key->key = key;
			config_key->value = value;
			list_add_tail(&config_key->list, head_key);
			//debug("alloc cls:%u, key:%p, value:%p\n", config_cls->cls, config_key->key, config_key->value);
			ret = 0;
		}
	}

	os_status = osMutexRelease(config_info_list_mutex);

	if(os_status != osOK) {
	}

	return ret;
}

int config_delete_class_key(uint32_t cls, uint32_t key)
{
	int ret = -1;
	osStatus os_status;
	struct list_head *head_cls;
	struct list_head *head_key;
	config_cls_t *config_cls_item;
	config_key_t *config_key_item;
	config_cls_t *config_cls = NULL;
	config_key_t *config_key = NULL;

	os_status = osMutexWait(config_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	head_cls = &config_info_list;
	list_for_each_entry(config_cls_item, head_cls, config_cls_t, list) {
		if(config_cls_item->cls == cls) {
			config_cls = config_cls_item;

			head_key = &config_cls->key_list;
			list_for_each_entry(config_key_item, head_key, config_key_t, list) {
				if(config_key_item->key == key) {
					config_key = config_key_item;
					break;
				}
			}
			break;
		}
	}


	if(config_key != NULL) {
		list_del(&config_key->list);
		//debug("free cls:%u, key:%u, value:%p\n", config_cls->cls, config_key->key, config_key->value);
		os_free(config_key);

		if(list_empty(head_key)) {
			list_del(&config_cls->list);
			//debug("free cls:%u\n", config_cls->cls);
			os_free(config_cls);
		}

		ret = 0;
	}

	os_status = osMutexRelease(config_info_list_mutex);

	if(os_status != osOK) {
	}

	return ret;
}


void config_dump_class_key_value(void)
{
	osStatus os_status;
	struct list_head *head_cls;
	config_cls_t *config_cls_item;

	os_status = osMutexWait(config_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	head_cls = &config_info_list;
	list_for_each_entry(config_cls_item, head_cls, config_cls_t, list) {
		struct list_head *head_key;
		config_key_t *config_key_item;

		debug("cls:%u\n", config_cls_item->cls);
		head_key = &config_cls_item->key_list;
		list_for_each_entry(config_key_item, head_key, config_key_t, list) {
			debug("\tkey:%p, value:%p\n", config_key_item->key, config_key_item->value);
		}
	}

	os_status = osMutexRelease(config_info_list_mutex);

	if(os_status != osOK) {
	}
}
