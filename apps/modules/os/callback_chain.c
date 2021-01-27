

/*================================================================
 *
 *
 *   文件名称：callback_chain.c
 *   创 建 者：肖飞
 *   创建日期：2020年03月20日 星期五 08时20分36秒
 *   修改日期：2021年01月27日 星期三 09时08分50秒
 *   描    述：
 *
 *================================================================*/
#include "callback_chain.h"
#include "os_utils.h"

void free_callback_chain(callback_chain_t *callback_chain)
{
	osStatus status;

	if(callback_chain == NULL) {
		return;
	}

	if(callback_chain->mutex != NULL) {
		status = osMutexWait(callback_chain->mutex, osWaitForever);

		if(status != osOK) {
		}
	}

	if(!list_empty(&callback_chain->list_callback)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &callback_chain->list_callback) {
			list_del(pos);
		}
	}

	if(callback_chain->mutex != NULL) {
		status = osMutexRelease(callback_chain->mutex);

		if(status != osOK) {
		}
	}

	if(callback_chain->mutex != NULL) {
		status = osMutexDelete(callback_chain->mutex);

		if(osOK != status) {
		}
	}

	os_free(callback_chain);
}

callback_chain_t *alloc_callback_chain(void)
{
	osMutexDef(mutex);
	callback_chain_t *callback_chain = (callback_chain_t *)os_alloc(sizeof(callback_chain_t));

	if(callback_chain == NULL) {
		return callback_chain;
	}

	callback_chain->mutex = osMutexCreate(osMutex(mutex));

	if(callback_chain->mutex == NULL) {
		goto failed;
	}

	INIT_LIST_HEAD(&callback_chain->list_callback);

	return callback_chain;
failed:
	free_callback_chain(callback_chain);
	callback_chain = NULL;

	return callback_chain;
}

int register_callback(callback_chain_t *callback_chain, callback_item_t *callback_item)
{
	int ret = -1;
	osStatus status;
	uint8_t found = 0;

	if(callback_chain == NULL) {
		return ret;
	}

	if(callback_item == NULL) {
		return ret;
	}

	status = osMutexWait(callback_chain->mutex, osWaitForever);

	if(status != osOK) {
	}

	if(!list_empty(&callback_chain->list_callback)) {
		struct list_head *pos;
		list_for_each(pos, &callback_chain->list_callback) {
			callback_item_t *entry = list_entry(pos, callback_item_t, list_head);

			if(callback_item == entry) {
				found = 1;
				break;
			}
		}
	}

	if(found == 0) {
		list_add_tail(&callback_item->list_head, &callback_chain->list_callback);
		ret = 0;
	}

	status = osMutexRelease(callback_chain->mutex);

	if(status != osOK) {
	}

	return ret;
}

int remove_callback(callback_chain_t *callback_chain, callback_item_t *callback_item)
{
	int ret = -1;
	osStatus status;

	if(callback_chain == NULL) {
		return ret;
	}

	if(callback_item == NULL) {
		return ret;
	}

	status = osMutexWait(callback_chain->mutex, osWaitForever);

	if(status != osOK) {
	}

	if(!list_empty(&callback_chain->list_callback)) {
		struct list_head *pos;
		struct list_head *n;
		list_for_each_safe(pos, n, &callback_chain->list_callback) {
			callback_item_t *entry = list_entry(pos, callback_item_t, list_head);

			if(callback_item == entry) {
				list_del(pos);
				ret = 0;
				break;
			}
		}
	}

	status = osMutexRelease(callback_chain->mutex);

	if(status != osOK) {
	}

	return ret;
}

void do_callback_chain(callback_chain_t *callback_chain, void *chain_ctx)
{
	osStatus status;
	callback_item_t *callback_item = NULL;

	if(callback_chain == NULL) {
		return;
	}

	status = osMutexWait(callback_chain->mutex, osWaitForever);

	if(status != osOK) {
	}

	if(!list_empty(&callback_chain->list_callback)) {
		struct list_head *pos;
		struct list_head *n;
		list_for_each_safe(pos, n, &callback_chain->list_callback) {
			callback_item = list_entry(pos, callback_item_t, list_head);

			if(callback_item->fn != NULL) {
				callback_item->fn(callback_item->fn_ctx, chain_ctx);
			}

		}
	}

	status = osMutexRelease(callback_chain->mutex);

	if(status != osOK) {
	}

	return;
}

int callback_chain_empty(callback_chain_t *callback_chain)
{
	if(callback_chain == NULL) {
		return 0;
	}

	return list_empty(&callback_chain->list_callback);
}
