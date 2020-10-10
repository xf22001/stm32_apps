

/*================================================================
 *
 *
 *   文件名称：net_protocol_tcp.c
 *   创 建 者：肖飞
 *   创建日期：2020年02月17日 星期一 14时39分04秒
 *   修改日期：2020年10月10日 星期六 08时59分48秒
 *   描    述：
 *
 *================================================================*/
#include "app_platform.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/dhcp.h"
#include "main.h"

#include "os_utils.h"
#include "net_client.h"
#include "net_protocol.h"

#include "log.h"

static int tcp_client_connect(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	socket_addr_info_t *socket_addr_info = net_client_info->net_client_addr_info.socket_addr_info;

	ret = socket_nonblock_connect(socket_addr_info, &net_client_info->sock_fd);

	return ret;
}

static int tcp_client_recv(void *ctx, void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	return recv(net_client_info->sock_fd, buf, len, 0);
}

static int tcp_client_send(void *ctx, const void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	return send(net_client_info->sock_fd, buf, len, 0);
}

static int tcp_client_close(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(net_client_info->sock_fd == -1) {
		return ret;
	}

	debug("close socket %d\n", net_client_info->sock_fd);

	ret = close(net_client_info->sock_fd);
	net_client_info->sock_fd = -1;

	return ret;
}

protocol_if_t protocol_if_tcp = {
	.name = "tcp",
	.type = TRANS_PROTOCOL_TCP,
	.net_connect = tcp_client_connect,
	.net_recv = tcp_client_recv,
	.net_send = tcp_client_send,
	.net_close = tcp_client_close,
};

