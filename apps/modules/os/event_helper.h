

/*================================================================
 *   
 *   
 *   文件名称：event_helper.h
 *   创 建 者：肖飞
 *   创建日期：2020年01月07日 星期二 09时56分07秒
 *   修改日期：2021年01月30日 星期六 08时08分06秒
 *   描    述：
 *
 *================================================================*/
#ifndef _EVENT_HELPER_H
#define _EVENT_HELPER_H
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
	struct list_head list_head;
	void *event;
} event_item_t;

typedef struct {
	os_mutex_t mutex;//保护消息链数据
	os_signal_t queue;
	struct list_head list_event;
} event_pool_t;

void free_event_pool(event_pool_t *event_pool);
event_pool_t *alloc_event_pool(void);
int event_pool_put_event(event_pool_t *event_pool, void *event, uint32_t timeout);
int event_pool_wait_event(event_pool_t *event_pool, uint32_t timeout);
void *event_pool_get_event(event_pool_t *event_pool);

#endif //_EVENT_HELPER_H
