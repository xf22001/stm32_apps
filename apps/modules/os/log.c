

/*================================================================
 *
 *
 *   文件名称：log.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月29日 星期五 12时45分56秒
 *   修改日期：2021年02月03日 星期三 08时30分43秒
 *   描    述：
 *
 *================================================================*/
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "os_utils.h"
#include "callback_chain.h"

#define LOG_BUFFER_SIZE (1024)

typedef struct {
	uint32_t log_mask;
	char *buffer;
	size_t size;
} log_ctx_t;

typedef struct {
	callback_chain_t *log_chain;
} log_info_t;

log_info_t *static_log_info = NULL;

static void free_log_info(log_info_t *log_info)
{
	if(log_info == NULL) {
		return;
	}

	if(log_info->log_chain != NULL) {
		free_callback_chain(log_info->log_chain);
	}

	os_free(log_info);
}

static log_info_t *alloc_log_info(void)
{
	log_info_t *log_info = NULL;

	if(__get_IPSR() != 0) {
		return log_info;
	}

	log_info = (log_info_t *)os_alloc(sizeof(log_info_t));

	if(log_info == NULL) {
		return log_info;
	}

	memset(log_info, 0, sizeof(log_info_t));

	log_info->log_chain = alloc_callback_chain();

	if(log_info->log_chain == NULL) {
		goto failed;
	}

	return log_info;
failed:
	free_log_info(log_info);
	log_info = NULL;
	return log_info;
}

static log_info_t *get_log_info(void)
{
	__disable_irq();

	if(static_log_info == NULL) {
		static_log_info = alloc_log_info();
	}

	__enable_irq();

	return static_log_info;
}

static void common_log_fn(void *fn_ctx, void *chain_ctx)
{
	log_fn_t log_fn = (log_fn_t)fn_ctx;
	log_ctx_t *log_ctx = (log_ctx_t *)chain_ctx;
	int ret;

	if(log_fn == NULL) {
		return;
	}

	if(log_ctx == NULL) {
		return;
	}

	ret = log_fn(log_ctx->log_mask, log_ctx->buffer, log_ctx->size);

	if(ret <= 0) {
	}
}

static int callback_item_filter(callback_item_t *callback_item, void *ctx)
{
	int ret = -1;
	log_fn_t fn = (log_fn_t)ctx;

	if(callback_item->fn_ctx == fn) {
		ret = 0;
	}

	return ret;
}

int add_log_handler(log_fn_t fn)
{
	int ret = -1;

	log_info_t *log_info = get_log_info();
	callback_item_t *callback_item;

	if(log_info == NULL) {
		return ret;
	}

	callback_item = get_callback(log_info->log_chain, callback_item_filter, fn);

	if(callback_item != NULL) {
		return ret;
	}

	callback_item = (callback_item_t *)os_alloc(sizeof(callback_item_t));

	if(callback_item == NULL) {
		goto failed;
	}

	memset(callback_item, 0, sizeof(callback_item_t));

	callback_item->fn = common_log_fn;
	callback_item->fn_ctx = fn;

	ret = register_callback(log_info->log_chain, callback_item);

	if(ret != 0) {
		goto failed;
	}

	return ret;

failed:

	if(callback_item != NULL) {
		os_free(callback_item);
	}

	return ret;
}

int remove_log_handler(log_fn_t fn)
{
	int ret = -1;
	log_info_t *log_info = get_log_info();
	callback_item_t *callback_item;

	if(log_info == NULL) {
		return ret;
	}

	callback_item = get_callback(log_info->log_chain, callback_item_filter, fn);

	if(callback_item == NULL) {
		return ret;
	}

	ret = remove_callback(log_info->log_chain, callback_item);

	if(ret != 0) {
		return ret;
	}

	os_free(callback_item);

	return ret;
}

static char isr_log_buffer[LOG_BUFFER_SIZE];

static int isr_log(uint32_t log_mask, const char *buffer, size_t size)
{
	return size;
}

