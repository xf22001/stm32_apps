

/*================================================================
 *
 *
 *   文件名称：log.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月29日 星期五 14时29分33秒
 *   修改日期：2021年01月29日 星期五 16时33分32秒
 *   描    述：
 *
 *================================================================*/
#ifndef _LOG_H
#define _LOG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uint32_t enable_log_uart : 1;
	uint32_t enable_log_udp : 1;
	uint32_t enable_log_file : 1;
} log_mask_t;

typedef union {
	log_mask_t s;
	uint32_t v;
} u_log_mask_t;

typedef int (*log_fn_t)(uint32_t log_mask, const char *buffer, size_t size);

#define LOG_MASK_UART_OFFSET 0
#define LOG_MASK_UDP_OFFSET 1
#define LOG_MASK_FILE_OFFSET 2

#define LOG_MASK_UART 0
#define LOG_MASK_UDP 0
#define LOG_MASK_FILE 0

#if !defined(LOG_NONE)
#if !defined(LOG_UDP) && !defined(LOG_UART) && !defined(LOG_FILE)
#define LOG_UART
#endif
#endif

#if defined(LOG_UART)
#undef LOG_MASK_UART
#define LOG_MASK_UART (1 << LOG_MASK_UART_OFFSET)
#endif

#if defined(LOG_UDP)
#undef LOG_MASK_UDP
#define LOG_MASK_UDP (1 << LOG_MASK_UDP_OFFSET)
#endif

#if defined(LOG_FILE)
#undef LOG_MASK_FILE
#define LOG_MASK_FILE (1 << LOG_MASK_FILE_OFFSET)
#endif

#define LOG_MASK (LOG_MASK_UART | LOG_MASK_UDP | LOG_MASK_FILE)

#define _printf(fmt, ...) log_printf(LOG_MASK, fmt, ## __VA_ARGS__)
#define _hexdump(label, data, len) log_hexdump(LOG_MASK, label, data, len)
#define _puts(s) log_puts(LOG_MASK, s)

#define debug(fmt, ...) _printf("[%s:%s:%d] " fmt, __FILE__, __func__, __LINE__, ## __VA_ARGS__)

int add_log_handler(log_fn_t fn);
int remove_log_handler(log_fn_t fn);
int log_printf(uint32_t log_mask, const char *fmt, ...);
void log_hexdump(uint32_t log_mask, const char *label, const char *data, int len);
int log_puts(uint32_t log_mask, const char *s);
#endif //_LOG_H
