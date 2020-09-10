

/*================================================================
 *
 *
 *   文件名称：request_ws.c
 *   创 建 者：肖飞
 *   创建日期：2020年02月23日 星期日 15时44分51秒
 *   修改日期：2020年09月10日 星期四 08时34分23秒
 *   描    述：
 *
 *================================================================*/
#include <string.h>

#include "net_client.h"
#include "request.h"

#include "log.h"

static void request_init(void *ctx)
{
	debug("\n");
}

static void request_before_create_server_connect(void *ctx)
{
	debug("\n");
}

static void request_after_create_server_connect(void *ctx)
{
	debug("\n");
}

static void request_before_close_server_connect(void *ctx)
{
	debug("\n");
}

static void request_after_close_server_connect(void *ctx)
{
	debug("\n");
}

static void request_parse(void *ctx, char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	*prequest = buffer;
	*request_size = size;
}

static void request_process(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	_hexdump("request_process", (const char *)request, request_size);
}

static void request_periodic(void *ctx, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	//debug("\n");
}

request_callback_t request_callback_ws = {
	.name = "websocket request",
	.init = request_init,
	.before_connect = request_before_create_server_connect,
	.after_connect = request_after_create_server_connect,
	.before_close = request_before_close_server_connect,
	.after_close = request_after_close_server_connect,
	.parse = request_parse,
	.process = request_process,
	.periodic = request_periodic,
};
