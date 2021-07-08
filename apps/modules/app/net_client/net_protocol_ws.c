

/*================================================================
 *
 *
 *   文件名称：net_protocol_ws.c
 *   创 建 者：肖飞
 *   创建日期：2020年02月23日 星期日 12时23分31秒
 *   修改日期：2021年07月08日 星期四 15时09分33秒
 *   描    述：
 *
 *================================================================*/
#include "app_platform.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/dhcp.h"
#include "main.h"

#include <string.h>
#include "os_utils.h"
#include "net_client.h"
#include "mbedtls/platform.h"

#include "log.h"

static void *_os_calloc(size_t nmemb, size_t size)
{
	void *ptr = os_calloc(nmemb, size);

	if(ptr == NULL) {
		app_panic();
	}

	return ptr;
}

static uint8_t enable_connect = 0;

void set_connect_enable(uint8_t enable)
{
	enable_connect = enable;
}

uint8_t get_connect_enable(void)
{
	return enable_connect;
}

//#include "test_https.h"

static int ws_client_connect(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	char *url = "https://httpbin.org/get";
	//char *url = "ws://192.168.41.2:8080/ocpp/";
	//char *url = "ws://47.244.218.210:8080/OCPP/echoSocket/13623";
	//char *url = "wss://35.201.125.176:433/SSECHINAEVSE";
	//char *url = "https://216.58.199.110";
	//char *url = "wss://ocpp-16-json.dev-plugitcloud.com/SSECHINAEVSE";
	//char *url = "wss://iot-ebus-ocpp-v16-server-test.azurewebsites.net/ws/test123";

	if(get_connect_enable() != 1) {
		debug("");
		return ret;
	}

	//test_https();

	if(net_client_info->hi == NULL) {
		net_client_info->hi = (HTTP_INFO *)_os_calloc(1, sizeof(HTTP_INFO));
		OS_ASSERT(net_client_info->hi != NULL);
	}

	http_init(net_client_info->hi, FALSE);

	mbedtls_platform_set_calloc_free(_os_calloc, os_free);

	ret = http_open(net_client_info->hi, url);

	if(ret != 0) {
		debug("");
		http_close(net_client_info->hi);
		net_client_info->sock_fd = -1;
		return ret;
	}

	net_client_info->sock_fd = net_client_info->hi->tls.ssl_fd.fd;
	OS_ASSERT(net_client_info->sock_fd != -1);

	ret = 0;
	set_connect_enable(0);

	return ret;
}

static int ws_client_recv(void *ctx, void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	net_client_info = net_client_info;
	return https_read(net_client_info->hi, (char *)buf, len);
}

static int ws_client_send(void *ctx, const void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	net_client_info = net_client_info;
	return https_write(net_client_info->hi, (char *)buf, len);
}

static int ws_client_close(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(net_client_info->hi != NULL) {
		ret = http_close(net_client_info->hi);
		net_client_info->sock_fd = -1;
		os_free(net_client_info->hi);
		net_client_info->hi = NULL;
	}

	return ret;
}

protocol_if_t protocol_if_ws = {
	.type = PROTOCOL_WS,
	.net_connect = ws_client_connect,
	.net_recv = ws_client_recv,
	.net_send = ws_client_send,
	.net_close = ws_client_close,
};
