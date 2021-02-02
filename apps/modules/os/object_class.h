

/*================================================================
 *   
 *   
 *   文件名称：object_class.h
 *   创 建 者：肖飞
 *   创建日期：2021年02月02日 星期二 08时56分29秒
 *   修改日期：2021年02月02日 星期二 11时04分56秒
 *   描    述：
 *
 *================================================================*/
#ifndef _OBJECT_CLASS_H
#define _OBJECT_CLASS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "list_utils.h"
#include "os_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	struct list_head list;
	os_mutex_t mutex;
} object_class_t;

typedef void *(*object_alloc_t)(void *ctx);
typedef void (*object_free_t)(void *o);
typedef int (*object_filter_t)(void *o, void *ctx);

typedef struct {
	struct list_head list;
	void *o;
	object_free_t free;
} object_class_item_t;

void object_class_free(object_class_t *object_class);
object_class_t *object_class_alloc(void);
int object_class_add_object(object_class_t *object_class, object_filter_t filter, void *ctx, void *o, object_free_t object_free);
void *object_class_get_object(object_class_t *object_class, object_filter_t filter, void *ctx);
int object_class_get_objects(object_class_t *object_class, object_filter_t filter, void *ctx, void **po, size_t *size);
int object_class_remove_object(object_class_t *object_class, void *o);
size_t object_class_size(object_class_t *object_class);
void *object_class_get_or_alloc_object(object_class_t *object_class, object_filter_t filter, void *ctx, object_alloc_t object_alloc, object_free_t object_free);

#endif //_OBJECT_CLASS_H
