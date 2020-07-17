

/*================================================================
 *   
 *   
 *   文件名称：modbus_data_value.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月14日 星期四 08时25分59秒
 *   修改日期：2020年07月17日 星期五 10时25分59秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MODBUS_DATA_VALUE_H
#define _MODBUS_DATA_VALUE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <string.h>

#ifdef __cplusplus
}
#endif

typedef enum {
	MODBUS_DATA_GET = 0,
	MODBUS_DATA_SET,
} modbus_data_op_t;

#define modbus_data_value_rw(value, store, op) do { \
	if(op == MODBUS_DATA_GET) { \
		*value = store; \
	} else if(op == MODBUS_DATA_SET) { \
		store = *value; \
	} \
} while(0)

#define modbus_data_value_r(value, store, op) do { \
	if(op == MODBUS_DATA_GET) { \
		*value = store; \
	} \
} while(0)

#define modbus_data_value_w(value, store, op) do { \
	if(op == MODBUS_DATA_SET) { \
		store = *value; \
	} \
} while(0)

#define modbus_data_value_with_offset_rw(value, store, offset, op) do { \
	if(op == MODBUS_DATA_GET) { \
		*value = store + (offset); \
	} else if(op == MODBUS_DATA_SET) { \
		store = *value - (offset); \
	} \
} while(0)

#define modbus_data_value_with_base_rw(value, store, base, op) do { \
	if(op == MODBUS_DATA_GET) { \
		*value = base - store; \
	} else if(op == MODBUS_DATA_SET) { \
		store = base - *value; \
	} \
} while(0)

static inline void modbus_data_value_copy(uint16_t *value, uint16_t *store, uint16_t size, uint16_t offset, modbus_data_op_t op)
{
	uint16_t copy_size = size - sizeof(uint16_t) * offset;
	uint16_t *from = NULL;
	uint16_t *to = NULL;

	if(offset * sizeof(uint16_t) >= size) {
		return;
	}

	if(copy_size > sizeof(uint16_t)) {
		copy_size = sizeof(uint16_t);
	}

	if(op == MODBUS_DATA_GET) {
		from = store + offset;
		to = value;
	} else if(op == MODBUS_DATA_SET) {
		from = value;
		to = store + offset;
	}

	if(from == NULL) {
		return;
	}

	if(to == NULL) {
		return;
	}

	memcpy(to, from, copy_size);
}

#endif //_MODBUS_DATA_VALUE_H
