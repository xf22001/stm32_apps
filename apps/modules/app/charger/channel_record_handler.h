

/*================================================================
 *   
 *   
 *   文件名称：channel_record_handler.h
 *   创 建 者：肖飞
 *   创建日期：2021年06月19日 星期六 12时12分20秒
 *   修改日期：2021年06月19日 星期六 12时29分58秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_RECORD_HANDLER_H
#define _CHANNEL_RECORD_HANDLER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "channel.h"

#ifdef __cplusplus
}
#endif

int channel_record_handler_init(channel_info_t *channel_info);

#endif //_CHANNEL_RECORD_HANDLER_H
