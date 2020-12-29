

/*================================================================
 *   
 *   
 *   文件名称：test_sys_class.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 16时03分50秒
 *   修改日期：2020年12月29日 星期二 16时21分15秒
 *   描    述：
 *
 *================================================================*/
#include "test_sys_class.h"
#include "sys_class.h"
#include "log.h"

#define test_register(v) do { \
	int ret = sys_class_info_register(sys_class_info_alloc((void *)v, "%d", v)); \
	debug("register %d return:%d\n", v, ret); \
} while(0)

#define test_find(v) do { \
	sys_class_info_t *sys_class_info = sys_class_info_find("%d", v); \
	debug("find %d return:%d\n", v, (int)sys_class_info->ctx); \
} while(0)

#define test_unregister(v) do { \
	debug("unregister %d return:%d\n", v, sys_class_info_unregister(sys_class_info_find("%d", v))); \
} while(0)

void test_sys_class(void)
{
	sys_class_init();

	test_register(1);
	test_register(2);
	test_register(3);
	test_register(4);
	test_register(5);
	test_register(6);
	test_register(7);
	test_register(8);
	test_register(9);
	test_register(10);
	test_register(11);

	test_find(1);
	test_find(2);
	test_find(3);
	test_find(4);
	test_find(5);
	test_find(6);
	test_find(7);
	test_find(8);
	test_find(9);
	test_find(10);
	test_find(11);

	test_unregister(4);
	test_unregister(5);
	test_unregister(6);
	test_unregister(7);
	test_unregister(8);

	test_find(1);
	test_find(2);
	test_find(3);
	test_find(4);
	test_find(5);
	test_find(6);
	test_find(7);
	test_find(8);
	test_find(9);
	test_find(10);
	test_find(11);

	sys_classs_uninit();
}
