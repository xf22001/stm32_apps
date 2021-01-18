

/*================================================================
 *   
 *   
 *   文件名称：test_map_utils.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 15时30分46秒
 *   修改日期：2021年01月18日 星期一 09时53分23秒
 *   描    述：
 *
 *================================================================*/
#include "test_map_utils.h"
#include "map_utils.h"
#include "log.h"

#define add_key_value(map, key, value) do { \
	debug("add key:%p, value:%p, return:%d\n", (void *)key, (void *)value, map_utils_add_key_value(map, (void *)key, (void *)value)); \
} while(0)

#define get_key_value(map, key) do { \
	debug("get key:%p, return:%p\n", (void *)key, map_utils_get_value(map, (void *)key)); \
} while(0)

#define remove_key_value(map, key) do { \
	debug("remove key:%p, return:%d\n", (void *)key, map_utils_remove_value(map, (void *)key)); \
} while(0)

void test_map_utils(void)
{
	map_utils_t *map_utils;

	__disable_irq();
	map_utils = map_utils_alloc(NULL);
	__enable_irq();

	add_key_value(map_utils, 2, 2);
	add_key_value(map_utils, 3, 3);
	add_key_value(map_utils, 4, 4);
	add_key_value(map_utils, 5, 5);
	add_key_value(map_utils, 6, 6);
	add_key_value(map_utils, 7, 7);
	add_key_value(map_utils, 8, 8);

	remove_key_value(map_utils, 2);
	remove_key_value(map_utils, 3);
	remove_key_value(map_utils, 4);
	remove_key_value(map_utils, 8);

	get_key_value(map_utils, 2);
	get_key_value(map_utils, 3);
	get_key_value(map_utils, 4);
	get_key_value(map_utils, 5);
	get_key_value(map_utils, 6);
	get_key_value(map_utils, 7);
	get_key_value(map_utils, 8);

	map_utils_free(map_utils);
}
