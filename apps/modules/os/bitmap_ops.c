

/*================================================================
 *
 *
 *   文件名称：bitmap_ops.c
 *   创 建 者：肖飞
 *   创建日期：2020年01月19日 星期日 13时14分10秒
 *   修改日期：2021年06月03日 星期四 10时04分50秒
 *   描    述：
 *
 *================================================================*/
#include "bitmap_ops.h"

#include <string.h>

bitmap_t *alloc_bitmap(int size)
{
	bitmap_t *bitmap = NULL;

	if(size <= 0) {
		return bitmap;
	}

	bitmap = (bitmap_t *)os_calloc(1, sizeof(bitmap_t));
	OS_ASSERT(bitmap != NULL);

	bitmap->mutex = mutex_create();
	bitmap->size = size;
	bitmap->cell_size = (size + 8 - 1) / 8;

	bitmap->data = (uint8_t *)os_calloc(1, bitmap->cell_size);
	OS_ASSERT(bitmap->data != NULL);

	return bitmap;
}

void free_bitmap(bitmap_t *bitmap)
{
	OS_ASSERT(bitmap != NULL);

	mutex_lock(bitmap->mutex);

	os_free(bitmap->data);

	mutex_unlock(bitmap->mutex);

	os_free(bitmap);
}

int get_first_value_index(bitmap_t *bitmap, uint8_t value)
{
	int ret = -1;
	int i = 0;
	int j = 0;
	uint8_t found = 0;

	if(bitmap == NULL) {
		return ret;
	}

	mutex_lock(bitmap->mutex);

	for(i = 0; i < bitmap->cell_size; i++) {
		for(j = 0; j < 8; j++) {
			if(value == get_u8_bits(bitmap->data[i], j)) {
				found = 1;
				break;
			}
		}

		if(found == 1) {
			break;
		}
	}

	ret = i * 8 + j;

	mutex_unlock(bitmap->mutex);

	return ret;
}

int set_bitmap_value(bitmap_t *bitmap, int index, uint8_t value)
{
	int ret = -1;
	int i = index / 8;
	int j = index % 8;

	if(bitmap == NULL) {
		return ret;
	}

	if(i >= bitmap->cell_size) {
		return ret;
	}

	mutex_lock(bitmap->mutex);

	bitmap->data[i] = set_u8_bits(bitmap->data[i], j, value);

	mutex_unlock(bitmap->mutex);

	ret = 0;

	return ret;
}

int get_bitmap_value(bitmap_t *bitmap, int index)
{
	int ret = -1;
	int i = index / 8;
	int j = index % 8;

	if(bitmap == NULL) {
		return ret;
	}

	if(i >= bitmap->cell_size) {
		return ret;
	}

	mutex_lock(bitmap->mutex);

	ret = get_u8_bits(bitmap->data[i], j);

	mutex_unlock(bitmap->mutex);

	return ret;
}
