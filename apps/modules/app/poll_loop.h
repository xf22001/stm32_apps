

/*================================================================
 *   
 *   
 *   文件名称：poll_loop.h
 *   创 建 者：肖飞
 *   创建日期：2020年08月11日 星期二 09时54分24秒
 *   修改日期：2020年08月11日 星期二 17时05分32秒
 *   描    述：
 *
 *================================================================*/
#ifndef _POLL_LOOP_H
#define _POLL_LOOP_H
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
	uint32_t poll_in : 1;	
	uint32_t poll_out : 1;	
	uint32_t poll_err : 1;	
} poll_mask_t;

typedef union {
	poll_mask_t s;
	uint32_t v;
} u_poll_mask_t;

typedef struct {
	int fd;
	uint8_t available;
	u_poll_mask_t config;
	u_poll_mask_t status;
} poll_fd_t;

typedef void (*poll_handler_t)(void *ctx);
typedef void (*poll_periodic_t)(void *ctx);

typedef struct {
	struct list_head list;
	poll_fd_t poll_fd;
	void *priv;
	char *name;
	poll_handler_t poll_handler;
	poll_periodic_t poll_periodic;
} poll_ctx_t;

typedef struct {
	struct list_head list;

	uint8_t id;
	osMutexId poll_ctx_list_mutex;
	struct list_head poll_ctx_list;
} poll_loop_t;

void free_poll_loop(poll_loop_t *poll_loop);
poll_loop_t *get_or_alloc_poll_loop(uint8_t id);
poll_ctx_t *alloc_poll_ctx(void);
int add_poll_loop_ctx_item(poll_loop_t *poll_loop, poll_ctx_t *poll_ctx);
int remove_poll_loop_ctx_item(poll_loop_t *poll_loop, poll_ctx_t *poll_ctx);
void task_poll_loop(void const *argument);

#endif //_POLL_LOOP_H
