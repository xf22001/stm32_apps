

/*================================================================
 *
 *
 *   文件名称：channel_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分48秒
 *   修改日期：2020年05月07日 星期四 17时31分17秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_COMMUNICATION_H
#define _CHANNEL_COMMUNICATION_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#include "os_utils.h"
#include "channel_config.h"
#include "callback_chain.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	RETURN_ERROR = 0,         //无
	BRM_TIMEOUT,              //BRM报文超时
	BCP_TIMEOUT,              //BCP超时
	BRO_TIMEOUT,              //BRO超时
	BCS_TIMEOUT,              //BCS超时
	BCL_TIMEOUT,              //BCL超时
	BST_TIMEOUT,              //BST超时
	BSD_TIMEOUT,              //BSD超时
	BRO_ERROR,                //BRO未准备就绪
	BMS_SUCCESS,              //BMS通讯正常

	BMS_UNCONNECT,            //BMS未连接
	RETURN_INC_TIMEOUT,       //绝缘检测超时
	RETURN_INC_ERROR,         //绝缘故障
	RETURN_DISCHARGE_TIMEOUT, //泄放电路超时
	RETURN_DISCHARGE_ERROR,   //泄放失败
	RETURN_COCON_TIMEOUT,     //粘连检测超时
	RETURN_COCON_ERROR,       //粘连检测失败
	BCS_SUCCESS,              //接收BCS成功
	RETURN_INC_BMS_ERROR,     //检测绝缘时前段带电
	BMS_STARTING,             //正在启动

	CRO_TIMEOUT,              //20检测车上电压超时
	PRECHARGE_ERROR,          //预充错误
	BMS_U_UNMATCH,            //车辆电压不匹配
	SHORT_CIRCUIT_ERROR,      //短路故障
	BFC_ERR,                  //CFC数据错误
	BRO_ABNORMAL,             //BRO异常

	RETURN_SUCCESS = 0xff,
} bms_status_t;

typedef struct {
	uint8_t cmd;
} cmd_common_t;

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
	uint8_t charger_power_on : 1;//充电机主板开机状态
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
	uint8_t bms_connect_state;//bms工作状态码
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
	uint8_t cmd;//4
	uint8_t precharge_voltage_l;//预充电电压
	uint8_t precharge_voltage_h;//预充电电压
	uint8_t precharge_action;//0-停止预充, 1-开始预充, 2-单模块预充
	uint8_t unused[4];
} cmd_4_t;//预充请求

typedef struct {
	uint8_t cmd;//104
	uint8_t unused[7];
} cmd_104_t;//预充请求回复

typedef struct {
	uint8_t cmd;//5
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
	uint8_t vin[3];//14-16
	uint8_t unused[4];
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


typedef struct {
	uint8_t cmd;//60
	uint8_t brm_data[7];//0
} cmd_60_t;//brm 0

typedef struct {
	uint8_t cmd;//160
	uint8_t unused[7];
} cmd_160_t;//辅板回复

typedef struct {
	uint8_t cmd;//61
	uint8_t brm_data[7];//1
} cmd_61_t;//brm 1

typedef struct {
	uint8_t cmd;//161
	uint8_t unused[7];
} cmd_161_t;//辅板回复

typedef struct {
	uint8_t cmd;//62
	uint8_t brm_data[7];//2
} cmd_62_t;//brm 2

typedef struct {
	uint8_t cmd;//162
	uint8_t unused[7];
} cmd_162_t;//辅板回复

typedef struct {
	uint8_t cmd;//63
	uint8_t brm_data[7];//3
} cmd_63_t;//brm 3

typedef struct {
	uint8_t cmd;//163
	uint8_t unused[7];
} cmd_163_t;//辅板回复

typedef struct {
	uint8_t cmd;//64
	uint8_t brm_data[7];//4
} cmd_64_t;//brm 4

typedef struct {
	uint8_t cmd;//164
	uint8_t unused[7];
} cmd_164_t;//辅板回复

typedef struct {
	uint8_t cmd;//65
	uint8_t brm_data[7];//5
} cmd_65_t;//brm 5

typedef struct {
	uint8_t cmd;//165
	uint8_t unused[7];
} cmd_165_t;//辅板回复

typedef struct {
	uint8_t cmd;//66
	uint8_t brm_data[7];//6
} cmd_66_t;//brm 6

typedef struct {
	uint8_t cmd;//166
	uint8_t unused[7];
} cmd_166_t;//辅板回复

typedef struct {
	uint8_t cmd;//67
	uint8_t bcp_data[7];//0
} cmd_67_t;//bcp 0

typedef struct {
	uint8_t cmd;//167
	uint8_t unused[7];
} cmd_167_t;//辅板回复

typedef struct {
	uint8_t cmd;//68
	uint8_t bcp_data[7];//1
} cmd_68_t;//bcp 1

typedef struct {
	uint8_t cmd;//168
	uint8_t unused[7];
} cmd_168_t;//辅板回复

typedef struct {
	uint8_t cmd;//69
	uint8_t bcs_data[7];//0
} cmd_69_t;//bcs 1

typedef struct {
	uint8_t cmd;//169
	uint8_t unused[7];
} cmd_169_t;//辅板回复

typedef struct {
	uint8_t cmd;//70
	uint8_t bcs_data[2];//1
	uint8_t bcl_data[5];//0
} cmd_70_t;//bcs_bcl 1 0

typedef struct {
	uint8_t cmd;//170
	uint8_t unused[7];
} cmd_170_t;//辅板回复

typedef struct {
	uint8_t cmd;//71
	uint8_t bsm_data[7];//0
} cmd_71_t;//bsm 0

typedef struct {
	uint8_t cmd;//171
	uint8_t unused[7];
} cmd_171_t;//辅板回复

typedef struct {
	uint8_t cmd;//72
	uint8_t bst_data[4];//0
	uint8_t unused[3];
} cmd_72_t;//bst 0

typedef struct {
	uint8_t cmd;//172
	uint8_t unused[7];
} cmd_172_t;//辅板回复

typedef struct {
	uint8_t cmd;//73
	uint8_t bsd_data[7];//0
} cmd_73_t;//bsd 0

typedef struct {
	uint8_t cmd;//173
	uint8_t unused[7];
} cmd_173_t;//辅板回复

typedef enum {
	CHANNEL_COM_CMD_1_101 = 0,
	CHANNEL_COM_CMD_2_102,
	CHANNEL_COM_CMD_13_113,
	CHANNEL_COM_CMD_3_103,
	CHANNEL_COM_CMD_4_104,
	CHANNEL_COM_CMD_5_105,
	CHANNEL_COM_CMD_6_106,
	CHANNEL_COM_CMD_7_107,
	CHANNEL_COM_CMD_8_108,
	CHANNEL_COM_CMD_9_109,
	CHANNEL_COM_CMD_10_110,
	CHANNEL_COM_CMD_11_111,
	CHANNEL_COM_CMD_20_120,
	CHANNEL_COM_CMD_21_121,
	CHANNEL_COM_CMD_22_122,
	CHANNEL_COM_CMD_25_125,
	CHANNEL_COM_CMD_30_130,
	CHANNEL_COM_CMD_50_150,
	CHANNEL_COM_CMD_51_151,
	CHANNEL_COM_CMD_60_160,
	CHANNEL_COM_CMD_61_161,
	CHANNEL_COM_CMD_62_162,
	CHANNEL_COM_CMD_63_163,
	CHANNEL_COM_CMD_64_164,
	CHANNEL_COM_CMD_65_165,
	CHANNEL_COM_CMD_66_166,
	CHANNEL_COM_CMD_67_167,
	CHANNEL_COM_CMD_68_168,
	CHANNEL_COM_CMD_69_169,
	CHANNEL_COM_CMD_70_170,
	CHANNEL_COM_CMD_71_171,
	CHANNEL_COM_CMD_72_172,
	CHANNEL_COM_CMD_73_173,
	CHANNEL_COM_CMD_TOTAL,
} channel_com_cmd_t;

typedef enum {
	CHANNEL_COM_STATE_IDLE = 0,
	CHANNEL_COM_STATE_REQUEST,
	CHANNEL_COM_STATE_RESPONSE,
	CHANNEL_COM_STATE_ERROR,
} channel_com_state_t;

typedef struct {
	channel_com_state_t state;
	uint32_t stamp;
	uint32_t duration;
} channel_com_cmd_ctx_t;

typedef struct {
	struct list_head list;
	can_info_t *can_info;
	osMutexId handle_mutex;
	channel_info_config_t *channel_info_config;

	channel_com_cmd_ctx_t cmd_ctx[CHANNEL_COM_CMD_TOTAL];

	void *channel_info;
	void *charger_info;
	void *a_f_b_info;

	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;

	bms_status_t bms_status;
	callback_item_t charger_info_report_status_cb;

} channel_com_info_t;

typedef int (*channel_com_request_callback_t)(channel_com_info_t *channel_com_info);
typedef int (*channel_com_response_callback_t)(channel_com_info_t *channel_com_info);

typedef struct {
	channel_com_cmd_t cmd;
	uint8_t request_code;
	channel_com_request_callback_t request_callback;
	uint8_t response_code;
	channel_com_response_callback_t response_callback;
} channel_com_command_item_t;

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

void free_channel_com_info(channel_com_info_t *channel_com_info);
channel_com_info_t *get_or_alloc_channel_com_info(channel_info_config_t *channel_info_config);
void request_precharge(channel_com_info_t *channel_com_info);

#endif //_CHANNEL_COMMUNICATION_H
