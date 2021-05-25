

/*================================================================
 *
 *
 *   文件名称：os_utils.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 11时13分36秒
 *   修改日期：2021年05月25日 星期二 21时09分14秒
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

#include <string.h>
#include <ctype.h>

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

static inline uint64_t get_u64_from_bcd_b01234567(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7)
{
	uint8_t v0 = get_u8_from_bcd(b0);
	uint8_t v1 = get_u8_from_bcd(b1);
	uint8_t v2 = get_u8_from_bcd(b2);
	uint8_t v3 = get_u8_from_bcd(b3);
	uint8_t v4 = get_u8_from_bcd(b4);
	uint8_t v5 = get_u8_from_bcd(b5);
	uint8_t v6 = get_u8_from_bcd(b6);
	uint8_t v7 = get_u8_from_bcd(b7);

	return v0 + v1 * 100 + v2 * 10000 + v3 * 1000000 + v4 * 100000000 + v5 * 10000000000 + v6 * 1000000000000 + v7 * 100000000000000;
}


static inline void bcd_to_ascii(char *ascii, size_t ascii_size, uint8_t *bcd, size_t bcd_size)
{
	u_uint8_bcd_t u_uint8_bcd;
	int i;
	uint8_t dh;
	uint8_t dl;

	memset(ascii, 0, ascii_size);

	for(i = 0; i < bcd_size; i++) {
		u_uint8_bcd.v = bcd[i];
		dh = u_uint8_bcd.s.h;
		dl = u_uint8_bcd.s.l;

		if(2 * i + 1 >= ascii_size) {
			break;
		}

		ascii[2 * i + 0] = ((dh >= 0) && (dh <= 9)) ? (dh + 0x30) : 0;

		if(2 * i + 2 >= ascii_size) {
			break;
		}

		ascii[2 * i + 1] = ((dl >= 0) && (dl <= 9)) ? (dl + 0x30) : 0;
	}
}

static inline void ascii_to_bcd(char *ascii, size_t ascii_size, uint8_t *bcd, size_t bcd_size)
{
	u_uint8_bcd_t u_uint8_bcd;
	int i;
	uint8_t c;

	memset(bcd, 0, bcd_size);

	for(i = 0; i < ascii_size; i++) {
		if((i / 2) >= bcd_size) {
			break;
		}

		c = ascii[i];

		if(c == 0) {
			break;
		}

		u_uint8_bcd.v = bcd[i / 2];

		if((c % 2) == 0) {
			u_uint8_bcd.s.h = (isdigit(c) == 0) ? 0 : (c - 0x30);
		} else {
			u_uint8_bcd.s.l = (isdigit(c) == 0) ? 0 : (c - 0x30);
		}

		bcd[i / 2] = u_uint8_bcd.v;
	}
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
	uint16_t u16_0;
	uint16_t u16_1;
} uint32_u16_t;

typedef union {
	uint32_u16_t s;
	uint32_t v;
} u_uint32_u16_t;

static inline uint32_t get_u32_from_u16_01(uint16_t u16_0, uint16_t u16_1)
{
	u_uint32_u16_t u_uint32_u16;

	u_uint32_u16.s.u16_0 = u16_0;
	u_uint32_u16.s.u16_1 = u16_1;

	return u_uint32_u16.v;
}

static inline uint32_t get_u16_0_from_u32(uint32_t u32)
{
	u_uint32_u16_t u_uint32_u16;

	u_uint32_u16.v = u32;

	return u_uint32_u16.s.u16_0;
}

static inline uint32_t get_u16_1_from_u32(uint32_t u32)
{
	u_uint32_u16_t u_uint32_u16;

	u_uint32_u16.v = u32;

	return u_uint32_u16.s.u16_1;
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

#define milliseconds_to_ticks(ms) (ms * configTICK_RATE_HZ / 1000)
#define ticks_to_milliseconds(ticks) (ticks * 1000 / configTICK_RATE_HZ)

typedef osMutexId os_mutex_t;
typedef osMessageQId os_signal_t;
typedef osSemaphoreId os_sem_t;

void app_panic(void);
os_mutex_t mutex_create(void);
void mutex_delete(os_mutex_t mutex);
void mutex_lock(os_mutex_t mutex);
void mutex_unlock(os_mutex_t mutex);
os_signal_t signal_create(size_t size);
void signal_delete(os_signal_t signal);
int signal_wait(os_signal_t signal, uint32_t *pvalue, uint32_t timeout);
int signal_send(os_signal_t signal, uint32_t value, uint32_t timeout);
os_sem_t sem_create(int32_t value);
void sem_delete(os_sem_t sem);
int sem_take(os_sem_t sem, uint32_t timeout);
int sem_release(os_sem_t sem);
uint32_t get_os_critical_state(void);
void os_enter_critical(void);
void os_leave_critical(void);
void *port_malloc(size_t size);
void port_free(void *p);
uint32_t get_total_heap_size(void);
int init_mem_info(void);
void get_mem_info(size_t *size, size_t *count, size_t *max_size);
void *__os_alloc(size_t size, const char *file, const char *func, int line);
void os_free(void *p);
void *__os_realloc(void *p, size_t size, const char *file, const char *func, int line);
void *__os_calloc(size_t n, size_t size, const char *file, const char *func, int line);
unsigned char mem_is_set(char *values, size_t size, char value);
unsigned int str_hash(const char *s);
uint8_t sum_crc8(const void *data, size_t size);
uint16_t sum_crc16(const void *data, size_t size);
uint32_t sum_crc32(const void *data, size_t size);
uint32_t ticks_duration(uint32_t a, uint32_t b);

#define os_alloc(size) __os_alloc(size, __FILE__, __func__, __LINE__)
#define os_realloc(p, size) __os_realloc(p, size, __FILE__, __func__, __LINE__)
#define os_calloc(n, size) __os_calloc(n, size, __FILE__, __func__, __LINE__)

#endif //_OS_UTILS_H
