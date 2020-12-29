

/*================================================================
 *   
 *   
 *   文件名称：sys_class.h
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 11时05分13秒
 *   修改日期：2020年12月29日 星期二 13时50分01秒
 *   描    述：
 *
 *================================================================*/
#ifndef _SYS_CLASS_H
#define _SYS_CLASS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "list_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uint8_t init;
	struct list_head list;
	osMutexId mutex;
} sys_class_t;

typedef int (*sys_class_info_init_t)(void *ctx);
typedef void (*sys_class_info_uninit_t)(void *ctx);

#define SYS_CLASS_NAME_SIZE 32
typedef struct {
	char name[SYS_CLASS_NAME_SIZE];
	uint32_t hash;

	void *ctx;
	struct list_head list;

	sys_class_info_init_t init;
	sys_class_info_uninit_t uninit;
} sys_class_info_t;

int sys_class_init(void);
void sys_classs_uninit(void);
sys_class_info_t *sys_class_info_alloc(void *ctx, const char *fmt, ...);
sys_class_info_t *sys_class_info_find(const char *fmt, ...);
int sys_class_info_register(sys_class_info_t *info);
int sys_class_info_unregister(sys_class_info_t *info);
#endif //_SYS_CLASS_H
