

/*================================================================
 *   
 *   
 *   文件名称：channel.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月29日 星期五 16时34分57秒
 *   修改日期：2020年05月30日 星期六 09时31分57秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_H
#define _CHANNEL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "channels.h"

#ifdef __cplusplus
}
#endif

void set_channel_request_state(channel_info_t *channel_info, channel_request_state_t channel_request_state);
channel_request_state_t get_channel_request_state(channel_info_t *channel_info);
channel_callback_t *get_channel_callback(uint8_t channel_id);
#endif //_CHANNEL_H
