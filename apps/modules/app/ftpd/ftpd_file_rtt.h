

/*================================================================
 *   
 *   
 *   文件名称：ftpd_file_rtt.h
 *   创 建 者：肖飞
 *   创建日期：2020年12月10日 星期四 08时57分27秒
 *   修改日期：2020年12月10日 星期四 10时51分47秒
 *   描    述：
 *
 *================================================================*/
#ifndef _FTPD_FILE_RTT_H
#define _FTPD_FILE_RTT_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "os_utils.h"
#include "time.h"

#ifdef __cplusplus
}
#endif

typedef int rt_bool_t;
typedef uint32_t rt_tick_t;
typedef uint32_t rt_uint32_t;
typedef int32_t rt_int32_t;

#define RT_TRUE 1
#define RT_FALSE 0
#define RT_NULL 0

#define RT_WAITING_FOREVER              -1              /**< Block forever until get resource. */
#define RT_TICK_PER_SECOND 1000

#define rt_tick_get osKernelSysTick
#define rt_sprintf sprintf
#define rt_kprintf _printf
#define rt_malloc os_alloc
#define rt_free os_free

rt_tick_t rt_tick_from_millisecond(rt_int32_t ms);

#endif //_FTPD_FILE_RTT_H
