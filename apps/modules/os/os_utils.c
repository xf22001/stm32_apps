

/*================================================================
 *
 *
 *   文件名称：os_utils.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分17秒
 *   修改日期：2020年05月14日 星期四 16时23分57秒
 *   描    述：
 *
 *================================================================*/
#include "os_utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOG_BUFFER_SIZE (1024)

void vPortFree( void *pv );
void *pvPortMalloc( size_t xWantedSize );

void *os_alloc(size_t size)
{
	return pvPortMalloc(size);
}

void os_free(void *p)
{
	vPortFree(p);
}

int log_printf(log_fn_t log_fn, const char *fmt, ...)
{
	va_list ap;
	int ret = -1;
	char *log_buffer = (char *)os_alloc(LOG_BUFFER_SIZE);

	if(log_buffer == NULL) {
		return ret;
	}

	va_start(ap, fmt);
	ret = vsnprintf(log_buffer, LOG_BUFFER_SIZE, fmt, ap);
	va_end(ap);

	if(ret > LOG_BUFFER_SIZE) {
		ret = LOG_BUFFER_SIZE;
	}

	if(log_fn != NULL) {
		ret = log_fn(log_buffer, ret);
	}

	os_free(log_buffer);

	return ret;
}

static int32_t my_isprint(int32_t c)
{
	if(((uint8_t)c >= 0x20) && ((uint8_t)c <= 0x7e)) {
		return 0x4000;
	} else {
		return 0;
	}
}

#define BUFFER_LEN 80
void log_hexdump(log_fn_t log_fn, const char *label, const char *data, int len)
{
	int ret = 0;
	char *buffer = (char *)os_alloc(BUFFER_LEN);
	const char *start = data;
	const char *end = start + len;
	int c;
	int puts(const char *s);
	char *buffer_start = buffer;
	int i;
	long offset = 0;
	int bytes_per_line = 16;

	if(buffer == NULL) {
		return;
	}

	if(label != NULL) {
		ret = snprintf(buffer, BUFFER_LEN, "%s:\n", label);

		if(ret > BUFFER_LEN) {
			ret = BUFFER_LEN;
		}

		if(log_fn != NULL) {
			ret = log_fn(buffer, ret);
		}
	}

	while(start < end) {
		int left = BUFFER_LEN;
		long address = start - data;

		buffer_start = buffer;

		c = end - start;

		if(c > bytes_per_line) {
			c = bytes_per_line;
		}

		ret = snprintf(buffer_start, left, "%08lx", offset + address);
		buffer_start += ret;

		if(ret >= left) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

		ret = snprintf(buffer_start, left, " ");
		buffer_start += ret;

		if(ret >= left) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

		for(i = 0; i < c; i++) {
			if(i % 8 == 0) {
				ret = snprintf(buffer_start, left, " ");
				buffer_start += ret;

				if(ret >= left) {
					left = 0;
					goto out;
				} else {
					left -= ret;
				}
			}

			ret = snprintf(buffer_start, left, "%02x ", (unsigned char)start[i]);
			buffer_start += ret;

			if(ret >= left) {
				left = 0;
				goto out;
			} else {
				left -= ret;
			}
		}

		for(i = c; i < bytes_per_line; i++) {
			if(i % 8 == 0) {
				ret = snprintf(buffer_start, left, " ");
				buffer_start += ret;

				if(ret >= left) {
					left = 0;
					goto out;
				} else {
					left -= ret;
				}
			}

			ret = snprintf(buffer_start, left, "%2s ", " ");
			buffer_start += ret;

			if(ret >= left) {
				left = 0;
				goto out;
			} else {
				left -= ret;
			}
		}

		ret = snprintf(buffer_start, left, "|");
		buffer_start += ret;

		if(ret >= left) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

		for(i = 0; i < c; i++) {
			ret = snprintf(buffer_start, left, "%c", my_isprint(start[i]) ? start[i] : '.');
			buffer_start += ret;

			if(ret >= left) {
				left = 0;
				goto out;
			} else {
				left -= ret;
			}
		}

		ret = snprintf(buffer_start, left, "|");
		buffer_start += ret;

		if(ret >= left) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

		ret = snprintf(buffer_start, left, "\n");
		buffer_start += ret;

		if(ret >= left) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

	out:

		if(log_fn != NULL) {
			ret = log_fn(buffer, BUFFER_LEN - left);
		}

		start += c;
	}

	os_free(buffer);
}

int log_puts(log_fn_t log_fn, const char *s)
{
	int ret = 0;
	ret = strlen(s);

	if(log_fn != NULL) {
		if(ret > (1024 - 1)) {
			log_hexdump(log_fn, NULL, s, ret);
		} else {
			ret = log_fn((void *)s, ret);
		}
	}

	return ret;
}

void app_panic(void)
{
	while(1);
}
