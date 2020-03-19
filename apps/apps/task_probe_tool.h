#ifndef _TASK_PROBE_TOOL_H
#define _TASK_PROBE_TOOL_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "request.h"

#define BROADCAST_PORT 6000
#define PROBE_TOOL_PORT 6001
#define LOG_TOOL_PORT 6002
#define RECV_BUFFER_SIZE 128
#define SEND_BUFFER_SIZE 128

typedef void (*response_t)(request_t *request);

typedef struct {
	uint32_t fn;
	response_t response;
} serve_map_t;

#if defined(UDP_LOG)
int udp_log_printf(const char *fmt, ...);
void udp_log_hexdump(const char *label, const char *data, int len);
#else//#if defined(UDP_LOG)
#define udp_log_printf(fmt, ...)
#define udp_log_hexdump(label, data, len)
#endif//#if defined(UDP_LOG)

void task_probe_tool(void const *argument);
uint8_t is_log_client_address_valid(void);
#endif //_TASK_PROBE_TOOL_H
