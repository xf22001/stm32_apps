

/*================================================================
 *   
 *   
 *   文件名称：ftp_client.h
 *   创 建 者：肖飞
 *   创建日期：2020年09月15日 星期二 09时32分14秒
 *   修改日期：2021年05月12日 星期三 10时39分17秒
 *   描    述：
 *
 *================================================================*/
#ifndef _FTP_CLIENT_H
#define _FTP_CLIENT_H
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

#ifdef __cplusplus
}
#endif

typedef void (*ftp_client_handler_t)(void *ctx);

typedef struct {
	struct list_head socket_addr_info_list;
	socket_addr_info_t *socket_addr_info;
} addr_info_t;

typedef enum {
	FTP_CLIENT_CMD_STATE_IDLE = 0,
	FTP_CLIENT_CMD_STATE_CONNECT,
	FTP_CLIENT_CMD_STATE_CONNECT_CONFIRM,
	FTP_CLIENT_CMD_STATE_CONNECTED,
	FTP_CLIENT_CMD_STATE_DISCONNECT,
	FTP_CLIENT_CMD_STATE_SUSPEND,
} ftp_client_cmd_state_t;

typedef struct {
	int sock_fd;
	addr_info_t addr_info;
	ftp_client_cmd_state_t request_state;
	ftp_client_cmd_state_t state;
	ftp_client_handler_t handler;
	uint8_t action_state;
	char rx_buffer[64];
	size_t rx_size;
	char tx_buffer[256];
	size_t tx_size;
} ftp_client_cmd_t;

typedef enum {
	FTP_CLIENT_DATA_STATE_IDLE = 0,
	FTP_CLIENT_DATA_STATE_CONNECT,
	FTP_CLIENT_DATA_STATE_CONNECT_CONFIRM,
	FTP_CLIENT_DATA_STATE_CONNECTED,
	FTP_CLIENT_DATA_STATE_DISCONNECT,
	FTP_CLIENT_DATA_STATE_SUSPEND,
} ftp_client_data_state_t;

typedef struct {
	int sock_fd;
	addr_info_t addr_info;
	ftp_client_data_state_t request_state;
	ftp_client_data_state_t state;
	ftp_client_handler_t handler;
	uint8_t action_state;
	char rx_buffer[1024];
	size_t rx_size;
	char tx_buffer[256];
	size_t tx_size;
} ftp_client_data_t;

typedef struct {
	os_mutex_t mutex;//保护回调链数据

	char host[256];
	char port[8];
	char path[256];
	char user[32];
	char password[64];
	uint8_t rest_enable;
} ftp_server_path_t;

typedef enum {
	FTP_CLIENT_ACTION_IDLE = 0,
	FTP_CLIENT_ACTION_DOWNLOAD,
} ftp_client_action_t;

typedef void (*ftp_download_callback_t)(void *fn_ctx, void *_ftp_client_info);

typedef struct {
	ftp_server_path_t ftp_server_path;
	ftp_client_action_t action;
	uint32_t stamp;
	ftp_client_cmd_t cmd;
	ftp_client_data_t data;

	uint32_t file_size;
	uint32_t download_size;
	uint32_t download_stamp;
	uint32_t debug_stamp;
	callback_chain_t *ftp_download_event_chain;
	callback_item_t ftp_download_callback_item;
} ftp_client_info_t;

char *get_ftp_client_cmd_state_des(ftp_client_cmd_state_t state);
char *get_ftp_client_data_state_des(ftp_client_data_state_t state);
int request_ftp_client_download(const char *host, const char *port, const char *path, const char *user, const char *password, void *fn_ctx, ftp_download_callback_t callback);
void ftp_client_add_poll_loop(poll_loop_t *poll_loop);

#endif //_FTP_CLIENT_H
