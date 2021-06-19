

/*================================================================
 *
 *
 *   文件名称：soft_timer.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月22日 星期五 10时28分46秒
 *   修改日期：2021年06月19日 星期六 12时48分14秒
 *   描    述：
 *
 *================================================================*/
#include "soft_timer.h"

#include <string.h>
#include <stdlib.h>

#include "object_class.h"
#include "os_utils.h"
#define LOG_DISABLE
#include "log.h"

static object_class_t *soft_timer_class = NULL;

static void soft_timer_update_timeout(soft_timer_info_t *soft_timer_info, uint32_t delay, uint8_t wakeup)
{
	if(soft_timer_info->delay > delay) {
		soft_timer_info->delay = delay;
	}

	if(wakeup != 0) {
		signal_send(soft_timer_info->wakeup, 0, 0);
	}
}

static void soft_timer_delay(soft_timer_info_t *soft_timer_info, uint32_t timeout)
{
	signal_wait(soft_timer_info->wakeup, NULL, timeout);
}

static void common_soft_timer_fn(void *fn_ctx, void *chain_ctx)
{
	soft_timer_ctx_t *soft_timer_ctx = (soft_timer_ctx_t *)fn_ctx;
	uint32_t ticks = osKernelSysTick();
	uint32_t delay;

	if(ticks_duration(ticks, soft_timer_ctx->stamp) >= soft_timer_ctx->period) {
		soft_timer_ctx->stamp = ticks;
		delay = osWaitForever;

		switch(soft_timer_ctx->type) {
			case SOFT_TIMER_FN_TYPE_REPEAT: {
				ticks = osKernelSysTick();

				if(soft_timer_ctx->stamp + soft_timer_ctx->period >= ticks) {
					delay = soft_timer_ctx->stamp + soft_timer_ctx->period - ticks;
				} else {
					delay = 0;
				}

			}
			break;

			default: {
				mutex_lock(soft_timer_ctx->soft_timer_info->mutex);

				list_move_tail(&soft_timer_ctx->list, &soft_timer_ctx->soft_timer_info->deactive_timers);

				mutex_unlock(soft_timer_ctx->soft_timer_info->mutex);
			}
			break;
		}

		soft_timer_update_timeout(soft_timer_ctx->soft_timer_info, delay, 0);

		if(soft_timer_ctx->fn != NULL) {
			soft_timer_ctx->fn(soft_timer_ctx->fn_ctx, chain_ctx);
		}
	} else {
		delay = soft_timer_ctx->stamp + soft_timer_ctx->period - ticks;
		soft_timer_update_timeout(soft_timer_ctx->soft_timer_info, delay, 0);
	}

}

soft_timer_ctx_t *add_soft_timer(soft_timer_info_t *soft_timer_info, callback_fn_t fn, void *fn_ctx, uint32_t period, soft_timer_fn_type_t type)
{
	soft_timer_ctx_t *soft_timer_ctx = NULL;
	callback_item_t *callback_item = NULL;
	uint32_t ticks = osKernelSysTick();

	if(soft_timer_info == NULL) {
		return soft_timer_ctx;
	}

	if(fn == NULL) {
		return soft_timer_ctx;
	}

	soft_timer_ctx = (soft_timer_ctx_t *)os_alloc(sizeof(soft_timer_ctx_t));

	if(soft_timer_ctx == NULL) {
		return soft_timer_ctx;
	}

	memset(soft_timer_ctx, 0, sizeof(soft_timer_ctx_t));

	soft_timer_ctx->fn = fn;
	soft_timer_ctx->fn_ctx = fn_ctx;
	soft_timer_ctx->period = period;
	soft_timer_ctx->type = type;
	soft_timer_ctx->stamp = ticks;
	soft_timer_ctx->soft_timer_info = soft_timer_info;

	callback_item = (callback_item_t *)os_alloc(sizeof(callback_item_t));

	if(callback_item == NULL) {
		goto failed;
	}

	memset(callback_item, 0, sizeof(callback_item_t));

	soft_timer_ctx->callback_item = callback_item;

	callback_item->fn = common_soft_timer_fn;
	callback_item->fn_ctx = soft_timer_ctx;

	return soft_timer_ctx;
failed:

	if(callback_item != NULL) {
		os_free(callback_item);
	}

	if(soft_timer_ctx != NULL) {
		os_free(soft_timer_ctx);
	}

	soft_timer_ctx = NULL;

	return soft_timer_ctx;
}

