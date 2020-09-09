

/*================================================================
 *   
 *   
 *   文件名称：config_list.h
 *   创 建 者：肖飞
 *   创建日期：2020年09月09日 星期三 16时46分30秒
 *   修改日期：2020年09月09日 星期三 16时46分58秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CONFIG_LIST_H
#define _CONFIG_LIST_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "os_utils.h"
#include "list_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	struct list_head list;
	struct list_head key_list;
	uint32_t cls;
} config_cls_t;

typedef struct {
	struct list_head list;
	uint32_t key;
	void *value;
} config_key_t;

void config_init(void);
void *config_get_class_key_value(uint32_t cls, uint32_t key);
int config_set_class_key_value(uint32_t cls, uint32_t key, void *value);
int config_delete_class_key(uint32_t cls, uint32_t key);
void config_dump_class_key_value(void);

#endif //_CONFIG_LIST_H
