

/*================================================================
 *
 *
 *   文件名称：net_utils.c
 *   创 建 者：肖飞
 *   创建日期：2020年09月15日 星期二 09时42分47秒
 *   修改日期：2020年09月15日 星期二 14时31分41秒
 *   描    述：
 *
 *================================================================*/
#include "net_utils.h"
#include "log.h"

#include <string.h>

int update_addr_info_list(struct list_head *list_head, const char *host, const char *port, int socktype, int protocol)
{
	int ret = -1;
	struct addrinfo hints;
	struct addrinfo *addr_list;
	struct addrinfo *cur;
	struct list_head *pos;
	struct list_head *n;

	list_for_each_safe(pos, n, list_head) {
		socket_addr_info_t *entry = list_entry(pos, socket_addr_info_t, list);

		list_del(pos);

		debug("free addr family:%d, socktype:%d, protocol:%d\n", entry->ai_family, entry->ai_socktype, entry->ai_socktype);

		os_free(entry);
	}

	/* Do name resolution with both IPv6 and IPv4 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;

	if(getaddrinfo(host, port, &hints, &addr_list) != 0) {
		debug("\n");
		return ret;
	}

	for(cur = addr_list; cur != NULL; cur = cur->ai_next) {
		socket_addr_info_t *socket_addr_info = (socket_addr_info_t *)os_alloc(sizeof(socket_addr_info_t));

		if(socket_addr_info == NULL) {
			debug("\n");
			break;
		}

		socket_addr_info->ai_family = cur->ai_family;
		socket_addr_info->ai_socktype = cur->ai_socktype;
		socket_addr_info->ai_protocol = cur->ai_protocol;
		socket_addr_info->addr_size = cur->ai_addrlen;
		memset(&socket_addr_info->addr, 0, sizeof(socket_addr_info->addr));
		memcpy(&socket_addr_info->addr, cur->ai_addr, cur->ai_addrlen);

		debug("add addr family:%d, socktype:%d, protocol:%d\n", socket_addr_info->ai_family, socket_addr_info->ai_socktype, socket_addr_info->ai_socktype);

		if(socket_addr_info->ai_family == AF_INET) {
			struct sockaddr_in *sockaddr_in = (struct sockaddr_in *)&socket_addr_info->addr;
			debug("ip:%s, port:%d\n",
			      inet_ntoa(sockaddr_in->sin_addr),
			      ntohs(sockaddr_in->sin_port)
			     );
		} else {
			//struct sockaddr_in6 *sockaddr_in6 = (struct sockaddr_in6 *)&socket_addr_info->addr;

			//debug("ip:%s, port:%d\n",
			//      inet6_ntoa(sockaddr_in6->sin6_addr),
			//      ntohs(sockaddr_in6->sin6_port)
			//     );
		}

		list_add_tail(&socket_addr_info->list, list_head);
	}

	freeaddrinfo(addr_list);

	if(!list_empty(list_head)) {
		ret = 0;
	} else {
		debug("\n");
	}

	return ret;
}


socket_addr_info_t *get_next_socket_addr_info(struct list_head *list_head, socket_addr_info_t *node)
{
	if(list_empty(list_head)) {
		return NULL;
	}

	if(node == NULL) {
		return list_first_entry(list_head, socket_addr_info_t, list);
	}

	if(node != list_last_entry(list_head, socket_addr_info_t, list)) {
		return list_next_entry(node, socket_addr_info_t, list);
	}

	return NULL;
}

int socket_connect_confirm(int fd)
{
	int ret = -1;
	int opt;
	socklen_t slen = sizeof(int);

	ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&opt, &slen);

	if(ret == 0) {
		if(opt == 0) {
			debug("connect success!\n");
		} else {
			ret = -1;
			debug("connect failed!(%d)\n", opt);
		}
	} else {
		debug("connect failed!(%d)\n", errno);
	}

	return ret;
}
