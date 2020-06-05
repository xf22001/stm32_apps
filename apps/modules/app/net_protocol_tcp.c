

/*================================================================
 *   
 *   
 *   文件名称：net_protocol_tcp.c
 *   创 建 者：肖飞
 *   创建日期：2020年02月17日 星期一 14时39分04秒
 *   修改日期：2020年06月05日 星期五 15时42分01秒
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

	net_client_info->sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(net_client_info->sock_fd == -1) {
		return ret;
	}

	ret = connect(net_client_info->sock_fd, (struct sockaddr *)(&net_client_info->addr_in), sizeof(struct sockaddr_in));

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

	ret = close(net_client_info->sock_fd);
	net_client_info->sock_fd = -1;

	return ret;
}

protocol_if_t protocol_if_tcp = {
	.name = "tcp",
	.net_connect = tcp_client_connect,
	.net_recv = tcp_client_recv,
	.net_send = tcp_client_send,
	.net_close = tcp_client_close,
};

