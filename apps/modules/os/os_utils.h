

/*================================================================
 *
 *
 *   文件名称：os_utils.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分36秒
 *   修改日期：2021年01月29日 星期五 23时04分11秒
 *   描    述：
 *
 *================================================================*/
#ifndef _OS_UTILS_H
#define _OS_UTILS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uint8_t byte0;
	uint8_t byte1;
	uint8_t byte2;
	uint8_t byte3;
} uint32_bytes_t;

typedef union {
	uint32_bytes_t s;
	uint32_t v;
} u_uint32_bytes_t;

typedef struct {
	uint8_t byte0;
	uint8_t byte1;
} uint16_bytes_t;

typedef union {
	uint16_bytes_t s;
	uint16_t v;
} u_uint16_bytes_t;

typedef struct {
	uint8_t l : 4;
	uint8_t h : 4;
} uint8_bcd_t;

typedef union {
	uint8_bcd_t s;
	uint8_t v;
} u_uint8_bcd_t;

static inline uint8_t get_u8_from_bcd(uint8_t v)
{
	u_uint8_bcd_t u_uint8_bcd;

	u_uint8_bcd.v = v;

	return u_uint8_bcd.s.h * 10 + u_uint8_bcd.s.l;
}

static inline uint8_t get_bcd_from_u8(uint8_t v)
{
	u_uint8_bcd_t u_uint8_bcd;

	u_uint8_bcd.s.h = v / 10;
	u_uint8_bcd.s.l = v % 10;

	return u_uint8_bcd.v;
}

static inline uint32_t get_u16_from_bcd_b01(uint8_t b0, uint8_t b1)
{
	uint8_t v0 = get_u8_from_bcd(b0);
	uint8_t v1 = get_u8_from_bcd(b1);

	return v0 + v1 * 100;
}

static inline uint32_t get_u32_from_bcd_b0123(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
	uint8_t v0 = get_u8_from_bcd(b0);
	uint8_t v1 = get_u8_from_bcd(b1);
	uint8_t v2 = get_u8_from_bcd(b2);
	uint8_t v3 = get_u8_from_bcd(b3);

	return v0 + v1 * 100 + v2 * 10000 + v3 * 1000000;
}

static inline uint16_t get_u16_from_u8_lh(uint8_t l, uint8_t h)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.v = 0;
	u_uint16_bytes.s.byte0 = l;
	u_uint16_bytes.s.byte1 = h;

	return u_uint16_bytes.v;
}

static inline uint16_t get_u8_l_from_u16(uint16_t v)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.v = v;

	return u_uint16_bytes.s.byte0;
}

static inline uint16_t get_u8_h_from_u16(uint16_t v)
{
	u_uint16_bytes_t u_uint16_bytes;

	u_uint16_bytes.v = v;

	return u_uint16_bytes.s.byte1;
}

static inline uint32_t get_u32_from_u8_b0123(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
	u_uint32_bytes_t u_uint32_bytes;

	u_uint32_bytes.v = 0;
	u_uint32_bytes.s.byte0 = b0;
	u_uint32_bytes.s.byte1 = b1;
	u_uint32_bytes.s.byte2 = b2;
	u_uint32_bytes.s.byte3 = b3;

	return u_uint32_bytes.v;
}

static inline uint8_t get_u8_b0_from_u32(uint32_t v)
{
	u_uint32_bytes_t u_uint32_bytes;

	u_uint32_bytes.v = v;

	return u_uint32_bytes.s.byte0;
}

static inline uint8_t get_u8_b1_from_u32(uint32_t v)
{
	u_uint32_bytes_t u_uint32_bytes;

	u_uint32_bytes.v = v;

	return u_uint32_bytes.s.byte1;
}

static inline uint8_t get_u8_b2_from_u32(uint32_t v)
{
	u_uint32_bytes_t u_uint32_bytes;

	u_uint32_bytes.v = v;

	return u_uint32_bytes.s.byte2;
}

static inline uint8_t get_u8_b3_from_u32(uint32_t v)
{
	u_uint32_bytes_t u_uint32_bytes;

	u_uint32_bytes.v = v;

	return u_uint32_bytes.s.byte3;
}

typedef struct {
	uint8_t bit0 : 1;
	uint8_t bit1 : 1;
	uint8_t bit2 : 1;
	uint8_t bit3 : 1;
	uint8_t bit4 : 1;
	uint8_t bit5 : 1;
	uint8_t bit6 : 1;
	uint8_t bit7 : 1;
} uint8_bits_t;

typedef union {
	uint8_bits_t s;
	uint8_t v;
} u_uint8_bits_t;

#define add_u8_bits_offset_set_case(e, value) \
	case e: { \
		u_uint8_bits.s.bit##e = value; \
	} \
	break

static inline uint8_t set_u8_bits(uint8_t v, uint8_t offset, uint8_t value)
{
	u_uint8_bits_t u_uint8_bits;
	u_uint8_bits.v = v;

	switch(offset) {
			add_u8_bits_offset_set_case(0, value);
			add_u8_bits_offset_set_case(1, value);
			add_u8_bits_offset_set_case(2, value);
			add_u8_bits_offset_set_case(3, value);
			add_u8_bits_offset_set_case(4, value);
			add_u8_bits_offset_set_case(5, value);
			add_u8_bits_offset_set_case(6, value);
			add_u8_bits_offset_set_case(7, value);

		default: {
		}
		break;
	}

	return u_uint8_bits.v;
}

#define add_u8_bits_offset_get_case(e, value) \
	case e: { \
		value = u_uint8_bits.s.bit##e; \
	} \
	break

static inline uint8_t get_u8_bits(uint8_t v, uint8_t offset)
{
	u_uint8_bits_t u_uint8_bits;
	uint8_t value = 0;

	u_uint8_bits.v = v;

	switch(offset) {
			add_u8_bits_offset_get_case(0, value);
			add_u8_bits_offset_get_case(1, value);
			add_u8_bits_offset_get_case(2, value);
			add_u8_bits_offset_get_case(3, value);
			add_u8_bits_offset_get_case(4, value);
			add_u8_bits_offset_get_case(5, value);
			add_u8_bits_offset_get_case(6, value);
			add_u8_bits_offset_get_case(7, value);

		default: {
		}
		break;
	}

	return value;
}

#define add_des_case(e) \
		case e: { \
			des = #e; \
		} \
		break

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#define OS_ASSERT(exp) do {\
	if(!(exp)) { \
		app_panic(); \
	} \
} while(0)

void app_panic(void);
osMutexId mutex_create(void);
void mutex_delete(osMutexId mutex);
void mutex_lock(osMutexId mutex);
void mutex_unlock(osMutexId mutex);
osMessageQId signal_create(void);
void signal_delete(osMessageQId signal);
int signal_wait(osMessageQId signal, uint32_t timeout);
int signal_send(osMessageQId signal);
void get_mem_info(size_t *size, size_t *count, size_t *max_size);
uint32_t get_total_heap_size(void);
void *os_alloc(size_t size);
void os_free(void *p);
unsigned char mem_is_set(char *values, size_t size, char value);
unsigned int str_hash(const char *s);
unsigned char calc_crc8(const void *data, size_t size);
#endif //_OS_UTILS_H
