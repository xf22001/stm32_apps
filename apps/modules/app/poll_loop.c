

/*================================================================
 *
 *
 *   文件名称：poll_loop.c
 *   创 建 者：肖飞
 *   创建日期：2020年08月11日 星期二 09时54分20秒
 *   修改日期：2021年05月19日 星期三 08时50分17秒
 *   描    述：
 *
 *================================================================*/
#include "poll_loop.h"
#include <string.h>
#include <lwip/sockets.h>

#include "object_class.h"

#include "sal_hook.h"
#include "log.h"

static object_class_t *poll_loop_class = NULL;

poll_ctx_t *alloc_poll_ctx(void)
{
	poll_ctx_t *poll_ctx = (poll_ctx_t *)os_calloc(1, sizeof(poll_ctx_t));

	OS_ASSERT(poll_ctx != NULL);
	INIT_LIST_HEAD(&poll_ctx->list);
	poll_ctx->poll_fd.fd = -1;

	return poll_ctx;
}

void free_poll_ctx(poll_ctx_t *poll_ctx)
{
	os_free(poll_ctx);
}

int add_poll_loop_ctx_item(poll_loop_t *poll_loop, poll_ctx_t *poll_ctx)
{
	int ret = -1;
	poll_ctx_t *poll_ctx_item;
	struct list_head *head;
	uint8_t found = 0;

	head = &poll_loop->poll_ctx_list;

	mutex_lock(poll_loop->poll_ctx_list_mutex);

	list_for_each_entry(poll_ctx_item, head, poll_ctx_t, list) {
		if(poll_ctx_item == poll_ctx) {
			found = 1;
		}
	}

	if(found == 0) {
		list_add_tail(&poll_ctx->list, &poll_loop->poll_ctx_list);
		ret = 0;
	}

	mutex_unlock(poll_loop->poll_ctx_list_mutex);

	return ret;
}

int remove_poll_loop_ctx_item(poll_loop_t *poll_loop, poll_ctx_t *poll_ctx)
{
	int ret = -1;

	mutex_lock(poll_loop->poll_ctx_list_mutex);

	list_del(&poll_ctx->list);
	ret = 0;

	mutex_unlock(poll_loop->poll_ctx_list_mutex);

	return ret;
}

static int poll_loop_poll(poll_loop_t *poll_loop)
{
	int ret = -1;
	struct fd_set rfds;
	struct fd_set wfds;
	struct fd_set efds;
	struct timeval tv = {0, 1000 * 50};
	int max_fd = -1;
	struct list_head *head;
	poll_ctx_t *poll_ctx;

	head = &poll_loop->poll_ctx_list;

	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&efds);

	mutex_lock(poll_loop->poll_ctx_list_mutex);

	list_for_each_entry(poll_ctx, head, poll_ctx_t, list) {
		poll_fd_t *poll_fd = &poll_ctx->poll_fd;

		if(poll_fd->available == 0) {
			continue;
		}

		if(poll_fd->config.v == 0) {
			continue;
		}

		if(poll_fd->fd > max_fd) {
			max_fd = poll_fd->fd;
		}

		if(poll_fd->config.s.poll_in != 0) {
			FD_SET(poll_fd->fd, &rfds);
		}

		if(poll_fd->config.s.poll_out != 0) {
			FD_SET(poll_fd->fd, &wfds);
		}

		if(poll_fd->config.s.poll_err != 0) {
			FD_SET(poll_fd->fd, &efds);
		}
	}

	mutex_unlock(poll_loop->poll_ctx_list_mutex);

	max_fd += 1;

	if(max_fd > 0) {
		ret = select(max_fd, &rfds, &wfds, &efds, &tv);
	} else {
		osDelay(tv.tv_sec * 1000 + tv.tv_usec / 1000);
		ret = 0;
	}

	if(ret >= 0) {
		mutex_lock(poll_loop->poll_ctx_list_mutex);

		list_for_each_entry(poll_ctx, head, poll_ctx_t, list) {
			poll_fd_t *poll_fd = &poll_ctx->poll_fd;

			if(poll_fd->available == 0) {
				continue;
			}

			poll_fd->status.v = 0;

			if(FD_ISSET(poll_fd->fd, &rfds)) {
				poll_fd->status.s.poll_in = 1;
			}

			if(FD_ISSET(poll_fd->fd, &wfds)) {
				poll_fd->status.s.poll_out = 1;
			}

			if(FD_ISSET(poll_fd->fd, &efds)) {
				poll_fd->status.s.poll_err = 1;
			}

			if(poll_fd->status.v == 0) {
				continue;
			}

			if(poll_ctx->poll_handler != NULL) {
				//debug("handler for %s", poll_ctx->name);
				poll_ctx->poll_handler(poll_ctx);

			}
		}

		mutex_unlock(poll_loop->poll_ctx_list_mutex);
	} else {
		debug("ret:%d, errno:%d", ret, errno);
	}

	return ret;
}

