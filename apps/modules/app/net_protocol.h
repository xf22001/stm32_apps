

/*================================================================
 *   
 *   
 *   文件名称：net_protocol.h
 *   创 建 者：肖飞
 *   创建日期：2020年02月17日 星期一 14时36分47秒
 *   修改日期：2020年06月10日 星期三 10时41分33秒
 *   描    述：
 *
 *================================================================*/
#ifndef _NET_PROTOCOL_H
#define _NET_PROTOCOL_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

typedef enum {
	TRANS_PROTOCOL_TCP = 0,
	TRANS_PROTOCOL_UDP,
	TRANS_PROTOCOL_WS,
} trans_protocol_type_t;

typedef int (*connect_t)(void *ctx);
typedef int (*recv_t)(void *ctx, void *buf, size_t len);
typedef int (*send_t)(void *ctx, const void *buf, size_t len);
typedef int (*close_t)(void *ctx);
typedef struct {
	char *name;
	trans_protocol_type_t type;
	connect_t net_connect;
	recv_t net_recv;
	send_t net_send;
	close_t net_close;
} protocol_if_t;

#endif //_NET_PROTOCOL_H
