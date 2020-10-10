

/*================================================================
 *   
 *   
 *   文件名称：net_utils.h
 *   创 建 者：肖飞
 *   创建日期：2020年09月15日 星期二 10时07分27秒
 *   修改日期：2020年10月10日 星期六 08时54分37秒
 *   描    述：
 *
 *================================================================*/
#ifndef _NET_UTILS_H
#define _NET_UTILS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "list_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	struct list_head list;
	int ai_family;/* Address family of socket. */
	int ai_socktype;/* Socket type. */
	int ai_protocol;/* Protocol of socket. */
	struct sockaddr_storage addr;
	socklen_t addr_size;
} socket_addr_info_t;

int update_addr_info_list(struct list_head *list_head, const char *host, const char *port, int socktype, int protocol);
socket_addr_info_t *get_next_socket_addr_info(struct list_head *list_head, socket_addr_info_t *node);
int socket_nonblock_connect(socket_addr_info_t *socket_addr_info, int *sock_fd);
int poll_wait_nonblock_connect_event(int fd, uint32_t timeout);
int socket_nonblock_connect_confirm(int fd);
int poll_wait_read_available(int fd, uint32_t timeout);
int poll_wait_write_available(int fd, uint32_t timeout);

#endif //_NET_UTILS_H
