

/*================================================================
 *   
 *   
 *   文件名称：bitmap_ops.h
 *   创 建 者：肖飞
 *   创建日期：2020年01月19日 星期日 13时16分54秒
 *   修改日期：2021年05月28日 星期五 15时56分10秒
 *   描    述：
 *
 *================================================================*/
#ifndef _BITMAP_OPS_H
#define _BITMAP_OPS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "os_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	os_mutex_t mutex;
	int size;
	int cell_size;
	uint8_t *data;
} bitmap_t;

bitmap_t *alloc_bitmap(int size);
void free_bitmap(bitmap_t *bitmap);
int get_first_value_index(bitmap_t *bitmap, uint8_t value);
int set_bitmap_value(bitmap_t *bitmap, int index, uint8_t value);
int get_bitmap_value(bitmap_t *bitmap, int index);

#endif //_BITMAP_OPS_H
