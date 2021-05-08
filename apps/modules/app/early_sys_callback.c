

/*================================================================
 *
 *
 *   文件名称：early_sys_callback.c
 *   创 建 者：肖飞
 *   创建日期：2021年03月25日 星期四 14时32分54秒
 *   修改日期：2021年05月08日 星期六 21时09分14秒
 *   描    述：
 *
 *================================================================*/
#include "early_sys_callback.h"
#include "os_utils.h"
#include "app.h"

__weak void early_sys_callback(void)
{
	init_mem_info();
	app_init();
}
