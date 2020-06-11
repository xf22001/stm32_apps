

/*================================================================
 *
 *
 *   文件名称：net_client_callback.c
 *   创 建 者：肖飞
 *   创建日期：2020年06月10日 星期三 10时48分33秒
 *   修改日期：2020年06月11日 星期四 09时08分19秒
 *   描    述：
 *
 *================================================================*/
#include "net_client_callback.h"
#include "app.h"

extern request_callback_t request_callback_environment_monitor;
extern request_callback_t request_callback_ws;
static request_callback_t *net_client_request_callback = &request_callback_environment_monitor;
//static request_callback_t *net_client_request_callback = &request_callback_ws;

extern protocol_if_t protocol_if_tcp;
extern protocol_if_t protocol_if_ws;
static protocol_if_t *net_client_protocol_if = &protocol_if_tcp;
//static protocol_if_t *net_client_protocol_if = &protocol_if_ws;


void set_request_callback(request_callback_t *request_callback)
{
	if(request_callback != NULL) {
		net_client_request_callback = request_callback;
	}
}

request_callback_t *get_request_callback(void)
{
	return net_client_request_callback;
}

void set_protocol_if(protocol_if_t *protocol_if)
{
	net_client_protocol_if = protocol_if;
}

protocol_if_t *get_protocol_if(void)
{
	return net_client_protocol_if;
}

int get_addr_host_port_service(char **host, char **port, char **path)
{
	int ret = 0;

	*host = "192.168.1.128";
	*port = "6003";
	*path = "/";

	return ret;
}
