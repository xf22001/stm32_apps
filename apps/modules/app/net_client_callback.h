

/*================================================================
 *   
 *   
 *   文件名称：net_client_callback.h
 *   创 建 者：肖飞
 *   创建日期：2020年06月10日 星期三 10时56分57秒
 *   修改日期：2020年06月10日 星期三 11时02分35秒
 *   描    述：
 *
 *================================================================*/
#ifndef _NET_CLIENT_CALLBACK_H
#define _NET_CLIENT_CALLBACK_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "net_client.h"
#include "net_protocol.h"

#ifdef __cplusplus
}
#endif
void set_request_callback(request_callback_t *request_callback);
request_callback_t *get_request_callback(void);
protocol_if_t *set_protocol_if(protocol_if_t *protocol_if);
protocol_if_t *get_protocol_if(void);

#endif //_NET_CLIENT_CALLBACK_H
