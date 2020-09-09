

/*================================================================
 *   
 *   
 *   文件名称：global.h
 *   创 建 者：肖飞
 *   创建日期：2020年09月09日 星期三 09时06分39秒
 *   修改日期：2020年09月09日 星期三 15时22分55秒
 *   描    述：
 *
 *================================================================*/
#ifndef _GLOBAL_H
#define _GLOBAL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "os_utils.h"
#include "list_utils.h"

#include "global_cls.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	struct list_head list;
	struct list_head key_list;
	uint32_t cls;
} global_cls_t;

typedef struct {
	struct list_head list;
	void *key;
	void *value;
} global_key_t;

void global_init(void);
void *global_get_class_key_value(uint32_t cls, void *key);
int global_set_class_key_value(uint32_t cls, void *key, void *value, uint8_t replace);
int global_delete_class_key(uint32_t cls, void *key);
void global_dump_class_key_value(void);

#endif //_GLOBAL_H