int start_soft_timer(soft_timer_ctx_t *soft_timer_ctx)
{
	int ret = -1;

	if(soft_timer_ctx == NULL) {
		return ret;
	}

	mutex_lock(soft_timer_ctx->soft_timer_info->mutex);

	soft_timer_ctx->stamp = osKernelSysTick();
	list_move_tail(&soft_timer_ctx->list, &soft_timer_ctx->soft_timer_info->active_timers);

	mutex_unlock(soft_timer_ctx->soft_timer_info->mutex);

	soft_timer_update_timeout(soft_timer_ctx->soft_timer_info, soft_timer_ctx->period, 1);

	ret = 0;

	return ret;
}

int stop_soft_timer(soft_timer_ctx_t *soft_timer_ctx)
{
	int ret = -1;

	if(soft_timer_ctx == NULL) {
		return ret;
	}

	mutex_lock(soft_timer_ctx->soft_timer_info->mutex);

	list_move_tail(&soft_timer_ctx->list, &soft_timer_ctx->soft_timer_info->deactive_timers);

	mutex_unlock(soft_timer_ctx->soft_timer_info->mutex);

	soft_timer_update_timeout(soft_timer_ctx->soft_timer_info, 0, 1);

	ret = 0;

	return ret;
}

int remove_soft_timer(soft_timer_ctx_t *soft_timer_ctx)
{
	int ret = -1;

	if(soft_timer_ctx == NULL) {
		return ret;
	}

	mutex_lock(soft_timer_ctx->soft_timer_info->mutex);

	list_move_tail(&soft_timer_ctx->list, &soft_timer_ctx->soft_timer_info->delete_timers);

	mutex_unlock(soft_timer_ctx->soft_timer_info->mutex);

	ret = 0;

	return ret;
}

static void active_deactive_timers(soft_timer_info_t *soft_timer_info)
{
	struct list_head *head;
	struct list_head *pos;
	struct list_head *n;

	mutex_lock(soft_timer_info->mutex);

	head = &soft_timer_info->deactive_timers;
	list_for_each_safe(pos, n, head) {
		soft_timer_ctx_t *soft_timer_ctx = list_entry(pos, soft_timer_ctx_t, list);

		if(remove_callback(soft_timer_ctx->soft_timer_info->timer_cb_chain, soft_timer_ctx->callback_item) != 0) {
			debug("remove_callback %p in %p failed", soft_timer_ctx->callback_item, soft_timer_ctx->soft_timer_info);
		}

		list_del(pos);
	}

	head = &soft_timer_info->active_timers;
	list_for_each_safe(pos, n, head) {
		soft_timer_ctx_t *soft_timer_ctx = list_entry(pos, soft_timer_ctx_t, list);

		if(register_callback(soft_timer_ctx->soft_timer_info->timer_cb_chain, soft_timer_ctx->callback_item) != 0) {
			debug("register_callback %p in %p failed", soft_timer_ctx->callback_item, soft_timer_ctx->soft_timer_info);
		}

		list_del(pos);
	}

	head = &soft_timer_info->delete_timers;
	list_for_each_safe(pos, n, head) {
		soft_timer_ctx_t *soft_timer_ctx = list_entry(pos, soft_timer_ctx_t, list);

		if(remove_callback(soft_timer_ctx->soft_timer_info->timer_cb_chain, soft_timer_ctx->callback_item) != 0) {
			debug("remove_callback %p in %p failed", soft_timer_ctx->callback_item, soft_timer_ctx->soft_timer_info);
		}

		list_del(pos);

		os_free(soft_timer_ctx->callback_item);
		os_free(soft_timer_ctx);
	}

	mutex_unlock(soft_timer_info->mutex);
}

