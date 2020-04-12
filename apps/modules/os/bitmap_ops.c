

/*================================================================
 *
 *
 *   文件名称：bitmap_ops.c
 *   创 建 者：肖飞
 *   创建日期：2020年01月19日 星期日 13时14分10秒
 *   修改日期：2020年04月12日 星期日 13时32分52秒
 *   描    述：
 *
 *================================================================*/
#include "bitmap_ops.h"

#include "os_utils.h"
#include "string.h"

bitmap_t *alloc_bitmap(int size)
{
	bitmap_t *bitmap = NULL;
	cell_type_t *data = NULL;
	uint16_t cell_size = (size + BIT_PER_CELL - 1) / BIT_PER_CELL;

	osMutexDef(bitmap_mutex);

	if(size <= 0) {
		return bitmap;
	}

	data = (cell_type_t *)os_alloc(cell_size * BYTES_PER_CELL);

	if(data == NULL) {
		return bitmap;
	}

	memset(data, 0, cell_size * BYTES_PER_CELL);

	bitmap = (bitmap_t *)os_alloc(sizeof(bitmap_t));

	if(bitmap == NULL) {
		os_free(data);
	}

	bitmap->size = size;
	bitmap->data = data;
	bitmap->bitmap_mutex = osMutexCreate(osMutex(bitmap_mutex));

	return bitmap;
}

void free_bitmap(bitmap_t *bitmap)
{
	osStatus os_status;

	if(bitmap == NULL) {
		return;
	}

	if(bitmap->bitmap_mutex) {
		os_status = osMutexWait(bitmap->bitmap_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	os_free(bitmap->data);
	bitmap->data = NULL;

	if(bitmap->bitmap_mutex) {
		os_status = osMutexRelease(bitmap->bitmap_mutex);

		if(os_status != osOK) {
		}
	}

	os_free(bitmap);
}

static int cell_get_first_value_offset(cell_type_t cell, uint8_t value)
{
	int ret = -1;
	int i;
	cell_type_t bit_value;

	for(i = 0; i < BIT_PER_CELL; i++) {
		bit_value = cell >> i;

		if((bit_value & 0x01) == value) {
			ret = i;
			break;
		}
	}

	return ret;
}

int get_first_value_index(bitmap_t *bitmap, uint8_t value)
{
	int ret = -1;
	int loops;
	int i;
	osStatus os_status;

	if(bitmap == NULL) {
		return ret;
	}

	if(bitmap->bitmap_mutex) {
		os_status = osMutexWait(bitmap->bitmap_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	loops = (bitmap->size + BIT_PER_CELL - 1) / BIT_PER_CELL;

	for(i = 0; i < loops; i++) {
		int offset = cell_get_first_value_offset(bitmap->data[i], value);

		if(offset != -1) {
			ret = i * BIT_PER_CELL + offset;
			break;
		}
	}

	if(bitmap->bitmap_mutex) {
		os_status = osMutexRelease(bitmap->bitmap_mutex);

		if(os_status != osOK) {
		}
	}

	return ret;
}

int set_bitmap_value(bitmap_t *bitmap, int index, uint8_t value)
{
	int ret = -1;
	cell_type_t *cell;
	int cell_index;
	int cell_offset;
	osStatus os_status;

	if(bitmap == NULL) {
		return ret;
	}

	if(bitmap->bitmap_mutex) {
		os_status = osMutexWait(bitmap->bitmap_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	if(index >= bitmap->size) {
		return ret;
	}

	cell_index = index / BIT_PER_CELL;
	cell_offset = index % BIT_PER_CELL;

	cell = bitmap->data + cell_index;

	if(value == 1) {
		*cell |= (1 << cell_offset);
	} else {
		*cell &= ~(1 << cell_offset);
	}

	if(bitmap->bitmap_mutex) {
		os_status = osMutexRelease(bitmap->bitmap_mutex);

		if(os_status != osOK) {
		}
	}

	ret = 0;

	return ret;
}
