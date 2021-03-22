

/*================================================================
 *
 *
 *   文件名称：net_utils.c
 *   创 建 者：肖飞
 *   创建日期：2020年09月15日 星期二 09时42分47秒
 *   修改日期：2021年01月29日 星期五 16时11分21秒
 *   描    述：
 *
 *================================================================*/
#include "net_utils.h"

#include <string.h>

#include "os_utils.h"
#include "log.h"

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

		debug("free addr family:%d, socktype:%d, protocol:%d", entry->ai_family, entry->ai_socktype, entry->ai_socktype);

		os_free(entry);
	}

	/* Do name resolution with both IPv6 and IPv4 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socktype;
	hints.ai_protocol = protocol;

	if(getaddrinfo(host, port, &hints, &addr_list) != 0) {
		debug("");
		return ret;
	}

	for(cur = addr_list; cur != NULL; cur = cur->ai_next) {
		socket_addr_info_t *socket_addr_info = (socket_addr_info_t *)os_alloc(sizeof(socket_addr_info_t));

		if(socket_addr_info == NULL) {
			debug("");
			break;
		}

		socket_addr_info->ai_family = cur->ai_family;
		socket_addr_info->ai_socktype = cur->ai_socktype;
		socket_addr_info->ai_protocol = cur->ai_protocol;
		socket_addr_info->addr_size = cur->ai_addrlen;
		memset(&socket_addr_info->addr, 0, sizeof(socket_addr_info->addr));
		memcpy(&socket_addr_info->addr, cur->ai_addr, cur->ai_addrlen);

		debug("add addr family:%d, socktype:%d, protocol:%d", socket_addr_info->ai_family, socket_addr_info->ai_socktype, socket_addr_info->ai_socktype);

		if(socket_addr_info->ai_family == AF_INET) {
			struct sockaddr_in *sockaddr_in = (struct sockaddr_in *)&socket_addr_info->addr;
			debug("ip:%s, port:%d",
			      inet_ntoa(sockaddr_in->sin_addr),
			      ntohs(sockaddr_in->sin_port)
			     );
		} else {
			//struct sockaddr_in6 *sockaddr_in6 = (struct sockaddr_in6 *)&socket_addr_info->addr;

			//debug("ip:%s, port:%d",
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
		debug("");
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

int socket_nonblock_connect(socket_addr_info_t *socket_addr_info, int *sock_fd)
{
	int ret = -1;
	int flags = 0;

	*sock_fd = -1;

	if(socket_addr_info == NULL) {
		debug("");
		return ret;
	}

	*sock_fd = socket(socket_addr_info->ai_family, socket_addr_info->ai_socktype, socket_addr_info->ai_protocol);

	if(*sock_fd == -1) {
		debug("");
		return ret;
	}

	debug("create socket %d", *sock_fd);

	flags = fcntl(*sock_fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(*sock_fd, F_SETFL, flags);

	ret = connect(*sock_fd, (struct sockaddr *)&socket_addr_info->addr, socket_addr_info->addr_size);

	if(ret != 0) {
		if(errno != EINPROGRESS) {
			debug("close socket %d(%d)", *sock_fd, errno);
			close(*sock_fd);
			*sock_fd = -1;
		} else {
			ret = 0;
		}
	}

	return ret;
}

int poll_wait_nonblock_connect_event(int fd, uint32_t timeout)
{
	int ret = -1;
	struct fd_set wfds;
	struct fd_set efds;
	struct timeval tv;
	int max_fd = -1;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = 1000 * (timeout % 1000);

	if(fd == -1) {
		debug("socket fd is not valid!");
		return ret;
	}

	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);

	FD_ZERO(&efds);
	FD_SET(fd, &efds);

	if(fd > max_fd) {
		max_fd = fd;
	}

	max_fd += 1;

	ret = select(max_fd, NULL, &wfds, &efds, &tv);

	if(ret >= 0) {
		if(FD_ISSET(fd, &wfds)) {
			ret = 0;
		} else if(FD_ISSET(fd, &efds)) {
			ret = 0;
		} else {
			ret = 1;
		}

	}

	return ret;
}

int socket_nonblock_connect_confirm(int fd)
{
	int ret;
	int opt;
	socklen_t slen = sizeof(int);

	ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, (void *)&opt, &slen);

	if(ret == 0) {
		if(opt == 0) {
			debug("connect success!");
		} else {
			ret = -1;
			debug("connect failed!(%d)", opt);
		}
	} else {
		debug("connect failed!(%d)", errno);
	}

	return ret;
}

int poll_wait_read_available(int fd, uint32_t timeout)
{
	int ret = -1;
	struct fd_set fds;
	struct timeval tv;
	int max_fd = -1;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = 1000 * (timeout % 1000);

	if(fd == -1) {
		debug("socket fd is not valid!");
		return ret;
	}

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if(fd > max_fd) {
		max_fd = fd;
	}

	max_fd += 1;

	ret = select(max_fd, &fds, NULL, NULL, &tv);

	if(ret >= 0) {
		if(FD_ISSET(fd, &fds)) {
			ret = 0;
		} else {
			ret = 1;
		}

	}

	return ret;
}

int poll_wait_write_available(int fd, uint32_t timeout)
{
	int ret = -1;
	struct fd_set fds;
	struct timeval tv;
	int max_fd = -1;

	tv.tv_sec = timeout / 1000;
	tv.tv_usec = 1000 * (timeout % 1000);

	if(fd == -1) {
		debug("socket fd is not valid!");
		return ret;
	}

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	if(fd > max_fd) {
		max_fd = fd;
	}

	max_fd += 1;

	ret = select(max_fd, NULL, &fds, NULL, &tv);

	if(ret >= 0) {
		if(FD_ISSET(fd, &fds)) {
			ret = 0;
		} else {
			ret = 1;
		}

	}

	return ret;
}