static void soft_timer_task(void const *argument)
{
	soft_timer_info_t *soft_timer_info = (soft_timer_info_t *)argument;

	if(soft_timer_info == NULL) {
		app_panic();
	}

	for(;;) {
		if(callback_chain_empty(soft_timer_info->timer_cb_chain)) {
			debug("%p idle delay", soft_timer_info);
			soft_timer_delay(soft_timer_info, osWaitForever);
		} else {
			soft_timer_info->delay = osWaitForever;
			do_callback_chain(soft_timer_info->timer_cb_chain, soft_timer_info);
			debug("%p delay:%d", soft_timer_info, soft_timer_info->delay);
			soft_timer_delay(soft_timer_info, soft_timer_info->delay);
		}

		active_deactive_timers(soft_timer_info);
	}
}

static void free_soft_timer_info(soft_timer_info_t *soft_timer_info)
{
	if(soft_timer_info == NULL) {
		return;
	}

	if(soft_timer_info->timer_cb_chain != NULL) {
		free_callback_chain(soft_timer_info->timer_cb_chain);
	}

	mutex_delete(soft_timer_info->mutex);

	signal_delete(soft_timer_info->wakeup);

	os_free(soft_timer_info);
}

static soft_timer_info_t *alloc_soft_timer_info(void *ctx)
{
	soft_timer_info_t *soft_timer_info = NULL;
	uint8_t *id = (uint8_t *)ctx;

	soft_timer_info = (soft_timer_info_t *)os_alloc(sizeof(soft_timer_info_t));

	if(soft_timer_info == NULL) {
		return soft_timer_info;
	}

	memset(soft_timer_info, 0, sizeof(soft_timer_info_t));

	soft_timer_info->id = *id;

	INIT_LIST_HEAD(&soft_timer_info->deactive_timers);
	INIT_LIST_HEAD(&soft_timer_info->active_timers);
	INIT_LIST_HEAD(&soft_timer_info->delete_timers);

	soft_timer_info->timer_cb_chain = alloc_callback_chain();

	if(soft_timer_info->timer_cb_chain == NULL) {
		debug("");
		goto failed;
	}

	soft_timer_info->wakeup = signal_create(1);

	if(soft_timer_info->wakeup == NULL) {
		debug("");
		goto failed;
	}

	soft_timer_info->mutex = mutex_create();

	if(soft_timer_info->mutex == NULL) {
		debug("");
		goto failed;
	}

	osThreadDef(soft_timer, soft_timer_task, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(soft_timer), soft_timer_info);

	return soft_timer_info;
failed:
	free_soft_timer_info(soft_timer_info);
	soft_timer_info = NULL;
	return soft_timer_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	soft_timer_info_t *soft_timer_info = (soft_timer_info_t *)o;
	uint32_t id = (uint32_t)ctx;

	if(soft_timer_info->id == id) {
		ret = 0;
	}

	return ret;
}

soft_timer_info_t *get_or_alloc_soft_timer_info(uint8_t id)
{
	soft_timer_info_t *soft_timer_info = NULL;

	os_enter_critical();

	if(soft_timer_class == NULL) {
		soft_timer_class = object_class_alloc();
	}

	os_leave_critical();

	soft_timer_info = (soft_timer_info_t *)object_class_get_or_alloc_object(soft_timer_class, object_filter, (void *)&id, (object_alloc_t)alloc_soft_timer_info, (object_free_t)free_soft_timer_info);

	return soft_timer_info;
}
