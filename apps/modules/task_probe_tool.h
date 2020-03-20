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
#include "os_utils.h"

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

void task_probe_tool(void const *argument);
uint8_t is_log_client_address_valid(void);
int log_udp_data(void *data, size_t size);

#if defined(UDP_LOG)
#define udp_log_printf(fmt, ...) log_printf((log_fn_t)log_udp_data, fmt, ## __VA_ARGS__)
#define udp_log_hexdump(label, data, len) log_hexdump((log_fn_t)log_udp_data, label, data, len)
#define udp_log_puts(s) log_puts((log_fn_t)log_udp_data, s)
#else//#if defined(UDP_LOG)
#define udp_log_printf(fmt, ...)
#define udp_log_hexdump(label, data, len)
#define udp_log_puts(s)
#endif//#if defined(UDP_LOG)

#endif //_TASK_PROBE_TOOL_H
