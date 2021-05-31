

/*================================================================
 *   
 *   
 *   文件名称：channel.h
 *   创 建 者：肖飞
 *   创建日期：2021年04月08日 星期四 09时51分16秒
 *   修改日期：2021年05月31日 星期一 10时17分25秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_H
#define _CHANNEL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "channels.h"

#ifdef __cplusplus
}
#endif

char *get_channel_state_des(channel_state_t state);
int set_channel_request_state(channel_info_t *channel_info, channel_state_t state);
channel_info_t *alloc_channels_channel_info(channels_info_t *channels_info);

#endif //_CHANNEL_H
