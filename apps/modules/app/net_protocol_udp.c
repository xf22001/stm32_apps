

/*================================================================
 *
 *
 *   文件名称：net_protocol_udp.c
 *   创 建 者：肖飞
 *   创建日期：2020年02月17日 星期一 14时41分21秒
 *   修改日期：2020年02月17日 星期一 14时56分08秒
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
#include "task_probe_tool.h"

static int udp_client_connect(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	net_client_info->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(net_client_info->sock_fd != -1) {
		ret = 0;
	}

	return ret;
}

static int udp_client_recv(void *ctx, void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	socklen_t socklen = sizeof(struct sockaddr_in);

	return recvfrom(net_client_info->sock_fd, buf, len, 0, (struct sockaddr *)(&net_client_info->addr_in), &socklen);
}

static int udp_client_send(void *ctx, const void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	return sendto(net_client_info->sock_fd, buf, len, 0, (struct sockaddr *)(&net_client_info->addr_in), sizeof(struct sockaddr_in));
}

static int udp_client_close(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	ret = close(net_client_info->sock_fd);
	net_client_info->sock_fd = -1;

	return ret;
}

protocol_if_t protocol_if_udp = {
	.net_connect = udp_client_connect,
	.net_recv = udp_client_recv,
	.net_send = udp_client_send,
	.net_close = udp_client_close,
};

