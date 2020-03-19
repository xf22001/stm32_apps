

/*================================================================
 *   
 *   
 *   文件名称：os_utils.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分36秒
 *   修改日期：2020年01月19日 星期日 13时46分31秒
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

void *os_alloc(size_t size);
void os_free(void *p);
void app_panic(void);
#endif //_OS_UTILS_H
