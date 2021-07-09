

/*================================================================
 *
 *
 *   文件名称：test_https.c
 *   创 建 者：肖飞
 *   创建日期：2020年01月27日 星期一 12时22分23秒
 *   修改日期：2021年07月09日 星期五 09时19分27秒
 *   描    述：
 *
 *================================================================*/
#include "test_https.h"
#include "https.h"
#include "mbedtls/platform.h"
#include <string.h>
#include <stdio.h>

#include "os_utils.h"
#include "log.h"

static HTTP_INFO *hi = NULL;
static BOOL verify_cert = FALSE;

static void *_os_calloc(size_t nmemb, size_t size)
{
	void *ptr = os_calloc(nmemb, size);

	if(ptr == NULL) {
		app_panic();
	}

	return ptr;
}

int test_https(char *url)
{
	int ret = 0;
	char *response = NULL;

	if(hi == NULL) {
		hi = os_calloc(1, sizeof(HTTP_INFO));
		OS_ASSERT(hi != NULL);
	}

	http_init(hi, verify_cert);

	mbedtls_platform_set_calloc_free(_os_calloc, os_free);

	ret = http_open(hi, url);

	if(ret != 0) {
		return ret;
	}

	snprintf((char *)hi->request.method, 8, "GET");
	snprintf((char *)hi->request.content_type, H_FIELD_SIZE, "application/json; charset=utf-8");

	ret = http_write_header(hi);

	if(ret != 0) {
		return ret;
	}

	response = os_calloc(1, 2048);
	OS_ASSERT(response != NULL);
	ret = http_read_chunked(hi, response, 1024);
	debug("return code: %d ", ret);
	debug("return body: %s ", response);

	os_free(response);

	http_close(hi);

	return ret;
}
