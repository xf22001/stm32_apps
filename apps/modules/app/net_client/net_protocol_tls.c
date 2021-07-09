

/*================================================================
 *
 *
 *   文件名称：net_protocol_tls.c
 *   创 建 者：肖飞
 *   创建日期：2021年07月09日 星期五 08时43分10秒
 *   修改日期：2021年07月09日 星期五 13时03分00秒
 *   描    述：
 *
 *================================================================*/
#include "app_platform.h"
#include "cmsis_os.h"

#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/dhcp.h"
#include "os_utils.h"
#include "net_client.h"
#include "mbedtls/platform.h"
#include "mbedtls/debug.h"
#include "app.h"

#include "log.h"

typedef struct {
	mbedtls_net_context         ssl_fd;
	mbedtls_entropy_context     entropy;
	mbedtls_ctr_drbg_context    ctr_drbg;
	mbedtls_ssl_context         ssl;
	mbedtls_ssl_config          conf;
	mbedtls_x509_crt            cacert;
} tls_ctx_t;

static void *_os_calloc(size_t nmemb, size_t size)
{
	void *ptr = os_calloc(nmemb, size);

	if(ptr == NULL) {
		app_panic();
	}

	return ptr;
}

static void my_debug(void *ctx, int level, const char *file, int line, const char *str)
{
	((void)level);
	//fprintf((FILE *)ctx, "%s:%04d: %s", file, line, str);
	//fflush((FILE *)ctx);
	_printf("%s:%04d: %s", file, line, str);
}

