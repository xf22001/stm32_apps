

/*================================================================
 *
 *
 *   文件名称：net_protocol_ws.c
 *   创 建 者：肖飞
 *   创建日期：2020年02月23日 星期日 12时23分31秒
 *   修改日期：2021年05月25日 星期二 19时57分36秒
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
#include "https.h"
#include "mbedtls/platform.h"

#include "log.h"

//static HTTP_INFO hi_instance;
static BOOL verify_cert = FALSE;
//static HTTP_INFO *hi = &hi_instance;
static HTTP_INFO *hi = NULL;

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

#include "test_https.h"

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

	if(hi == NULL) {
		hi = (HTTP_INFO *)_os_calloc(1, sizeof(HTTP_INFO));

		if(hi == NULL) {
			debug("");
			return ret;
		}
	}

	http_init(hi, verify_cert);

	mbedtls_platform_set_calloc_free(_os_calloc, os_free);

	ret = http_open(hi, url);

	if(ret != 0) {
		http_close(hi);
		net_client_info->sock_fd = -1;
		set_connect_enable(0);
		debug("");
		return ret;
	}

	snprintf((char *)hi->request.method, 8, "GET");
	snprintf((char *)hi->request.content_type, H_FIELD_SIZE, "application/json; charset=utf-8");

	ret = http_write_header(hi);
	//ret = http_write_ws_header(hi);

	if(ret != 0) {
		http_close(hi);
		net_client_info->sock_fd = -1;
		set_connect_enable(0);
		debug("");
		return ret;
	}

	net_client_info->sock_fd = hi->tls.ssl_fd.fd;

	if(net_client_info->sock_fd == -1) {
		http_close(hi);
		net_client_info->sock_fd = -1;
		set_connect_enable(0);
		debug("");
		return ret;
	}

	ret = 0;
	set_connect_enable(0);

	return ret;
}

static int ws_client_recv(void *ctx, void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	net_client_info = net_client_info;
	return https_read(hi, (char *)buf, len);
}

static int ws_client_send(void *ctx, const void *buf, size_t len)
{
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	net_client_info = net_client_info;
	return https_write(hi, (char *)buf, len);
}

static int ws_client_close(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(hi != NULL) {
		ret = http_close(hi);
		net_client_info->sock_fd = -1;
		os_free(hi);
		hi = NULL;
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
