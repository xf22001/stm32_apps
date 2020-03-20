

/*================================================================
 *   
 *   
 *   文件名称：os_utils.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分36秒
 *   修改日期：2020年03月20日 星期五 11时11分14秒
 *   描    述：
 *
 *================================================================*/
#ifndef _OS_UTILS_H
#define _OS_UTILS_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct
{
  uint8_t byte0;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
} uint32_bytes_t;

typedef union
{
  uint32_bytes_t s;
  uint32_t v;
} u_uint32_bytes_t;

typedef struct
{
  uint8_t byte0;
  uint8_t byte1;
} uint16_bytes_t;

typedef union
{
  uint16_bytes_t s;
  uint16_t v;
} u_uint16_bytes_t;

typedef int (*log_fn_t)(const char *buffer, size_t size);

void *os_alloc(size_t size);
void os_free(void *p);
void app_panic(void);
int log_printf(log_fn_t log_fn, const char *fmt, ...);
void log_hexdump(log_fn_t log_fn, const char *label, const char *data, int len);
int log_puts(log_fn_t log_fn, const char *s);
#endif //_OS_UTILS_H
