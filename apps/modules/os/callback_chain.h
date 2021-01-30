

/*================================================================
 *   
 *   
 *   文件名称：callback_chain.h
 *   创 建 者：肖飞
 *   创建日期：2020年03月20日 星期五 08时20分40秒
 *   修改日期：2021年01月30日 星期六 08时09分48秒
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

typedef void (*callback_fn_t)(void *fn_ctx, void *chain_ctx);

typedef struct {
	struct list_head list_head;
	callback_fn_t fn;
	void *fn_ctx;
} callback_item_t;

typedef struct {
	os_mutex_t callback_mutex;
	struct list_head list_callback;
} callback_chain_t;

void free_callback_chain(callback_chain_t *callback_chain);
callback_chain_t *alloc_callback_chain(void);
int register_callback(callback_chain_t *callback_chain, callback_item_t *callback_item);
int remove_callback(callback_chain_t *callback_chain, callback_item_t *callback_item);
void do_callback_chain(callback_chain_t *callback_chain, void *chain_ctx);
int callback_chain_empty(callback_chain_t *callback_chain);
#endif //_CALLBACK_CHAIN_H
