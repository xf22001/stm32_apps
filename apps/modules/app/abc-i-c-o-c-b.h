

/*================================================================
 *
 *
 *   文件名称：abc-i-c-o-c-b.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月22日 星期三 13时40分15秒
 *   修改日期：2020年05月11日 星期一 08时38分02秒
 *   描    述：
 *
 *================================================================*/
#ifndef _ACB-I-C-O-C-B_H
#define _ACB-I-C-O-C-B_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	acb_i_c_o_c_b_fn_a_phase_line_voltage = 256,//1v
	acb_i_c_o_c_b_fn_b_phase_line_voltage = 257,//1v
	acb_i_c_o_c_b_fn_c_phase_line_voltage = 258,//1v
	acb_i_c_o_c_b_fn_phase_average_voltage = 259,//1v
	acb_i_c_o_c_b_fn_ab_line_voltage = 260,//1v
	acb_i_c_o_c_b_fn_bc_line_voltage = 261,//1v
	acb_i_c_o_c_b_fn_ca_line_voltage = 262,//1v
	acb_i_c_o_c_b_fn_line_average_voltage = 263,//1v
	acb_i_c_o_c_b_fn_a_phase_current = 268,//1a
	acb_i_c_o_c_b_fn_b_phase_current = 269,//1a
	acb_i_c_o_c_b_fn_c_phase_current = 270,//1a
	acb_i_c_o_c_b_fn_a_phase_current_unevenness_rate = 275,//1%
	acb_i_c_o_c_b_fn_b_phase_current_unevenness_rate = 276,//1%
	acb_i_c_o_c_b_fn_c_phase_current_unevenness_rate = 277,//1%
	acb_i_c_o_c_b_fn_max_current_unevenness_rate = 278,//1%
} acb_i_c_o_c_b_fn_t;

typedef struct {
	uint16_t status : 2;//O-1 O,1 1= OPenning 2= Close 3= Closing 断路器状态
	uint16_t warning_flag : 1;//2 0,1 0=无 1=有 报警标志
	uint16_t fault_break_flag : 1;//3 0,1 0=无 1=有 故障跳闸标志
	uint16_t di1_status : 1;//4 0,1 0=复位 1=动作 DI1 状态
	uint16_t di2_status : 1;//5 0,1 0=复位 1=动作 DI2 状态
	uint16_t do1_status : 1;//6 0,1 0=释放 1=吸合 DO1 状态
	uint16_t do2_status : 1;//7 0,1 0=释放 1=吸合 DO2 状态
	uint16_t do3_status : 1;//8 0,2 0=释放 1=吸合 DO3 状态
	uint16_t do4_status : 1;//9 0,2 0=释放 1=吸合 DO4 状态
	uint16_t new_warning : 1;//10 0,1 0=无 1=有 新报警
	uint16_t new_fault_break : 1;//11 0,1 0=无 1=有 新故障跳闸
	uint16_t new_bit_change_event : 1;//12 0,1 0=无 1=有 新变位事件
	uint16_t self_diagnose_info : 1;//13-15 0-4 0=无 1=EEPROM 出错 2=AD 采样出错 3=断路器拒动 4=ROM 出错 控制器自诊断信息
} acb_i_c_o_c_b_reg_offset_512_t;

typedef struct {
	uint16_t load_control_warning1 : 1;//负载控制 1 报警
	uint16_t load_control_warning2 : 1;//负载控制 2 报警
	uint16_t overload_warning2 : 1;//过载报警
	uint16_t electric_leakage_warning : 1;//接地/漏电报警
	uint16_t unstable_current_warning : 1;//电流不平报警
	uint16_t a_phase_usage_warning : 1;//A 相最大需用值报警
	uint16_t b_phase_usage_warning : 1;//B 相最大需用值报警
	uint16_t c_phase_usage_warning : 1;//C 相最大需用值报警
	uint16_t n_phase_usage_warning : 1;//N 相最大需用值报警
	uint16_t voltage_unevenness_warning : 1;//电压不平衡报警
	uint16_t low_voltage_warning : 1;//欠压报警
	uint16_t over_voltage_warning : 1;//过压报警
	uint16_t reverse_power_warning : 1;//逆功率报警
	uint16_t low_frequency_warning : 1;//欠频报警
	uint16_t over_frequency_warning : 1;//过频报警
	uint16_t phase_warning : 1;//相序报警
} acb_i_c_o_c_b_reg_offset_513_t;

typedef struct {
	uint16_t input_1_warning : 1;//输入 1 报警
	uint16_t input_2_warning : 1;//输入 2 报警
	uint16_t overload_warning : 1;//过载预报警
	uint16_t connector_worn_out_warning : 1;//触头磨损报警
	uint16_t self_diagnose_warning : 1;//自诊断报警
} acb_i_c_o_c_b_reg_offset_514_t;

typedef struct {
	uint16_t fault_phase_type : 4;//0-3 定值 0=A 相 1=B 相 2=C 相 3=N 相 4=联锁 A 相 5=联锁 B 相 6=联锁 C 相 7=联锁 N 相 C=试验 其他,无意义 相别
	uint16_t fault_type : 4;//4-7 0-8 0 无故障 无报警 1 过载故障 过载报警 2 短路延时定时限故障 -- 3 短路延时反时限故障 -- 4 短路瞬时故障 -- 5 MCR 故障 -- 6 接地故障 接地报警/漏电报警 7 需用值故障 需用值故障 8 负载监控故障 负载监控报警 类型
} acb_i_c_o_c_b_reg_offset_515_t;

#ifdef __cplusplus
}
#endif
//intelligent controller of circuit breaker
#endif //_ACB-I-C-O-C-B_H
