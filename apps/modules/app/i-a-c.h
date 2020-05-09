

/*================================================================
 *   
 *   
 *   文件名称：i-a-c.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月22日 星期三 13时03分47秒
 *   修改日期：2020年05月09日 星期六 13时33分49秒
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
//industrial air conditioning

typedef enum {
	i_a_c_fn_on_off = 256,//开关机设定 读/写 1 0~1 0 关闭,1 开启
	i_a_c_fn_teamperature_refrigeration_start_threshold = 257,//制冷开启温度 读/写 32 -30~45 单位:°C,实际值*10
	i_a_c_fn_teamperature_refrigeration_tolerance = 258,//制冷回差 读/写 3 1~8 单位:°C,实际值*10
	i_a_c_fn_teamprature_heating_start_threshold = 259,//制热开启温度 读/写 5 -30~35 单位:°C,实际值*10
	i_a_c_fn_teamprature_heating_tolerance = 260,//制热回差 读/写 4 1~30 单位:°C,实际值*10
	i_a_c_fn_dehumidification_threshold = 261,//除湿开启湿度 读/写 85 50~90 单位: %RH,实际值*10
	i_a_c_fn_dehumidification_tolerance = 262,//除湿回差 读/写 10 2~15 单位: %RH,实际值*10
	i_a_c_fn_over_teamprature_warning_type = 267,//应急风机/温度报警 读/写 1 0~1 0 应急风机,1 温度报警
	i_a_c_fn_emergency_fan_start_threshold = 268,//应急风机开启温度 读/写 60 30~60 单位:°C,实际值*10
	i_a_c_fn_humidity_warning_type = 269,//排氢风机/湿度报警 读/写 1 0~1 0 排氢风机,1 湿度报警
	i_a_c_fn_teamprature_warning_upper_limit = 273,//温度报警上限 读/写 60 0~90 单位:°C,实际值*10
	i_a_c_fn_teamprature_warning_floor_limit = 274,//温度报警下限 读/写 -20 -40~40 单位:°C,实际值*10
	i_a_c_fn_teamprature_sensor_and_dehumidification_setting = 276,//湿度传感器使能及除湿使能 读/写 3 0~3 0:不开启,1:开启传感器 2:开启除湿1,3:开启除湿2
	i_a_c_fn_humidity_warning_upper_limit = 278,//湿度报警上限 读/写 99 40~99 单位: %RH,实际值*10
	i_a_c_fn_humidity_warning_floor_limit = 279,//湿度报警下限 读/写 0 0~60 单位: %RH,实际值*10
} i_a_c_fn_t;

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
