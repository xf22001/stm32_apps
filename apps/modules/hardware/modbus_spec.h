

/*================================================================
 *   
 *   
 *   文件名称：modbus_spec.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月25日 星期一 17时21分29秒
 *   修改日期：2019年11月27日 星期三 10时21分48秒
 *   描    述：
 *
 *================================================================*/
#ifndef _MODBUS_SPEC_H
#define _MODBUS_SPEC_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

typedef struct {
	uint8_t station;
	uint8_t fn;
} modbus_head_t;

typedef struct {
	uint8_t addr_h;
	uint8_t addr_l;
} modbus_addr_t;

typedef struct {
	uint8_t number_h;
	uint8_t number_l;
} modbus_number_t;

typedef struct {
	uint8_t data_h;
	uint8_t data_l;
} modbus_data_item_t;

typedef struct {
	uint8_t crc_l;
	uint8_t crc_h;
} modbus_crc_t;

#define MODBUS_BUFFER_SIZE 192

#endif //_MODBUS_SPEC_H
