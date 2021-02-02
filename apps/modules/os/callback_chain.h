

/*================================================================
 *   
 *   
 *   文件名称：callback_chain.h
 *   创 建 者：肖飞
 *   创建日期：2020年03月20日 星期五 08时20分40秒
 *   修改日期：2021年02月02日 星期二 11时46分19秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CALLBACK_CHAIN_H
#define _CALLBACK_CHAIN_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "os_utils.h"
#include "list_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	struct list_head list_callback;
	os_mutex_t callback_mutex;
} callback_chain_t;

typedef void (*callback_fn_t)(void *fn_ctx, void *chain_ctx);

typedef struct {
	struct list_head list_head;
	callback_fn_t fn;
	void *fn_ctx;
} callback_item_t;

typedef int (*callback_item_filter_t)(callback_item_t *callback_item, void *ctx);

void free_callback_chain(callback_chain_t *callback_chain);
callback_chain_t *alloc_callback_chain(void);
int register_callback(callback_chain_t *callback_chain, callback_item_t *callback_item);
callback_item_t *get_callback(callback_chain_t *callback_chain, callback_item_filter_t filter, void *ctx);
int remove_callback(callback_chain_t *callback_chain, callback_item_t *callback_item);
void do_callback_chain(callback_chain_t *callback_chain, void *chain_ctx);
int callback_chain_empty(callback_chain_t *callback_chain);
#endif //_CALLBACK_CHAIN_H