int log_printf(uint32_t log_mask, const char *fmt, ...)
{
	va_list ap;
	int ret = -1;
	char *log_buffer;
	log_ctx_t log_ctx;
	log_fn_t log_fn;
	log_info_t *log_info = get_log_info();

	if(log_info == NULL) {
		return ret;
	}

	if(__get_IPSR() != 0) {
		log_buffer = isr_log_buffer;
		log_fn = isr_log;
	} else {
		log_buffer = (char *)os_alloc(LOG_BUFFER_SIZE);

		if(log_buffer == NULL) {
			return ret;
		}
	}

	va_start(ap, fmt);
	ret = vsnprintf(log_buffer, LOG_BUFFER_SIZE, fmt, ap);
	va_end(ap);

	if(ret > LOG_BUFFER_SIZE) {
		ret = LOG_BUFFER_SIZE;
	}

	if(__get_IPSR() != 0) {
		ret = log_fn(log_mask, log_buffer, ret);
	} else {
		log_ctx.log_mask = log_mask;
		log_ctx.buffer = log_buffer;
		log_ctx.size = ret;
		do_callback_chain(log_info->log_chain, &log_ctx);
	}

	if(__get_IPSR() == 0) {
		os_free(log_buffer);
	}

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
void log_hexdump(uint32_t log_mask, const char *label, const char *data, int len)
{
	int ret = 0;
	char *log_buffer = NULL;
	const char *start = data;
	const char *end = start + len;
	int c;
	int puts(const char *s);
	char *buffer_start = NULL;
	int i;
	long offset = 0;
	int bytes_per_line = 16;
	log_ctx_t log_ctx;
	log_fn_t log_fn;
	log_info_t *log_info = get_log_info();

	if(log_info == NULL) {
		return;
	}

	if(__get_IPSR() != 0) {
		log_buffer = isr_log_buffer;
		log_fn = isr_log;
	} else {
		log_buffer = (char *)os_alloc(LOG_BUFFER_SIZE);

		if(log_buffer == NULL) {
			return;
		}
	}

	buffer_start = log_buffer;

	if(label != NULL) {
		ret = snprintf(log_buffer, BUFFER_LEN, "%s:\n", label);

		if(ret > BUFFER_LEN) {
			ret = BUFFER_LEN;
		}

		if(__get_IPSR() != 0) {
			ret = log_fn(log_mask, log_buffer, ret);
		} else {
			log_ctx.log_mask = log_mask;
			log_ctx.buffer = log_buffer;
			log_ctx.size = ret;
			do_callback_chain(log_info->log_chain, &log_ctx);
		}
	}

	while(start < end) {
		int left = BUFFER_LEN;
		long address = start - data;

		buffer_start = log_buffer;

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

		if(__get_IPSR() != 0) {
			ret = log_fn(log_mask, log_buffer, ret);
		} else {
			log_ctx.log_mask = log_mask;
			log_ctx.buffer = log_buffer;
			log_ctx.size = ret;
			do_callback_chain(log_info->log_chain, &log_ctx);
		}

		start += c;
	}

	if(__get_IPSR() == 0) {
		os_free(log_buffer);
	}
}

int log_puts(uint32_t log_mask, const char *s)
{
	int ret = 0;
	ret = strlen(s);
	log_ctx_t log_ctx;
	log_fn_t log_fn;
	log_info_t *log_info = get_log_info();

	if(log_info == NULL) {
		return ret;
	}

	if(ret > (1024 - 1)) {
		log_hexdump(log_mask, NULL, s, ret);
	} else {
		if(__get_IPSR() != 0) {
			log_fn = isr_log;
			ret = log_fn(log_mask, s, ret);
		} else {
			log_ctx.log_mask = log_mask;
			log_ctx.buffer = (char *)s;
			log_ctx.size = ret;
			do_callback_chain(log_info->log_chain, &log_ctx);
		}
	}

	return ret;
}
