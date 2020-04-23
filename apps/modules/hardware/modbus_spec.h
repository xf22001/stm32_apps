

/*================================================================
 *   
 *   
 *   文件名称：modbus_spec.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月25日 星期一 17时21分29秒
 *   修改日期：2020年04月23日 星期四 09时48分18秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MODBUS_SPEC_H
#define _MODBUS_SPEC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "os_utils.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uint8_t station;
	uint8_t fn;
} modbus_head_t;

typedef struct {
	uint8_t h;
	uint8_t l;
} modbus_addr_t;

typedef struct {
	uint8_t h;
	uint8_t l;
} modbus_number_t;

typedef struct {
	uint8_t h;
	uint8_t l;
} modbus_data_item_t;

typedef struct {
	uint8_t l;
	uint8_t h;
} modbus_crc_t;


static inline void set_modbus_addr(modbus_addr_t *modbus_addr, uint16_t addr)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.v = addr;
	modbus_addr->l = u_uint16_bytes.s.byte0;
	modbus_addr->h = u_uint16_bytes.s.byte1;
}

static inline uint16_t get_modbus_addr(modbus_addr_t *modbus_addr)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.s.byte0 = modbus_addr->l;
	u_uint16_bytes.s.byte1 = modbus_addr->h;

	return u_uint16_bytes.v;
}

static inline void set_modbus_number(modbus_number_t *modbus_number, uint16_t number)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.v = number;
	modbus_number->l = u_uint16_bytes.s.byte0;
	modbus_number->h = u_uint16_bytes.s.byte1;
}

static inline uint16_t get_modbus_number(modbus_number_t *modbus_number)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.s.byte0 = modbus_number->l;
	u_uint16_bytes.s.byte1 = modbus_number->h;

	return u_uint16_bytes.v;
}

static inline void set_modbus_data_item(modbus_data_item_t *modbus_data_item, uint16_t data)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.v = data;
	modbus_data_item->l = u_uint16_bytes.s.byte0;
	modbus_data_item->h = u_uint16_bytes.s.byte1;
}

static inline uint16_t get_modbus_data_item(modbus_data_item_t *modbus_data_item)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.s.byte0 = modbus_data_item->l;
	u_uint16_bytes.s.byte1 = modbus_data_item->h;

	return u_uint16_bytes.v;
}

static inline void set_modbus_crc(modbus_crc_t *modbus_crc, uint16_t crc)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.v = crc;
	modbus_crc->l = u_uint16_bytes.s.byte0;
	modbus_crc->h = u_uint16_bytes.s.byte1;
}

static inline uint16_t get_modbus_crc(modbus_crc_t *modbus_crc)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.s.byte0 = modbus_crc->l;
	u_uint16_bytes.s.byte1 = modbus_crc->h;

	return u_uint16_bytes.v;
}

#define MODBUS_BUFFER_SIZE 192

uint16_t modbus_calc_crc(uint8_t *data, uint16_t size);

#endif //_MODBUS_SPEC_H