static void poll_loop_periodic(poll_loop_t *poll_loop)
{
	struct list_head *pos;
	struct list_head *next;
	struct list_head *head;
	poll_ctx_t *poll_ctx;

	head = &poll_loop->poll_ctx_list;

	mutex_lock(poll_loop->poll_ctx_list_mutex);

	list_for_each_safe(pos, next, head) {
		poll_ctx = list_entry(pos, poll_ctx_t, list);

		if(poll_ctx->poll_periodic != NULL) {
			//debug("periodic for %s", poll_ctx->name);
			poll_ctx->poll_periodic(poll_ctx);
		}
	}

	mutex_unlock(poll_loop->poll_ctx_list_mutex);
}

static uint8_t dump_poll_ctx = 0;

void set_dump_poll_ctx(void)
{
	dump_poll_ctx = 1;
}

static void task_poll_dump_poll_ctx(poll_loop_t *poll_loop)
{
	struct list_head *head;
	poll_ctx_t *poll_ctx;

	if(dump_poll_ctx == 0) {
		return;
	}

	dump_poll_ctx = 0;

	head = &poll_loop->poll_ctx_list;

	mutex_lock(poll_loop->poll_ctx_list_mutex);

	list_for_each_entry(poll_ctx, head, poll_ctx_t, list) {
		debug("%p, name:%s, fd:%d, available:%d, in:%d, out:%d, err:%d",
		      poll_ctx,
		      poll_ctx->name,
		      poll_ctx->poll_fd.fd,
		      poll_ctx->poll_fd.available,
		      poll_ctx->poll_fd.config.s.poll_in,
		      poll_ctx->poll_fd.config.s.poll_out,
		      poll_ctx->poll_fd.config.s.poll_err);
	}

	mutex_unlock(poll_loop->poll_ctx_list_mutex);
}

static void task_poll_loop(void const *argument)
{
	poll_loop_t *poll_loop = (poll_loop_t *)argument;

	if(poll_loop == NULL) {
		app_panic();
	}

	while(1) {
		int ret;

		ret = poll_loop_poll(poll_loop);

		if(ret < 0) {
			//debug("poll result:%d", ret);
			//dump_poll_ctx = 1;
		}

		poll_loop_periodic(poll_loop);

		task_poll_dump_poll_ctx(poll_loop);
	}
}

void free_poll_loop(poll_loop_t *poll_loop)
{
	if(poll_loop == NULL) {
		return;
	}

	mutex_lock(poll_loop->poll_ctx_list_mutex);

	if(!list_empty(&poll_loop->poll_ctx_list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &poll_loop->poll_ctx_list) {
			list_del(pos);
		}
	}

	mutex_unlock(poll_loop->poll_ctx_list_mutex);

	mutex_delete(poll_loop->poll_ctx_list_mutex);

	os_free(poll_loop);
}

static poll_loop_t *alloc_poll_loop(uint8_t id)
{
	poll_loop_t *poll_loop = NULL;

	poll_loop = (poll_loop_t *)os_alloc(sizeof(poll_loop_t));

	if(poll_loop == NULL) {
		return poll_loop;
	}

	memset(poll_loop, 0, sizeof(poll_loop_t));

	poll_loop->id = id;
	INIT_LIST_HEAD(&poll_loop->poll_ctx_list);

	poll_loop->poll_ctx_list_mutex = mutex_create();

	if(poll_loop->poll_ctx_list_mutex == NULL) {
		goto failed;
	}

	osThreadDef(poll_loop, task_poll_loop, osPriorityNormal, 0, 128 * 2 * 8);
	osThreadCreate(osThread(poll_loop), poll_loop);

	return poll_loop;

failed:
	free_poll_loop(poll_loop);
	poll_loop = NULL;
	return poll_loop;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	poll_loop_t *poll_loop = (poll_loop_t *)o;
	uint32_t id = (uint32_t)ctx;

	if(poll_loop->id == id) {
		ret = 0;
	}

	return ret;
}

poll_loop_t *get_or_alloc_poll_loop(uint32_t id)
{
	poll_loop_t *poll_loop = NULL;

	os_enter_critical();

	if(poll_loop_class == NULL) {
		poll_loop_class = object_class_alloc();
	}

	os_leave_critical();

	poll_loop = (poll_loop_t *)object_class_get_or_alloc_object(poll_loop_class, object_filter, (void *)id, (object_alloc_t)alloc_poll_loop, (object_free_t)free_poll_loop);

	return poll_loop;
}