static int mbedtls_net_connect_timeout(net_client_info_t *net_client_info, uint32_t timeout)
{
	int ret;
	socket_addr_info_t *socket_addr_info = net_client_info->net_client_addr_info.socket_addr_info;
	int fd = -1;

	ret = socket_nonblock_connect(socket_addr_info, &fd);

	if(ret != 0) {
		debug("ret:%d", ret);
		return ret;
	}

	ret = poll_wait_nonblock_connect_event(fd, timeout);

	if(ret != 0) {
		debug("ret:%d", ret);
		close(fd);
		debug("close socket %d", fd);
		return ret;
	}

	ret = socket_nonblock_connect_confirm(fd);

	if(ret != 0) {
		debug("ret:%d", ret);
		close(fd);
		debug("close socket %d", fd);
		return ret;
	}

	ret = fd;

	return ret;
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

static int tls_client_close(void *ctx);
static int tls_client_connect(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	tls_ctx_t *tls = (tls_ctx_t *)net_client_info->protocol_ctx;
	char error_message[100];

	if(get_connect_enable() == 0) {
		return ret;
	}

	set_connect_enable(0);

	//test_https("https://httpbin.org/get");
	//return ret;

	OS_ASSERT(tls == NULL);
	tls = os_calloc(1, sizeof(tls_ctx_t));
	OS_ASSERT(tls != NULL);
	net_client_info->protocol_ctx = tls;

	mbedtls_ssl_init(&tls->ssl);
	mbedtls_ssl_config_init(&tls->conf);
	mbedtls_x509_crt_init(&tls->cacert);
	mbedtls_ctr_drbg_init(&tls->ctr_drbg);

	tls->ssl_fd.fd  = -1;

	mbedtls_platform_set_calloc_free(_os_calloc, os_free);

	mbedtls_entropy_init(&tls->entropy);
	ret = mbedtls_ctr_drbg_seed(&tls->ctr_drbg, mbedtls_entropy_func, &tls->entropy, NULL, 0);

	if(ret != 0) {
		mbedtls_strerror(ret, error_message, 100);
		debug("socket error: %s(%d)", error_message, ret);
		tls_client_close(net_client_info);
		return ret;
	}

	//ca_crt_rsa[ca_crt_rsa_size - 1] = 0;
	//ret = mbedtls_x509_crt_parse(&tls->cacert, (uint8_t *)ca_crt_rsa, ca_crt_rsa_size);
	//if(ret != 0)
	//{
	//    return ret;
	//}

	ret = mbedtls_ssl_config_defaults(&tls->conf,
	                                  MBEDTLS_SSL_IS_CLIENT,
	                                  MBEDTLS_SSL_TRANSPORT_STREAM,
	                                  MBEDTLS_SSL_PRESET_DEFAULT);

	if(ret != 0) {
		mbedtls_strerror(ret, error_message, 100);
		debug("socket error: %s(%d)", error_message, ret);
		tls_client_close(net_client_info);
		return ret;
	}

	/* OPTIONAL is not optimal for security,
	 * but makes interop easier in this simplified example */
	mbedtls_ssl_conf_dbg(&tls->conf, my_debug, stdout);
	mbedtls_debug_set_threshold(4);
	mbedtls_ssl_conf_authmode(&tls->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
	mbedtls_ssl_conf_ca_chain(&tls->conf, &tls->cacert, NULL);
	mbedtls_ssl_conf_rng(&tls->conf, mbedtls_ctr_drbg_random, &tls->ctr_drbg);
	mbedtls_ssl_conf_read_timeout(&tls->conf, 5000);

	ret = mbedtls_ssl_setup(&tls->ssl, &tls->conf);

	if(ret != 0) {
		mbedtls_strerror(ret, error_message, 100);
		debug("socket error: %s(%d)", error_message, ret);
		tls_client_close(net_client_info);
		return ret;
	}

	debug("host:%s", net_client_info->net_client_addr_info.host);
	ret = mbedtls_ssl_set_hostname(&tls->ssl, net_client_info->net_client_addr_info.host);

	if(ret != 0) {
		mbedtls_strerror(ret, error_message, 100);
		debug("socket error: %s(%d)", error_message, ret);
		tls_client_close(net_client_info);
		return ret;
	}

	ret = mbedtls_net_connect_timeout(net_client_info, 5000);

	if(ret < 0) {
		mbedtls_strerror(ret, error_message, 100);
		debug("socket error: %s(%d)", error_message, ret);
		tls_client_close(net_client_info);
		return ret;
	}

	tls->ssl_fd.fd = ret;
	net_client_info->sock_fd = ret;

	mbedtls_ssl_set_bio(&tls->ssl, &tls->ssl_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

	while((ret = mbedtls_ssl_handshake(&tls->ssl)) != 0) {
		if(ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_strerror(ret, error_message, 100);
			debug("socket error: %s(%d)", error_message, ret);
			tls_client_close(net_client_info);
			return ret;
		}
	}

	/* In real life, we probably want to bail out when ret != 0 */
	//if(mbedtls_ssl_get_verify_result(&tls->ssl) != 0) {
	//	ret = MBEDTLS_ERR_X509_CERT_VERIFY_FAILED;
	//	mbedtls_strerror(ret, error_message, 100);
	//	debug("socket error: %s(%d)", error_message, ret);
	//	tls_client_close(net_client_info);
	//	return ret;
	//}

	return ret;
}

static int tls_client_recv(void *ctx, void *buf, size_t len)
{
	int ret;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	tls_ctx_t *tls = (tls_ctx_t *)net_client_info->protocol_ctx;

	while(1) {
		ret = mbedtls_ssl_read(&tls->ssl, (unsigned char *)buf, len);

		if(ret == MBEDTLS_ERR_SSL_WANT_READ) {
			continue;
		} else if(ret <= 0) {
			char error_message[100];
			mbedtls_strerror(ret, error_message, 100);
			debug("socket error: %s(%d)", error_message, ret);
			return -1;
		} else {
			return ret;
		}
	}
}

static int tls_client_send(void *ctx, const void *buf, size_t len)
{
	int ret;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	tls_ctx_t *tls = (tls_ctx_t *)net_client_info->protocol_ctx;
	int sent = 0;

	while(1) {
		ret = mbedtls_ssl_write(&tls->ssl, ((unsigned char *)buf) + sent, (size_t)(len - sent));

		if(ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
			continue;
		} else if(ret <= 0) {
			char error_message[100];
			mbedtls_strerror(ret, error_message, 100);
			debug("socket error: %s(%d)", error_message, ret);
			return ret;
		} else {
			sent += ret;

			if(sent >= len) {
				ret = sent;
				break;
			}
		}
	}

	return ret;
}

static int tls_client_close(void *ctx)
{
	int ret = -1;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	tls_ctx_t *tls = (tls_ctx_t *)net_client_info->protocol_ctx;

	OS_ASSERT(tls != NULL);
	net_client_info->sock_fd = -1;
	debug("close socket %d", tls->ssl_fd.fd);
	mbedtls_ssl_close_notify(&tls->ssl);
	mbedtls_net_free(&tls->ssl_fd);
	mbedtls_x509_crt_free(&tls->cacert);
	mbedtls_ssl_free(&tls->ssl);
	mbedtls_ssl_config_free(&tls->conf);
	mbedtls_ctr_drbg_free(&tls->ctr_drbg);
	mbedtls_entropy_free(&tls->entropy);
	os_free(tls);
	net_client_info->protocol_ctx = NULL;
	return ret;
}

protocol_if_t protocol_if_tls = {
	.type = PROTOCOL_TLS,
	.net_connect = tls_client_connect,
	.net_recv = tls_client_recv,
	.net_send = tls_client_send,
	.net_close = tls_client_close,
};
