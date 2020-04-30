

/*================================================================
 *   
 *   
 *   文件名称：channel.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月30日 星期四 08时56分47秒
 *   修改日期：2020年04月30日 星期四 08时57分40秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_H
#define _CHANNEL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "list_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	struct list_head list;
	uint8_t id;
} channel_info_t;


#endif //_CHANNEL_H
