

/*================================================================
 *
 *
 *   文件名称：channel_command.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月26日 星期二 08时50分38秒
 *   修改日期：2020年06月01日 星期一 17时45分19秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_COMMAND_H
#define _CHANNEL_COMMAND_H
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
	CMD_CHANNEL_HEARTBEAT = 0,
	CMD_MAIN_SETTTINGS,
	CMD_MAIN_OUTPUT_CONFIG,
	CMD_CHANNEL_OUTPUT_REQUEST,
	CMD_MAIN_OUTPUT_STATUS,
	CMD_MAIN_CONTROL,
	CMD_CHANNEL_REQUEST,
	CMD_CHANNEL_BHM,
	CMD_CHANNEL_BRM,
	CMD_CHANNEL_BCP,
	CMD_CHANNEL_BRO,
	CMD_CHANNEL_BCL,
	CMD_CHANNEL_BCS,
	CMD_CHANNEL_BSM,
	CMD_CHANNEL_BST,
	CMD_CHANNEL_BSD,
	CMD_CHANNEL_BEM,
	CHANNEL_COM_CMD_TOTAL,
} cmd_t;

typedef enum {
	CHANNEL_CONTROL_STATUS_WAIT = 0,
	CHANNEL_CONTROL_STATUS_DONE,
} cmd_status_t;//channel

typedef enum {
	MAIN_CONTROL_TYPE_GUN_NONE = 0,
	MAIN_CONTROL_TYPE_GUN_LOCK,
	MAIN_CONTROL_TYPE_GUN_UNLOCK,
} main_control_type_t;//main

typedef enum {
	CHANNEL_REPORT_TYPE_NONE = 0,
	CHANNEL_REPORT_TYPE_START,//通道请求开机
	CHANNEL_REPORT_TYPE_STOP,//通道请求停机
} channel_request_type_t;//channel

#pragma pack(push, 1)

typedef struct {
	uint8_t cmd;
	uint8_t index;
	uint8_t data[6];
} cmd_common_t;

typedef struct {
	uint8_t cmd;
	uint8_t index;
	uint8_t cmd_status;
} cmd_response_t;

typedef struct {

	uint16_t channel_ver;//辅助功能板版本号
	uint16_t a_f_b_ver;//辅助功能板版本号

	uint8_t gun_state : 1;//有无插枪
	uint8_t battery_available : 1;//车上电池继电器吸合状态，通过电池电压判断---a-f-b
	uint8_t adhesion_p : 1;//正继电器粘连---a-f-b
	uint8_t adhesion_n : 1;//负继电器粘连---a-f-b
	uint8_t gun_lock_state : 1;//枪上锁状态
	uint8_t bms_charge_enable : 1;//bms充电允许状态
	uint8_t a_f_b_state : 1;//辅助功能板连接状态

	uint8_t dc_p_temperature;//-20-220 +20偏移
	uint8_t dc_n_temperature;//-20-220 +20偏移
	uint8_t insulation_resistor_value;//0.1M欧每位

	uint8_t charger_state;//bms阶段
	uint8_t charger_info_status;//工作状态码

	uint8_t door : 1;//门禁报警
	uint8_t stop : 1;//急停
} channel_status_data_t;//channel heartbeat

typedef struct {
	uint8_t charger_sn;//充电机编号
	uint8_t gb;//标准

	uint8_t test_mode : 1;//测试模式
	uint8_t precharge_enable : 1;//允许预充
	uint8_t fault : 1;//充电机故障
	uint8_t charger_power_on : 1;//充电机通道开机状态
	uint8_t manual : 1;//手动模式
	uint8_t adhesion_test : 1;//粘连检测
	uint8_t double_gun_one_car : 1;//双枪充一车
	uint8_t cp_ad : 1;//cp-ad采样

	uint8_t auxiliary_power_type;//12-24v选择
	uint16_t charger_max_output_voltage;//最大输出电压
	uint16_t charger_min_output_voltage;//最小输出电压
	uint16_t charger_max_output_current;//最大输出电流
	uint16_t charger_min_output_current;//最小输出电流
	//uint16_t channel_max_output_power;//通道最大输出功率
} main_settings_t;//main

typedef struct {
	//0:关掉所有pwm
	//1:如果charger_output_enable为1,预充后打开相应pwm.
	uint8_t state;//0:disable output, 1:enable output
	uint32_t bitmask;
} main_output_config_t;//main

typedef struct {
	uint8_t charger_output_enable;
	uint16_t charger_require_output_voltage;//模块充电电压
	uint16_t charger_require_output_current;//模块充电电流
	uint8_t use_single_module;
} channel_output_request_t;//channel

typedef struct {
	uint16_t output_voltage;//0.1v
	uint16_t output_current;//0.1a -400
	uint16_t total_charge_time;//1min 0-600
} main_output_status_t;//main

typedef struct {
	uint8_t main_control_type;
} main_control_t;//main

typedef struct {
	uint8_t channel_request_type;
	uint8_t reason;
} channel_request_t;

#pragma pack(pop)

#endif //_CHANNEL_COMMAND_H
