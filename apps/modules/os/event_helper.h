

/*================================================================
 *   
 *   
 *   文件名称：event_helper.h
 *   创 建 者：肖飞
 *   创建日期：2020年01月07日 星期二 09时56分07秒
 *   修改日期：2020年03月24日 星期二 10时45分13秒
 *   描    述：
 *
 *================================================================*/
#ifndef _EVENT_HELPER_H
#define _EVENT_HELPER_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "list_utils.h"

typedef struct {
	struct list_head list_head;
	void *event;
} event_item_t;

typedef struct {
	osMutexId mutex;//保护消息链数据
	osMessageQId queue;
	struct list_head list_event;
} event_pool_t;

event_pool_t *alloc_event_pool(void);
void free_event_pool(event_pool_t *event_pool);
int event_pool_put_event(event_pool_t *event_pool, void *event, uint32_t timeout);
int event_pool_wait_event(event_pool_t *event_pool, uint32_t timeout);
void *event_pool_get_event(event_pool_t *event_pool);

#endif //_EVENT_HELPER_H