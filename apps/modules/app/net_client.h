

/*================================================================
 *
 *
 *   文件名称：net_client.h
 *   创 建 者：肖飞
 *   创建日期：2019年09月04日 星期三 08时38分02秒
 *   修改日期：2020年07月30日 星期四 09时00分03秒
 *   描    述：
 *
 *================================================================*/
#ifndef _NET_CLIENT_H
#define _NET_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"

#include "net_protocol.h"
#include "list_utils.h"

#define TASK_NET_CLIENT_PERIODIC (100) //ms
#define TASK_NET_CLIENT_CONNECT_PERIODIC (1000 * 1) //ms
#define NET_MESSAGE_BUFFER_SIZE (1024 * 2)

typedef enum {
	CLIENT_DISCONNECT = 0,
	CLIENT_CONNECTING,
	CLIENT_CONNECTED,
	CLIENT_RESET,
} client_state_t;

typedef struct {
	struct list_head list;
	int ai_family;/* Address family of socket. */
	int ai_socktype;/* Socket type. */
	int ai_protocol;/* Protocol of socket. */
	struct sockaddr_storage addr;
	socklen_t addr_size;
} socket_addr_info_t;

typedef struct {
	char host[256];
	char port[8];
	char path[256];
	struct list_head socket_addr_info_list;
	socket_addr_info_t *socket_addr_info;
} net_client_addr_info_t;

typedef struct {
	uint16_t used;
	uint8_t buffer[NET_MESSAGE_BUFFER_SIZE];
} net_message_buffer_t;

typedef void (*set_lan_led_state_t)(uint32_t state);
typedef void (*init_t)(void);
typedef void (*before_connect_t)(void);
typedef void (*after_connect_t)(void);
typedef void (*before_close_t)(void);
typedef void (*after_close_t)(void);
typedef void (*parse_t)(char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size);
typedef void (*process_t)(uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef void (*periodic_t)(uint8_t *send_buffer, uint16_t send_buffer_size);
typedef struct {
	char *name;
	set_lan_led_state_t set_lan_led_state;
	init_t init;
	before_connect_t before_connect;
	after_connect_t after_connect;
	before_close_t before_close;
	after_close_t after_close;
	parse_t parse;
	process_t process;
	periodic_t periodic;
} request_callback_t;

typedef struct {
	int sock_fd;
	uint32_t connect_id;
	uint32_t retry_count;
	uint32_t connect_stamp;
	uint8_t reset_connect;
	client_state_t state;
	trans_protocol_type_t trans_protocol_type;
	net_client_addr_info_t net_client_addr_info;
	net_message_buffer_t recv_message_buffer;
	net_message_buffer_t send_message_buffer;
	request_callback_t *request_callback;
	protocol_if_t *protocol_if;
} net_client_info_t;

trans_protocol_type_t get_net_client_protocol(void);
void set_net_client_protocol(trans_protocol_type_t type);
void set_client_state(client_state_t state);
client_state_t get_client_state(void);
uint32_t get_net_client_connect_id(void);
int send_to_server(uint8_t *buffer, size_t len);
void task_net_client(void const *argument);
#endif //_NET_CLIENT_H
