

/*================================================================
 *   
 *   
 *   文件名称：modbus_data_value.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月14日 星期四 08时25分59秒
 *   修改日期：2021年05月14日 星期五 15时31分37秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MODBUS_DATA_VALUE_H
#define _MODBUS_DATA_VALUE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

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

#define modbus_data_buffer_rw(value, store, size, offset, op) do { \
	uint16_t *buffer = (store) + (offset); \
	uint16_t store_offset = (offset) * sizeof(uint16_t); \
	if(size > store_offset) { \
		if(size - store_offset >= sizeof(uint16_t)) { \
			uint16_t *store_buffer = buffer; \
			if(op == MODBUS_DATA_GET) { \
				*value = *store_buffer; \
			} else if(op == MODBUS_DATA_SET) { \
				*store_buffer = *value; \
			} \
		} else { \
			uint8_t *store_buffer = (uint8_t *)buffer; \
			if(op == MODBUS_DATA_GET) { \
				*value = *store_buffer; \
			} else if(op == MODBUS_DATA_SET) { \
				*store_buffer = *value; \
			} \
		} \
	} \
} while(0)

#endif //_MODBUS_DATA_VALUE_H
