

/*================================================================
 *
 *
 *   文件名称：os_utils.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分17秒
 *   修改日期：2019年11月27日 星期三 10时45分38秒
 *   描    述：
 *
 *================================================================*/
#include "os_utils.h"

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

void app_panic(void)
{
	while(1);
}
