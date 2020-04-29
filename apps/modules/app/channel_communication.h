

/*================================================================
 *
 *
 *   文件名称：channel_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分48秒
 *   修改日期：2020年04月29日 星期三 16时20分40秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_COMMUNICATION_H
#define _CHANNEL_COMMUNICATION_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

typedef struct {
	uint8_t gun_state : 1;//有无插枪
	uint8_t battery_available : 1;//车上电池继电器吸合状态，通过电池电压判断---a-f-b
	uint8_t output_state : 1;//输出继电器有没有吸合
	uint8_t adhesion_p : 1;//正继电器粘连---a-f-b
	uint8_t adhesion_n : 1;//负继电器粘连---a-f-b
	uint8_t gun_lock_state : 1;//枪上锁状态
	uint8_t bms_charger_enable : 1;//bms充电允许状态
	uint8_t a_f_b_state : 1;//辅助功能板连接状态
} cmd_1_b1_t;

typedef struct {
	uint8_t cmd;//1
	cmd_1_b1_t b1;
	uint8_t bms_state;//bms阶段
	uint8_t dc_p_temperature;//-20-220 +20偏移
	uint8_t dc_n_temperature;//-20-220 +20偏移
	uint8_t insulation_resistor_value;//0.1M欧每位
	uint8_t ver_h;//版本号h 程序版本
	uint8_t ver_l;//版本号l 程序版本
} cmd_1_t;//心跳

typedef struct {
	uint8_t test_mode : 1;//测试模式
	uint8_t precharge_enable : 1;//允许预充
	uint8_t fault : 1;//充电机故障
	uint8_t charger_power_on : 1;//充电机开机状态
	uint8_t manual : 1;//手动模式
	uint8_t adhesion_test : 1;//粘连检测
	uint8_t double_gun_one_car : 1;//双枪充一车
	uint8_t cp_ad : 1;//cp-ad采样
} cmd_101_b3_t;

typedef struct {
	uint8_t cmd;//101
	uint8_t charger_sn;//充电机编号
	uint8_t gb;//标准
	cmd_101_b3_t b3;
	uint8_t charger_output_voltage_l;//充电电压
	uint8_t charger_output_voltage_h;//充电电压
	uint8_t charger_output_current_l;//充电电流
	uint8_t charger_output_current_h;//充电电流
} cmd_101_t;//心跳回复

typedef struct {
	uint8_t cmd;//2
	uint8_t unused[7];
} cmd_2_t;//启动命令回复

typedef struct {
	uint8_t cmd;//102
	uint8_t auxiliary_power_type;//12-24v选择
	uint8_t charger_max_output_voltage_l;//最大输出电压
	uint8_t charger_max_output_voltage_h;//最大输出电压
	uint8_t charger_min_output_voltage_l;//最小输出电压
	uint8_t charger_min_output_voltage_h;//最小输出电压
	uint8_t charger_max_output_current_l;//最大输出电流
	uint8_t charger_max_output_current_h;//最大输出电流
} cmd_102_t;//启动命令

typedef struct {
	uint8_t cmd;//13
	uint8_t unused[7];
} cmd_13_t;//启动命令2回复

typedef struct {
	uint8_t cmd;//113
	uint8_t charger_min_output_current_l;//最小输出电流
	uint8_t charger_min_output_current_h;//最小输出电流
	uint8_t unused[5];
} cmd_113_t;//启动命令2

typedef struct {
	uint8_t door : 1;//门禁报警
	uint8_t stop : 1;//急停
} cmd_3_b4_t;

typedef struct {
	uint8_t cmd;//3
	uint8_t a_f_b_ver_h;//辅助功能板版本号h
	uint8_t a_f_b_ver_l;//辅助功能板版本号l
	uint8_t bms_connect_state;//bms通信质量
	cmd_3_b4_t b4;
	uint8_t unused[3];
} cmd_3_t;//心跳2

typedef struct {
	uint8_t cmd;//103
	uint8_t module_output_voltage_l;//模块充电电压
	uint8_t module_output_voltage_h;//模块充电电压
	uint8_t charnnel_max_output_power_l;//通道最大输出功率
	uint8_t charnnel_max_output_power_h;//通道最大输出功率
	uint8_t module_output_current_l;//模块充电电流
	uint8_t module_output_current_h;//模块充电电流
	uint8_t unused;
} cmd_103_t;//心跳2回复

typedef struct {
	uint8_t stop_precharge : 1;//停止预充
	uint8_t start_precharge : 1;//开始预充
	uint8_t single_module_precharge : 1;//单模块预充
} cmd_4_b3_t;

typedef struct {
	uint8_t cmd;//4
	uint8_t precharge_voltage_l;//预充电电压
	uint8_t precharge_voltage_h;//预充电电压
	cmd_4_b3_t b3;
	uint8_t unused[4];
} cmd_4_t;//预充请求

typedef struct {
	uint8_t cmd;//104
	uint8_t unused[7];
} cmd_104_t;//预充请求回复

typedef struct {
	uint8_t cmd;//4
	uint8_t bms_version_h;//bms版本号 brm version_1
	uint8_t bms_version_l;//bms版本号 brm version_0
	uint8_t battery_type;//电池类型
	uint8_t total_battery_rate_capicity_l;//电池容量
	uint8_t total_battery_rate_capicity_h;//电池容量
	uint8_t total_battery_rate_voltage_l;//电池额定总电压
	uint8_t total_battery_rate_voltage_h;//电池额定总电压
} cmd_5_t;//上传bms数据 brm

typedef struct {
	uint8_t cmd;//105
	uint8_t unused[7];
} cmd_105_t;//上传bms数据回复

typedef struct {
	uint8_t cmd;//6
	uint8_t single_battery_max_voltage_l;//单体最高允许电压 max_charge_voltage_single_battery
	uint8_t single_battery_max_voltage_h;//单体最高允许电压 max_charge_voltage_single_battery
	uint8_t max_temperature;//最高允许温度
	uint8_t max_charge_voltage_l;//最高允许充电总电压
	uint8_t max_charge_voltage_h;//最高允许充电总电压
	uint8_t total_voltage_l;//电池总电压
	uint8_t total_voltage_h;//电池总电压
} cmd_6_t;//上传bms数据2 bcp

typedef struct {
	uint8_t cmd;//106
	uint8_t unused[7];
} cmd_106_t;//上传bms数据回复2

typedef struct {
	uint8_t cmd;//7
	uint8_t vin[7];//0-6
} cmd_7_t;//vin

typedef struct {
	uint8_t cmd;//107
	uint8_t unused[7];
} cmd_107_t;//vin回复

typedef struct {
	uint8_t cmd;//8
	uint8_t vin[7];//7-13
} cmd_8_t;//vin2

typedef struct {
	uint8_t cmd;//108
	uint8_t unused[7];
} cmd_108_t;//vin回复2

typedef struct {
	uint8_t cmd;//9
	uint8_t vin[7];//14-16
} cmd_9_t;//vin3

typedef struct {
	uint8_t cmd;//109
	uint8_t unused[7];
} cmd_109_t;//vin回复3

typedef struct {
	uint8_t cmd;//10
	uint8_t require_voltage_l;//需求电压
	uint8_t require_voltage_h;//需求电压
	uint8_t require_current_l;//需求电流
	uint8_t require_current_h;//需求电流
	uint8_t soc;//soc
	uint8_t single_battery_max_voltage_l;//单体最高电压
	uint8_t single_battery_max_voltage_h;//单体最高电压
} cmd_10_t;//定时上报数据1 bcl

typedef struct {
	uint8_t cmd;//110
	uint8_t output_voltage_l;//输出电压
	uint8_t output_voltage_h;//输出电压
	uint8_t output_current_l;//输出电流
	uint8_t output_current_h;//输出电流
	uint8_t total_charge_time_l;//累计充电时间
	uint8_t total_charge_time_h;//累计充电时间
	uint8_t unused;
} cmd_110_t;//定时上报数据1回复 ccs

typedef struct {
	uint8_t cmd;//11
	uint8_t charge_voltage_l;//充电电压测量值
	uint8_t charge_voltage_h;//充电电压测量值
	uint8_t charge_current_l;//充电电流测量值
	uint8_t charge_current_h;//充电电流测量值
	uint8_t remain_min_l;//剩余充电时间
	uint8_t battery_max_temperature;//电池最高温度
	uint8_t remain_min_h;//剩余充电时间
} cmd_11_t;//定时上报数据2 bcs bsm

typedef struct {
	uint8_t cmd;//111
	uint8_t charger_output_energy_l;//输出总度数
	uint8_t charger_output_energy_h;//输出总度数
	uint8_t unused[5];
} cmd_111_t;//定时上报数据2 bcs bsm

typedef struct {
	uint8_t cmd;//20
	uint8_t unused[7];
} cmd_20_t;//辅板回复

typedef struct {
	uint8_t cmd;//120
	uint8_t unused[7];
} cmd_120_t;//打开辅板输出继电器

typedef struct {
	uint8_t cmd;//21
	uint8_t unused[7];
} cmd_21_t;//辅板回复

typedef struct {
	uint8_t cmd;//121
	uint8_t unused[7];
} cmd_121_t;//锁定辅板电子锁

typedef struct {
	uint8_t cmd;//22
	uint8_t unused[7];
} cmd_22_t;//辅板回复

typedef struct {
	uint8_t cmd;//122
	uint8_t unused[7];
} cmd_122_t;//解除辅板电子锁

typedef struct {
	uint8_t cmd;//25
	uint8_t unused[7];
} cmd_25_t;//通道主动开机命令

typedef struct {
	uint8_t cmd;//125
	uint8_t unused[7];
} cmd_125_t;//回复

typedef struct {
	uint8_t cmd;//30
	uint8_t unused[7];
} cmd_30_t;//通道主动停机命令

typedef struct {
	uint8_t cmd;//130
	uint8_t unused[7];
} cmd_130_t;//回复

typedef struct {
	uint8_t cmd;//50
	uint8_t unused[7];
} cmd_50_t;//辅板回复

typedef struct {
	uint8_t cmd;//150
	uint8_t reason;//停机原因
	uint8_t unused[6];
} cmd_150_t;//发送停机命令

typedef struct {
	uint8_t cmd;//51
	uint8_t unused[7];
} cmd_51_t;//辅板回复

typedef struct {
	uint8_t cmd;//151
	uint8_t unused[7];
} cmd_151_t;//关闭辅板输出继电器
#endif //_CHANNEL_COMMUNICATION_H
