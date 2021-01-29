

/*================================================================
 *
 *
 *   文件名称：test_event.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 08时47分25秒
 *   修改日期：2021年01月29日 星期五 16时14分43秒
 *   描    述：
 *
 *================================================================*/
#include "test_event.h"
#include <string.h>
#include "event_helper.h"
#include "os_utils.h"
#include "log.h"

typedef struct {
	uint32_t count;
} test_event_t;

static event_pool_t *event_pool = NULL;

void try_get_test_event()
{
	int ret;
	test_event_t *test_event;

	ret = event_pool_wait_event(event_pool, 0);

	if(ret != 0) {
		debug("no event!\n");
		return;
	}

	test_event = (test_event_t *)event_pool_get_event(event_pool);

	if(test_event == NULL) {
		debug("get event error!\n");
		return;
	}

	debug("get event %d!\n", test_event->count);
}

static void task_test_event(void const *argument)
{
	int ret = 0;
	uint32_t count = 0;

	if(event_pool == NULL) {
		event_pool = alloc_event_pool();

		if(event_pool == NULL) {
			app_panic();
		}
	}

	while(1) {
		test_event_t *test_event = os_alloc(sizeof(test_event_t));

		if(test_event == NULL) {
			debug("os_alloc error!\n");
			continue;
		}

		memset(test_event, 0, sizeof(test_event_t));
		test_event->count = count;

		ret = event_pool_put_event(event_pool, test_event, osWaitForever);

		if(ret != 0) {
			debug("event_pool_put_event error!\n");
			os_free(test_event);
		}

		debug("sent event %d!\n", test_event->count);
		count++;
	}
}

void test_event(void)
{
	osThreadDef(test_event, task_test_event, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(test_event), NULL);
}
