

/*================================================================
 *   
 *   
 *   文件名称：net_utils.h
 *   创 建 者：肖飞
 *   创建日期：2020年09月15日 星期二 10时07分27秒
 *   修改日期：2020年09月15日 星期二 14时31分50秒
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
int socket_connect_confirm(int fd);

#endif //_NET_UTILS_H
