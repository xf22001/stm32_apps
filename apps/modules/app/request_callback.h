

/*================================================================
 *   
 *   
 *   文件名称：request_callback.h
 *   创 建 者：肖飞
 *   创建日期：2020年06月09日 星期二 11时42分43秒
 *   修改日期：2020年06月10日 星期三 10时32分23秒
 *   描    述：
 *
 *================================================================*/
#ifndef _REQUEST_CALLBACK_H
#define _REQUEST_CALLBACK_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "net_client.h"

#ifdef __cplusplus
}
#endif

void set_request_callback(request_callback_t *callback);
request_callback_t *get_request_callback(void);
void set_net_client_protocol(trans_protocol_type_t type);
trans_protocol_type_t get_net_client_protocol(void);
#endif //_REQUEST_CALLBACK_H
