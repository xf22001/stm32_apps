

/*================================================================
 *   
 *   
 *   文件名称：ftpd_file_rtt.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月10日 星期四 08时57分21秒
 *   修改日期：2020年12月10日 星期四 08时57分49秒
 *   描    述：
 *
 *================================================================*/
#include "ftpd_file_rtt.h"

rt_tick_t rt_tick_from_millisecond(rt_int32_t ms)
{
	rt_tick_t tick;

	if (ms < 0) {
		tick = (rt_tick_t)RT_WAITING_FOREVER;
	} else {
		tick = RT_TICK_PER_SECOND * (ms / 1000);
		tick += (RT_TICK_PER_SECOND * (ms % 1000) + 999) / 1000;
	}

	/* return the calculated tick */
	return tick;
}
