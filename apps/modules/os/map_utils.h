

/*================================================================
 *   
 *   
 *   文件名称：map_utils.h
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 12时22分56秒
 *   修改日期：2021年02月01日 星期一 15时52分59秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MAP_UTILS_H
#define _MAP_UTILS_H
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

typedef int (*key_match_t)(void *key1, void *key2);
typedef int (*key_filter_t)(void *key);
typedef int (*value_filter_t)(void *value);
typedef void *(*map_utils_value_alloc_t)(void *key);
typedef void *(*map_utils_value_free_t)(void *vlue);

typedef struct {
	struct list_head list;
	os_mutex_t mutex;
	key_match_t match;
} map_utils_t;

typedef struct {
	struct list_head list;
	void *key;
	void *value;
} map_utils_item_t;

void map_utils_free(map_utils_t *map_utils);
map_utils_t *map_utils_alloc(key_match_t match);
int map_utils_add_key_value(map_utils_t *map_utils, void *key, void *value);
void *map_utils_get_value(map_utils_t *map_utils, void *key);
int map_utils_remove_value(map_utils_t *map_utils, void *key);
int map_utils_get_keys(map_utils_t *map_utils, void **pkey, size_t *size, key_filter_t filter);
int map_utils_get_values(map_utils_t *map_utils, void **pvalue, size_t *size, value_filter_t filter);
size_t map_utils_get_size(map_utils_t *map_utils);
void *map_utils_get_or_alloc_value(map_utils_t *map_utils, void *key, map_utils_value_alloc_t map_utils_value_alloc, map_utils_value_free_t map_utils_value_free);
#endif //_MAP_UTILS_H
