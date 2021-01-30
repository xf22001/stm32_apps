

/*================================================================
 *
 *
 *   文件名称：callback_chain.c
 *   创 建 者：肖飞
 *   创建日期：2020年03月20日 星期五 08时20分36秒
 *   修改日期：2021年01月30日 星期六 09时29分22秒
 *   描    述：
 *
 *================================================================*/
#include "callback_chain.h"
#include "os_utils.h"

void free_callback_chain(callback_chain_t *callback_chain)
{
	if(callback_chain == NULL) {
		return;
	}

	mutex_lock(callback_chain->callback_mutex);

	if(!list_empty(&callback_chain->list_callback)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &callback_chain->list_callback) {
			list_del(pos);
		}
	}

	mutex_unlock(callback_chain->callback_mutex);

	mutex_delete(callback_chain->callback_mutex);

	os_free(callback_chain);
}

callback_chain_t *alloc_callback_chain(void)
{
	callback_chain_t *callback_chain = (callback_chain_t *)os_alloc(sizeof(callback_chain_t));

	if(callback_chain == NULL) {
		return callback_chain;
	}

	callback_chain->callback_mutex = mutex_create();

	if(callback_chain->callback_mutex == NULL) {
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
	uint8_t found = 0;

	if(callback_chain == NULL) {
		return ret;
	}

	if(callback_item == NULL) {
		return ret;
	}

	mutex_lock(callback_chain->callback_mutex);

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

	mutex_unlock(callback_chain->callback_mutex);

	return ret;
}

int remove_callback(callback_chain_t *callback_chain, callback_item_t *callback_item)
{
	int ret = -1;

	if(callback_chain == NULL) {
		return ret;
	}

	if(callback_item == NULL) {
		return ret;
	}

	mutex_lock(callback_chain->callback_mutex);

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

	mutex_unlock(callback_chain->callback_mutex);

	return ret;
}

void do_callback_chain(callback_chain_t *callback_chain, void *chain_ctx)
{
	callback_item_t *callback_item = NULL;

	if(callback_chain == NULL) {
		return;
	}

	mutex_lock(callback_chain->callback_mutex);

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

	mutex_unlock(callback_chain->callback_mutex);

	return;
}

int callback_chain_empty(callback_chain_t *callback_chain)
{
	int ret;

	if(callback_chain == NULL) {
		return 1;
	}

	mutex_lock(callback_chain->callback_mutex);

	ret = list_empty(&callback_chain->list_callback);

	mutex_unlock(callback_chain->callback_mutex);
	
	return ret;
}
