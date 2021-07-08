

/*================================================================
 *
 *
 *   文件名称：net_protocol_udp.c
 *   创 建 者：肖飞
 *   创建日期：2020年02月17日 星期一 14时41分21秒
 *   修改日期：2021年05月25日 星期二 20时39分49秒
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

#include "sal_hook.h"

#include "log.h"

static int udp_client_connect(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	socket_addr_info_t *socket_addr_info = net_client_info->net_client_addr_info.socket_addr_info;

	ret = socket_nonblock_connect(socket_addr_info, &net_client_info->sock_fd);
	debug("ret %d", ret);

	return ret;
}

static int udp_client_recv(void *ctx, void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	socket_addr_info_t *socket_addr_info = net_client_info->net_client_addr_info.socket_addr_info;

	return recvfrom(net_client_info->sock_fd, buf, len, 0, (struct sockaddr *)&socket_addr_info->addr, &socket_addr_info->addr_size);
}

static int udp_client_send(void *ctx, const void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	socket_addr_info_t *socket_addr_info = net_client_info->net_client_addr_info.socket_addr_info;

	return sendto(net_client_info->sock_fd, buf, len, 0, (struct sockaddr *)&socket_addr_info->addr, socket_addr_info->addr_size);
}

static int udp_client_close(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(net_client_info->sock_fd == -1) {
		return ret;
	}

	ret = close(net_client_info->sock_fd);
	debug("close socket %d(%d)", net_client_info->sock_fd, ret);
	net_client_info->sock_fd = -1;

	return ret;
}

protocol_if_t protocol_if_udp = {
	.type = PROTOCOL_UDP,
	.net_connect = udp_client_connect,
	.net_recv = udp_client_recv,
	.net_send = udp_client_send,
	.net_close = udp_client_close,
};
