

/*================================================================
 *
 *
 *   文件名称：net_client.h
 *   创 建 者：肖飞
 *   创建日期：2019年09月04日 星期三 08时38分02秒
 *   修改日期：2021年07月10日 星期六 00时35分09秒
 *   描    述：
 *
 *================================================================*/
#ifndef _NET_CLIENT_H
#define _NET_CLIENT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"

#include "poll_loop.h"
#include "list_utils.h"
#include "net_utils.h"
#include "callback_chain.h"
#include "https.h"

#ifdef __cplusplus
}
#endif

#define TASK_NET_CLIENT_PERIODIC (100) //ms
#define TASK_NET_CLIENT_CONNECT_PERIODIC (1000 * 1) //ms
#define NET_MESSAGE_BUFFER_SIZE (1024 * 2)

typedef enum {
	CLIENT_DISCONNECT = 0,
	CLIENT_CONNECTING,
	CLIENT_CONNECT_CONFIRM,
	CLIENT_CONNECTED,
	CLIENT_RESET,
	CLIENT_REINIT,
	CLIENT_SUSPEND,
} client_state_t;

typedef enum {
	PROTOCOL_TCP = 0,
	PROTOCOL_UDP,
	PROTOCOL_TLS,
} protocol_type_t;

typedef struct {
	char scheme[8];
	char host[64];
	char port[8];
	char path[256];
	protocol_type_t protocol_type;

	struct list_head socket_addr_info_list;
	socket_addr_info_t *socket_addr_info;
} net_client_addr_info_t;

typedef struct {
	uint16_t used;
	uint8_t buffer[NET_MESSAGE_BUFFER_SIZE];
} net_message_buffer_t;

typedef int (*connect_t)(void *ctx);
typedef int (*recv_t)(void *ctx, void *buf, size_t len);
typedef int (*send_t)(void *ctx, const void *buf, size_t len);
typedef int (*close_t)(void *ctx);
typedef struct {
	protocol_type_t type;
	connect_t net_connect;
	recv_t net_recv;
	send_t net_send;
	close_t net_close;
} protocol_if_t;

typedef enum {
	REQUEST_TYPE_DEFAULT = 0,
	REQUEST_TYPE_SSE,
	REQUEST_TYPE_OCPP_1_6,
} request_type_t;

typedef void (*init_t)(void *ctx);
typedef void (*before_connect_t)(void *ctx);
typedef void (*after_connect_t)(void *ctx);
typedef void (*before_close_t)(void *ctx);
typedef void (*after_close_t)(void *ctx);
typedef void (*parse_t)(void *ctx, char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size);
typedef void (*process_t)(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef void (*periodic_t)(void *ctx, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef struct {
	request_type_t type;
	init_t init;
	before_connect_t before_connect;
	after_connect_t after_connect;
	before_close_t before_close;
	after_close_t after_close;
	parse_t parse;
	process_t process;
	periodic_t periodic;
} request_callback_t;

typedef enum {
	ACCOUNT_TYPE_CARD = 0,
	ACCOUNT_TYPE_ACCOUNT,
	ACCOUNT_TYPE_VIN,
} account_type_t;

typedef struct {
	uint8_t account_type;//account_type_t
	uint64_t card_id;
	char *password;

	callback_fn_t fn;
	void *fn_ctx;
} account_request_info_t;

typedef enum {
	ACCOUNT_STATE_CODE_OK = 0,
	ACCOUNT_STATE_CODE_AUTH,
	ACCOUNT_STATE_CODE_AMOUNT,
	ACCOUNT_STATE_CODE_STOP,
	ACCOUNT_STATE_CODE_UNUSED,
	ACCOUNT_STATE_CODE_UNKNOW,
	ACCOUNT_STATE_CODE_BUSY,
	ACCOUNT_STATE_CODE_OFFLINE,
	ACCOUNT_STATE_CODE_TIMEOUT,
} account_state_code_t;

typedef struct {
	uint8_t code;//account_state_code_t
	uint32_t balance;
} account_response_info_t;

typedef enum {
	NET_CLIENT_CTRL_CMD_NONE = 0,
	NET_CLIENT_CTRL_CMD_QUERY_ACCOUNT,
} net_client_ctrl_cmd_t;

typedef struct {
	net_client_ctrl_cmd_t cmd;
	void *args;
} net_client_ctrl_cmd_info_t;

typedef struct {
	int sock_fd;
	uint32_t connect_id;
	uint32_t retry_count;
	uint32_t periodic_stamp;
	uint32_t connect_stamp;
	uint32_t connect_confirm_stamp;
	uint8_t reset_connect;
	client_state_t state;
	net_client_addr_info_t net_client_addr_info;
	net_message_buffer_t recv_message_buffer;
	net_message_buffer_t send_message_buffer;
	request_type_t request_type;
	protocol_if_t *protocol_if;
	request_callback_t *request_callback;

	callback_chain_t *net_client_ctrl_cmd_chain;
	callback_item_t net_client_ctrl_cmd_callback_item;

	void *protocol_ctx;
} net_client_info_t;

void set_net_client_request_type(net_client_info_t *net_client_info, request_type_t request_type);
void set_client_state(net_client_info_t *net_client_info, client_state_t state);
client_state_t get_client_state(net_client_info_t *net_client_info);
void set_lan_led(GPIO_PinState PinState);
uint32_t get_net_client_connect_id(net_client_info_t *net_client_info);
int send_to_server(net_client_info_t *net_client_info, uint8_t *buffer, size_t len);
net_client_info_t *get_net_client_info(void);
void net_client_add_poll_loop(poll_loop_t *poll_loop);
int net_client_net_client_ctrl_cmd(net_client_info_t *net_client_info, net_client_ctrl_cmd_t cmd, void *args);

#endif //_NET_CLIENT_H
