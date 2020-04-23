

/*================================================================
 *   
 *   
 *   文件名称：i-a-c.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月22日 星期三 13时03分47秒
 *   修改日期：2020年04月22日 星期三 13时43分38秒
 *   描    述：
 *
 *================================================================*/
#ifndef _I-A-C_H
#define _I-A-C_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "os_utils.h"

#ifdef __cplusplus
}
#endif
//industrial ait conditioning


//warn
typedef struct {
	uint16_t internal_fan_sensor : 1;//内风机传感器故障
	uint16_t air_out_sensor : 1;//排气传感器故障
	uint16_t evaporator_sensor : 1;//蒸发器传感器故障
	uint16_t humidity_sensor : 1;//湿度传感器故障
	uint16_t high_humidity_warn : 1;//高湿报警
	uint16_t low_humidity_warn : 1;//低湿报警
	uint16_t high_temperature_warn : 1;//高温报警
	uint16_t low_temperature_warn : 1;//低温报警
	uint16_t high_voltage_warn : 1;//高压报警
	uint16_t low_voltage_warn : 1;//低压报警
	uint16_t door_switch_warn : 1;//门开关报警
	uint16_t reserved_warn : 1;//备用报警
	uint16_t power_supply_warn : 1;//电源异常
	uint16_t phase_sequence_warn : 1;//相序报警
	uint16_t air_out_high_temperature_warn : 1;//排气高温保护
	uint16_t overcurrent_warn : 1;//过流报警
} reg_offset_14_t;

typedef struct {
	uint16_t compressor_runs_for_4_hours_warn : 1;//压机连续4小时运行报警
	uint16_t online_in_turn_warn : 1;//轮值联机故障
	uint16_t parameter_access_error_warn : 1;//参数存取错误故障
} reg_offset_15_t;
#endif //_I-A-C_H
