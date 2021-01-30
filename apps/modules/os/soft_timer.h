

/*================================================================
 *   
 *   
 *   文件名称：soft_timer.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月22日 星期五 10时28分59秒
 *   修改日期：2021年01月28日 星期四 15时45分12秒
 *   描    述：
 *
 *================================================================*/
#ifndef _SOFT_TIMER_H
#define _SOFT_TIMER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "callback_chain.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uint32_t id;
	os_mutex_t mutex;
	struct list_head deactive_timers;
	struct list_head active_timers;
	struct list_head delete_timers;
	callback_chain_t *timer_cb_chain;
	os_signal_t wakeup;
	uint32_t delay;
} soft_timer_info_t;

typedef enum {
	SOFT_TIMER_FN_TYPE_ONESHOT = 0,
	SOFT_TIMER_FN_TYPE_REPEAT
} soft_timer_fn_type_t;

typedef struct {
	struct list_head list;
	callback_fn_t fn;
	void *fn_ctx;
	uint32_t period;
	soft_timer_fn_type_t type;
	uint32_t stamp;
	callback_item_t *callback_item;
	soft_timer_info_t *soft_timer_info;
} soft_timer_ctx_t;

soft_timer_ctx_t *add_soft_timer(soft_timer_info_t *soft_timer_info, callback_fn_t fn, void *fn_ctx, uint32_t period, soft_timer_fn_type_t type);
int start_soft_timer(soft_timer_ctx_t *soft_timer_ctx);
int stop_soft_timer(soft_timer_ctx_t *soft_timer_ctx);
int remove_soft_timer(soft_timer_ctx_t *soft_timer_ctx);
soft_timer_info_t *get_or_alloc_soft_timer_info(uint32_t id);

#endif //_SOFT_TIMER_H
