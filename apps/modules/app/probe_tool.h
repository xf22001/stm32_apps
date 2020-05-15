

/*================================================================
 *   
 *   
 *   文件名称：probe_tool.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 08时03分37秒
 *   修改日期：2020年05月15日 星期五 08时04分16秒
 *   描    述：
 *
 *================================================================*/
#ifndef _PROBE_TOOL_H
#define _PROBE_TOOL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "request.h"
#include "os_utils.h"

#ifdef __cplusplus
}
#endif

#define BROADCAST_PORT 6000
#define PROBE_TOOL_PORT 6001
#define LOG_TOOL_PORT 6002
#define RECV_BUFFER_SIZE 128
#define SEND_BUFFER_SIZE 128

typedef void (*response_t)(request_t *request);

typedef struct {
	uint32_t fn;
	response_t response;
} server_item_t;

typedef struct {
	server_item_t *server_map;
	size_t server_map_size;
} server_map_info_t;

int log_udp_data(void *data, size_t size);
int probe_server_chunk_sendto(uint32_t fn, void *data, size_t size);
void loopback(request_t *request);
void fn_hello(request_t *request);
uint8_t is_log_client_address_valid(void);
void task_probe_tool(void const *argument);

#endif //_PROBE_TOOL_H
