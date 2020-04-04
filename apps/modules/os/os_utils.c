

/*================================================================
 *
 *
 *   文件名称：os_utils.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分17秒
 *   修改日期：2020年03月20日 星期五 11时13分28秒
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
	int size = 0;
	char *log_buffer = (char *)os_alloc(LOG_BUFFER_SIZE);

	if(log_buffer == NULL) {
		return size;
	}

	va_start(ap, fmt);
	size = vsnprintf(log_buffer, LOG_BUFFER_SIZE, fmt, ap);
	va_end(ap);

	if((LOG_BUFFER_SIZE - 1) <= size) {
		size = LOG_BUFFER_SIZE;
	} else {
		size += 1;
	}

	if(log_fn != NULL) {
		size = log_fn(log_buffer, size);
	}

	os_free(log_buffer);

	return (size - 1);
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

		if((BUFFER_LEN - 1) <= ret) {
			ret = BUFFER_LEN;
		} else {
			ret += 1;
		}

		if(log_fn != NULL) {
			ret = log_fn(buffer, ret);
		}
	}

	while(start < end) {
		int left = BUFFER_LEN - 1;//剩余可打印字符数,去掉结束的0
		long address = start - data;

		buffer_start = buffer;

		c = end - start;

		if(c > bytes_per_line) {
			c = bytes_per_line;
		}

		ret = snprintf(buffer_start, left + 1, "%08lx", offset + address);
		buffer_start += ret;

		if(left <= ret) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

		ret = snprintf(buffer_start, left + 1, " ");
		buffer_start += ret;

		if(left <= ret) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

		for(i = 0; i < c; i++) {
			if(i % 8 == 0) {
				ret = snprintf(buffer_start, left + 1, " ");
				buffer_start += ret;

				if(left <= ret) {
					left = 0;
					goto out;
				} else {
					left -= ret;
				}
			}

			ret = snprintf(buffer_start, left + 1, "%02x ", (unsigned char)start[i]);
			buffer_start += ret;

			if(left <= ret) {
				left = 0;
				goto out;
			} else {
				left -= ret;
			}
		}

		for(i = c; i < bytes_per_line; i++) {
			if(i % 8 == 0) {
				ret = snprintf(buffer_start, left + 1, " ");
				buffer_start += ret;

				if(left <= ret) {
					left = 0;
					goto out;
				} else {
					left -= ret;
				}
			}

			ret = snprintf(buffer_start, left + 1, "%2s ", " ");
			buffer_start += ret;

			if(left <= ret) {
				left = 0;
				goto out;
			} else {
				left -= ret;
			}
		}

		ret = snprintf(buffer_start, left + 1, "|");
		buffer_start += ret;

		if(left <= ret) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

		for(i = 0; i < c; i++) {
			ret = snprintf(buffer_start, left + 1, "%c", my_isprint(start[i]) ? start[i] : '.');
			buffer_start += ret;

			if(left <= ret) {
				left = 0;
				goto out;
			} else {
				left -= ret;
			}
		}

		ret = snprintf(buffer_start, left + 1, "|");
		buffer_start += ret;

		if(left <= ret) {
			left = 0;
			goto out;
		} else {
			left -= ret;
		}

		ret = snprintf(buffer_start, left + 1, "\n");
		buffer_start += ret;

		if(left <= ret) {
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
			ret = log_fn((void *)s, ret + 1);
		}
	}

	return (ret - 1);
}

void app_panic(void)
{
	while(1);
}
