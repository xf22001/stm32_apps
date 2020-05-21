

/*================================================================
 *   
 *   
 *   文件名称：dlt_645_spec.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月21日 星期四 10时20分10秒
 *   修改日期：2020年05月21日 星期四 14时08分28秒
 *   描    述：
 *
 *================================================================*/
#ifndef _DLT_645_SPEC_H
#define _DLT_645_SPEC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "os_utils.h"

#ifdef __cplusplus
}
#endif

#define DLT_645_BUFFER_SIZE 256

#define DLT_645_FRAME_START_FLAG 0x68
#define DLT_645_FRAME_STOP_FLAG 0x16

typedef struct {
	uint8_t data[4];//0xfe 0xfe 0xfe 0xfe
} dlt_645_wakeup_t;

typedef struct {
	uint8_t data[6];
} dlt_645_addr_t;

typedef enum {
	DLT_645_CONTROL_FN_RESERVED = 0X00,
	DLT_645_CONTROL_FN_BROADCASE_DATE_TIME = 0X08,
	DLT_645_CONTROL_FN_READ_DATA = 0X11,
	DLT_645_CONTROL_FN_READ_DATA_CONTINUE = 0X12,
	DLT_645_CONTROL_FN_READ_COMM_ADDR = 0X13,
	DLT_645_CONTROL_FN_WRITE_DATA = 0X14,
	DLT_645_CONTROL_FN_WRITE_COMM_ADDR = 0X15,
	DLT_645_CONTROL_FN_FROZEN = 0X16,
	DLT_645_CONTROL_FN_CHANGE_BAUDRATE = 0X17,
	DLT_645_CONTROL_FN_CHANGE_PASSWD = 0X18,
	DLT_645_CONTROL_FN_CLEAR_MAX_REQUIRE = 0x19,
	DLT_645_CONTROL_FN_CLEAR_ELECTRIC_METER = 0x1a,
	DLT_645_CONTROL_FN_CLEAR_EVENT = 0x1b,
} dlt_645_control_fn_t;

typedef struct {
	uint8_t fn : 5;//功能码
	uint8_t continue_flag : 1;//有无后续数据
	uint8_t fault : 1;//0-正常 1-异常
	uint8_t frame_type : 1;//0-主站请求 1-从站应答
} dlt_645_control_t;

typedef union {
	dlt_645_control_t s;
	uint8_t v;
} u_dlt_645_control_t;

typedef struct {
	uint8_t addr_start_flag;//0x68
	dlt_645_addr_t addr;
	uint8_t content_start_flag;//0x68
	u_dlt_645_control_t control;
	uint8_t len;
} dlt_645_frame_head_t;

#endif //_DLT_645_SPEC_H
