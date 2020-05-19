

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_huawei.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 17时23分55秒
 *   修改日期：2020年05月19日 星期二 17时35分35秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_huawei.h"
#include "os_utils.h"
#include <string.h>

#define POWER_ID_TX_CAN1_CONTROL_HUAWEI 0x068081FC
#define POWER_ID_RX_CAN1_CONTROL_HUAWEI 0x0680817C

#define POWER_ID_TX_CAN1_SINGLE_QUERY_HUAWEI 0x068082FC
#define POWER_ID_RX_CAN1_SINGLE_QUERY_HUAWEI 0x0680827C

#define POWER_ID_TX_CAN1_BATCH_QUERY_HUAWEI 0x068040FC
#define POWER_ID_RX_CAN1_BATCH_QUERY_HUAWEI_LAST 0x0680407C
#define POWER_ID_RX_CAN1_BATCH_QUERY_HUAWEI_MIDDLE 0x0680407D

typedef struct {
	uint32_t module_0183_status_bit0 : 1; //输出过压锁死(故障) 1:故障 0:正常
	uint32_t module_0183_status_bit1 : 1;//环境过温关机(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit2 : 1;//模块故障开关(运行状态) 1:故障 0:正常
	uint32_t module_0183_status_bit3 : 1;//模块保护开关(运行状态) 1:模块保护 0:正常
	uint32_t module_0183_status_bit4 : 1;//模块风扇(故障) 1:故障 0:正常
	uint32_t module_0183_status_bit5 : 1;//EEPROM(运行状态) 1:故障 0:正常
	uint32_t module_0183_status_bit6 : 1;//输出过流(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit7 : 1;//输出欠压(故障) 1:故障 0:正常
	uint32_t module_0183_status_bit8 : 1;//环温低温关机(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit9 : 1;//模块开关机(运行状态) 1:关机 0:开机
	uint32_t module_0183_status_bit10 : 1; //风扇(设定状态) 1:固定全速状态 0:自动状态
	uint32_t module_0183_status_bit11 : 1; //预留
	uint32_t module_0183_status_bit12 : 1; //模块内部过温(故障) 1:故障 0:正常
	uint32_t module_0183_status_bit13 : 1; //软地址重排(运行状态) 1:软地址重排中 0:正常
	uint32_t module_0183_status_bit14 : 1; //输出模式自动切换使能(设定状态) 1:使能 0:不使能
	uint32_t module_0183_status_bit15 : 1; //CAN通讯质量(运行状态) 1:CAN通讯质量差 0:CAN通讯正常
	uint32_t module_0183_status_bit16 : 1; //模块顺序起机功能使能(设定状态) 1:使能 0:不使能
	uint32_t module_0183_status_bit17 : 1; //模块输入欠压(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit18 : 1; //模块交流不平衡(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit19 : 1; //模块交流缺相(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit20 : 1; //模块严重不均流(运行状态) 1:不均流 0:正常
	uint32_t module_0183_status_bit21 : 1; //模块序列号重复关机(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit22 : 1; //模块输入过压(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit23 : 1; //模块PFC(故障) 1:故障 0:正常
	uint32_t module_0183_status_bit24 : 1; //模块不均流(运行状态) 1:不均流 0:正常
	uint32_t module_0183_status_bit25 : 1; //预留
	uint32_t module_0183_status_bit26 : 1; //模块内部通信异常(故障) 1:故障 0:正常
	uint32_t module_0183_status_bit27 : 1; //模块输出短路限流(运行状态) 1:限流状态 0:正常
	uint32_t module_0183_status_bit28 : 1; //模块限流(运行状态) 1:限流状态 0:怛压状态
	uint32_t module_0183_status_bit29 : 1; //模块输入停电(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit30 : 1; //PFC母线不平衡(保护) 1:保护 0:正常
	uint32_t module_0183_status_bit31 : 1; //PFC母线电压过欠压(保护) 1:保护 0:正常
} module_0183_status_offset_huawei_t;

typedef struct {
	uint32_t module_0183_extra_status_bit0 : 1; //模块未接插到位(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit1 : 1; //模块输出模式(设定状态) 1:500V 0:950V
	uint32_t module_0183_extra_status_bit2 : 1; //模块输出电容过压(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit3 : 1; //模块输出电容电压不平衡(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit4 : 1; //模块与监控通讯失败关机(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit5 : 1; //模块均流屏蔽(设定状态) 1:均流屏蔽 0:正常均流
	uint32_t module_0183_extra_status_bit6 : 1; //模块硬地址充突(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit7 : 1; //模块硬件地址异常(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit8 : 1; //模块放电电路异常(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit9 : 1; //模块高位硬件地址使能(设定状态) 1:使能 0:不使能
	uint32_t module_0183_extra_status_bit10 : 1; //模块短路锁死(故障) 1:故障 0:正常
	uint32_t module_0183_extra_status_bit11 : 1; //模块内部继电器电路异常(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit12 : 1; //放电电路(故障) 1:故障 0:正常
	uint32_t module_0183_extra_status_bit13 : 1; //输出继电器(故障) 1:故障 0:正常
	uint32_t module_0183_extra_status_bit14 : 1; //输出负载震荡(保护) 1:保护 0:正常
	uint32_t module_0183_extra_status_bit15 : 1; //模块测试模式(设定状态) 1:测试模式 0:正常模式
} module_0183_extra_status_offset_huawei_t;

typedef struct {
	uint32_t module_018f_status_bit0 : 1; //模块输入停电(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit1 : 1; //模块输入欠压(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit2 : 1; //模块输入过压(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit3 : 1; //模块交流不平衡(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit4 : 1; //模块交流缺相(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit5 : 1; //环温过温关机(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit6 : 1; //环温低温关机(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit7 : 1; //模块与监控通讯失败关机(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit8 : 1; //模块硬地址冲突(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit9 : 1; //模块硬件地址异常(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit10 : 1; //模块未插按到位(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit11 : 1; //模块序列号重复关机(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit12 : 1; //PFC母线不平衡(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit13 : 1; //PFC母线电压过欠压(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit14 : 1; //模块输出异常(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit15 : 1; //模块内部继电器电路异常(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit16 : 1; //模块输出电容电压不平衡(保护) 1:保护 0:正常
	uint32_t module_018f_status_bit17 : 1; //模块输出电容过压(保护) 1:保护 0:正常
} module_018f_status_offset_huawei_t;

typedef struct {
	uint32_t module_018f_extra_status_bit0 : 1; //输出过压锁死(故障) 1:故障 0:正常
	uint32_t module_018f_extra_status_bit1 : 1; //模块短路锁死(故障) 1:故障 0:正常
	uint32_t module_018f_extra_status_bit2 : 1; //模块内部过温(故障) 1:故障 0:正常
	uint32_t module_018f_extra_status_bit3 : 1; //模块PFC(故障) 1:故障 0:正常
	uint32_t module_018f_extra_status_bit4 : 1; //模块风扇(故障) 1:故障 0:正常
	uint32_t module_018f_extra_status_bit5 : 1; //模块内部通信异常(故障) 1:故障 0:正常
	uint32_t module_018f_extra_status_bit6 : 1; //输出继电器(故障) 1:故障 0:正常
	uint32_t module_018f_extra_status_bit7 : 1; //放电电路断路(故障) 1:故障 0:正常
	uint32_t module_018f_extra_status_bit8 : 1; //放电电路短路(故障) 1:保护 0:正常
	uint32_t module_018f_extra_status_bit9 : 1; //放电电路故障锁死(故障) 1:故障 0:正常
} module_018f_extra_status_offset_huawei_t;

typedef struct {
	uint32_t module_0190_status_bit0 : 1; //模块设定开关机(设定状态) 1:关机 0:开机
	uint32_t module_0190_status_bit1 : 1; //模块测试模式(设定状态) 1:测试模式 0:正常模式
	uint32_t module_0190_status_bit2 : 1; //模块输出模式(设定状态) 1:950V模式 0:500V模式
	uint32_t module_0190_status_bit3 : 1; //模块均流屏蔽(设定状态) 1:均流屏蔽 0:正常均流
	uint32_t module_0190_status_bit4 : 1; //风扇(设定状态) 1:固定全速状态 0:自动状态
	uint32_t module_0190_status_bit5 : 1; //模块高位硬件地址使能(设定状态) 1:使能 0:不使能
	uint32_t module_0190_status_bit6 : 1; //模块顺序启机功能使能(设定状态) 1:使能 0:不使能
	uint32_t module_0190_status_bit7 : 1; //输出模式自动切换使能(设定状态) 1:使能 0:不使能
} module_0190_status_offset_huawei_t;

typedef struct {
	uint16_t module_0191_status_bit0 : 1; //模块开关机(运行状态) 1:关机 0:开机
	uint16_t module_0191_status_bit1 : 1; //模块保护关机(运行状态) 1:模块保护 0:正常
	uint16_t module_0191_status_bit2 : 1; //模块故障关机(运行状态) 1:模块故障 0:正常
	uint16_t module_0191_status_bit3 : 1; //模块限流(运行状态) 1:限流状态 0:恒压状态
	uint16_t module_0191_status_bit4 : 1; //CAN通讯质量(运行状态) 1:CAN通讯质量差 0:CAN通讯质量正常
	uint16_t module_0191_status_bit5 : 1; //EEPROM(运行状态) 1:读写错误 0:正常
	uint16_t module_0191_status_bit6 : 1; //软地址重排(运行状态) 1:软地址重排中 0:正常
	uint16_t module_0191_status_bit7 : 1; //模块不均流(运行状态) 1:不均流 0:正常
	uint16_t module_0191_status_bit8 : 1; //模块严重不均流(运行状态) 1:不均流 0:正常
	uint16_t module_0191_status_bit9 : 1; //输出短路限流(运行状态) 1:限流状态 0:正常
} module_0191_status_offset_huawei_t;

typedef struct {
	uint32_t module_batch_query_bit0 : 1; //模块运行时间查询
	uint32_t module_batch_query_bit1 : 1; //模块交流输入功率查询
	uint32_t module_batch_query_bit2 : 1; //模块交流输入频率查询
	uint32_t module_batch_query_bit3 : 1; //模块交流输入电流查询
	uint32_t module_batch_query_bit4 : 1; //模块直流输出功率查询
	uint32_t module_batch_query_bit5 : 1; //模块直流输出效率查询
	uint32_t module_batch_query_bit6 : 1; //模块输出电压查询
	uint32_t module_batch_query_bit7 : 1; //直流输出当前工作限流点查询
	uint32_t module_batch_query_bit8 : 1; //直流输出实际限功率点查询
	uint32_t module_batch_query_bit9 : 1; //交流输入线电压查询
	uint32_t module_batch_query_bit10 : 1; //交流输入线电压uv查询
	uint32_t module_batch_query_bit11 : 1; //交流输入线电压vw查询
	uint32_t module_batch_query_bit12 : 1; //交流输入线电压wu查询
	uint32_t module_batch_query_bit13 : 1; //模块内部温度查询
	uint32_t module_batch_query_bit14 : 1; //模块入风口环境温度查询
	uint32_t module_batch_query_bit15 : 1; //模块输出电流1查询
	uint32_t module_batch_query_bit16 : 1; //模块输出电流2查询
	uint32_t module_batch_query_bit17 : 1; //模块告警/状态信息查询
	uint32_t module_batch_query_bit18 : 1; //直流输出电压设定值查询
	uint32_t module_batch_query_bit19 : 1; //直流输出电流设定值(比率)查询
	uint32_t module_batch_query_bit20 : 1; //直流输出电流设定值(物理量)查询
	uint32_t module_batch_query_bit21 : 1; //模块告警信息查询
	uint32_t module_batch_query_bit22 : 1; //模块设定信息查询
	uint32_t module_batch_query_bit23 : 1; //模块运行信息查询
} module_batch_query_bit_t;

typedef struct {
	uint16_t module_status_common_bit0 : 1; //1:模块关机 0:模块运行
	uint16_t module_status_common_bit1 : 1; //1:模块故障 0:模块正常
	uint16_t module_status_common_bit2 : 1; //1:模块限流 0:模块恒压
	uint16_t module_status_common_bit3 : 1; //1:风扇故障 0:风扇正常
	uint16_t module_status_common_bit4 : 1; //1:输入过压 0:输入正常
	uint16_t module_status_common_bit5 : 1; //1:输入欠压 0:输入正常
	uint16_t module_status_common_bit6 : 1; //1:输出过压 0:输出正常
	uint16_t module_status_common_bit7 : 1; //1:输出欠压 0:输出正常
	uint16_t module_status_common_bit8 : 1; //1:过流保护 0:正常
	uint16_t module_status_common_bit9 : 1; //1:过温保护 0:正常
	uint16_t module_status_common_bit10 : 1; //1:设置关机 0:设置开机
} module_status_common_t;

typedef struct {
	uint32_t module_batch_query_control_bit0 : 1; //模块运行时间查询 0x010e
	uint32_t module_batch_query_control_bit1 : 1; //模块交流输入功率查询 0x0170
	uint32_t module_batch_query_control_bit2 : 1; //模块交流输入频率查询 0x0171
	uint32_t module_batch_query_control_bit3 : 1; //模块交流输入电流查询 0x0172
	uint32_t module_batch_query_control_bit4 : 1; //模块直流输出功率查询 0x0173
	uint32_t module_batch_query_control_bit5 : 1; //模块效率查询 0x0174
	uint32_t module_batch_query_control_bit6 : 1; //模块输出电压查询 0x0175
	uint32_t module_batch_query_control_bit7 : 1; //直流输出当前工作限流点查询 0x0176
	uint32_t module_batch_query_control_bit8 : 1; //输出实际限功率点查询 0x0177
	uint32_t module_batch_query_control_bit9 : 1; //交流输入线电压uv查询 0x0178
	uint32_t module_batch_query_control_bit10 : 1; //交流输入线电压uv查询 0x0179
	uint32_t module_batch_query_control_bit11 : 1; //交流输入线电压vw查询 0x017a
	uint32_t module_batch_query_control_bit12 : 1; //交流输入线电压wu查询 0x017b
	uint32_t module_batch_query_control_bit13 : 1; //模块内部温度查询 0x017f
	uint32_t module_batch_query_control_bit14 : 1; //模块入风口环境温度查询 0x0180
	uint32_t module_batch_query_control_bit15 : 1; //模块输出电流1查询 0x0181
	uint32_t module_batch_query_control_bit16 : 1; //模块输出电流2查询 0x0182
	uint32_t module_batch_query_control_bit17 : 1; //模块告警/状态信息查询 0x0183
	uint32_t module_batch_query_control_bit18 : 1; //直流输出电压设定值查询 0x0100
	uint32_t module_batch_query_control_bit19 : 1; //直流输出电流设定值(比率)查询 0x0103
	uint32_t module_batch_query_control_bit20 : 1; //直流输出电流设定值(物理量)查询 0x010f
	uint32_t module_batch_query_control_bit21 : 1; //模块告警信息查询 0x018f
	uint32_t module_batch_query_control_bit22 : 1; //模块设定信息查询 0x0190
	uint32_t module_batch_query_control_bit23 : 1; //模块运行信息查询 0x0191
} module_batch_query_control_t;

typedef struct {
	uint32_t id0 : 16;
	uint32_t module_addr : 7;//1-127
	uint32_t id1 : 9;
} s_module_extid_t;

typedef union {
	s_module_extid_t s;
	uint32_t v;
} u_module_extid_t;

typedef struct {
	uint16_t cmd : 12;
	uint16_t unused : 4;
} s_module_cmd_t;

typedef union {
	s_module_cmd_t s;
	uint16_t v;
} u_module_cmd_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[2];
	uint8_t voltage_b3;//电压(V) * 1024 / 额定电流
	uint8_t voltage_b2;
	uint8_t voltage_b1;
	uint8_t voltage_b0;
} cmd_0x100_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[2];
	uint8_t current_b3;//电流(A) * 1024 / 额定电流
	uint8_t current_b2;
	uint8_t current_b1;
	uint8_t current_b0;
} cmd_0x103_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused;
	uint8_t poweroff;
	uint8_t unused1[4];
} cmd_0x132_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused;
	uint8_t output_voltage_auto_adapt;//高低压自动切换
	uint8_t unused1[4];
} cmd_0x14a_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[6];
} cmd_0x191_t;

typedef struct {
	u_module_cmd_t cmd;
	module_0191_status_offset_huawei_t status;
	uint8_t output_current_h;
	uint8_t output_current_l;
	uint8_t output_voltage_h;
	uint8_t output_voltage_l;
} cmd_0x191_response_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[6];
} cmd_0x183_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[2];
	module_0183_status_offset_huawei_t status;
} cmd_0x183_response_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[6];
} cmd_0x190_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[2];
	module_0190_status_offset_huawei_t status;
	uint8_t setting_current_h;
	uint8_t setting_current_l;
	uint8_t setting_voltage_h;
	uint8_t setting_voltage_l;
} cmd_0x190_response_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[6];
} cmd_0x179_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused1;
	uint8_t phase_voltage_status;
	uint8_t setting_voltage_b3;
	uint8_t setting_voltage_b2;
	uint8_t setting_voltage_b1;
	uint8_t setting_voltage_b0;
} cmd_0x179_response_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[6];
} cmd_0x17a_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused1;
	uint8_t phase_voltage_status;
	uint8_t setting_voltage_b3;
	uint8_t setting_voltage_b2;
	uint8_t setting_voltage_b1;
	uint8_t setting_voltage_b0;
} cmd_0x17a_response_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[6];
} cmd_0x17b_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused1;
	uint8_t phase_voltage_status;
	uint8_t setting_voltage_b3;
	uint8_t setting_voltage_b2;
	uint8_t setting_voltage_b1;
	uint8_t setting_voltage_b0;
} cmd_0x17b_response_t;

typedef enum {
	MODULE_CMD_TOTAL_0x100_0x100 = 0,
	MODULE_CMD_TOTAL_0x103_0x103,
	MODULE_CMD_TOTAL_0x132_0x132,
	MODULE_CMD_TOTAL_0x14a_0x14a,
	MODULE_CMD_TOTAL_0x191_0x191,
	MODULE_CMD_TOTAL_0x183_0x183,
	MODULE_CMD_TOTAL_0x190_0x190,
	MODULE_CMD_TOTAL_0x179_0x179,
	MODULE_CMD_TOTAL_0x17a_0x17a,
	MODULE_CMD_TOTAL_0x17b_0x17b,
	MODULE_CMD_TOTAL,
} module_cmd_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_cmd_t cmd;
	uint32_t request_ext_id;
	uint32_t request_code;
	module_request_callback_t request_callback;
	uint32_t response_ext_id;
	uint32_t response_code;
	module_response_callback_t response_callback;
} module_command_item_t;

static int power_modules_init_huawei(power_modules_info_t *power_modules_info)
{
	int ret = -1;
	int i;

	if(MODULE_CMD_TOTAL > POWER_MODULES_CMD_STATE_SIZE) {//命令状态缓冲区不够用
		return ret;
	}

	for(i = 0; i < POWER_MODULES_SIZE; i++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + i;
		memset(power_module_info, 0, sizeof(power_module_info_t));
	}

	power_modules_info->power_modules_valid = 1;

	ret = 0;

	return ret;
}

static void set_out_voltage(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage)//mv
{
	power_modules_info->power_module_info[module_id].setting_voltage = voltage * 1024 / 1000;
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x100_0x100].state = MODULE_CMD_STATE_REQUEST;
}

static int request_0x100(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x100_t *cmd_0x100 = (cmd_0x100_t *)power_modules_info->can_tx_msg.Data;

	cmd_0x100->voltage_b0 = get_u8_b0_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0x100->voltage_b1 = get_u8_b1_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0x100->voltage_b2 = get_u8_b2_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0x100->voltage_b3 = get_u8_b3_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x100_0x100].state = MODULE_CMD_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x100(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x100_0x100].state = MODULE_CMD_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x100_0x100 = {
	.cmd = MODULE_CMD_TOTAL_0x100_0x100,
	.request_ext_id = POWER_ID_TX_CAN1_CONTROL_HUAWEI,
	.request_code = 0x100,
	.request_callback = request_0x100,
	.response_ext_id = POWER_ID_RX_CAN1_CONTROL_HUAWEI,
	.response_code = 0x100,
	.response_callback = response_0x100,
};

static void set_out_current(power_modules_info_t *power_modules_info, int module_id, uint32_t current)//mv
{
	power_modules_info->power_module_info[module_id].setting_current = current * 1024 / (power_modules_info->rate_current * 1000);
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x103_0x103].state = MODULE_CMD_STATE_REQUEST;
}

static int request_0x103(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x103_t *cmd_0x103 = (cmd_0x103_t *)power_modules_info->can_tx_msg.Data;

	cmd_0x103->current_b0 = get_u8_b0_from_u32(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0x103->current_b1 = get_u8_b1_from_u32(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0x103->current_b2 = get_u8_b2_from_u32(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0x103->current_b3 = get_u8_b3_from_u32(power_modules_info->power_module_info[module_id].setting_current);

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x103_0x103].state = MODULE_CMD_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x103(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x103_0x103].state = MODULE_CMD_STATE_IDLE;
	ret = 0;
	return ret;
}

static void set_out_voltage_current_huawei(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint32_t current)
{
	set_out_voltage(power_modules_info, module_id, voltage);
	set_out_current(power_modules_info, module_id, current);
}

static module_command_item_t module_command_item_0x103_0x103 = {
	.cmd = MODULE_CMD_TOTAL_0x103_0x103,
	.request_ext_id = POWER_ID_TX_CAN1_CONTROL_HUAWEI,
	.request_code = 0x103,
	.request_callback = request_0x103,
	.response_ext_id = POWER_ID_RX_CAN1_CONTROL_HUAWEI,
	.response_code = 0x103,
	.response_callback = response_0x103,
};

static int request_0x132(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x132_t *cmd_0x132 = (cmd_0x132_t *)power_modules_info->can_tx_msg.Data;

	cmd_0x132->poweroff = power_modules_info->power_module_info[module_id].poweroff;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x132_0x132].state = MODULE_CMD_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x132(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x132_0x132].state = MODULE_CMD_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x132_0x132 = {
	.cmd = MODULE_CMD_TOTAL_0x132_0x132,
	.request_ext_id = POWER_ID_TX_CAN1_CONTROL_HUAWEI,
	.request_code = 0x132,
	.request_callback = request_0x132,
	.response_ext_id = POWER_ID_RX_CAN1_CONTROL_HUAWEI,
	.response_code = 0x132,
	.response_callback = response_0x132,
};

static void set_poweroff_huawei(power_modules_info_t *power_modules_info, int module_id, uint8_t poweroff)
{
	power_modules_info->power_module_info[module_id].poweroff = poweroff;
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x132_0x132].state = MODULE_CMD_STATE_REQUEST;

	power_modules_info->power_module_info[module_id].automode = 1;
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x14a_0x14a].state = MODULE_CMD_STATE_REQUEST;
}

static int request_0x14a(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x14a_t *cmd_0x14a = (cmd_0x14a_t *)power_modules_info->can_tx_msg.Data;

	cmd_0x14a->output_voltage_auto_adapt = power_modules_info->power_module_info[module_id].automode;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x14a_0x14a].state = MODULE_CMD_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x14a(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x14a_0x14a].state = MODULE_CMD_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x14a_0x14a = {
	.cmd = MODULE_CMD_TOTAL_0x14a_0x14a,
	.request_ext_id = POWER_ID_TX_CAN1_CONTROL_HUAWEI,
	.request_code = 0x14a,
	.request_callback = request_0x14a,
	.response_ext_id = POWER_ID_RX_CAN1_CONTROL_HUAWEI,
	.response_code = 0x14a,
	.response_callback = response_0x14a,
};

static void query_status_huawei(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x191_0x191].state = MODULE_CMD_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x183_0x183].state = MODULE_CMD_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x190_0x190].state = MODULE_CMD_STATE_REQUEST;
}

static int request_0x191(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x191_t *cmd_0x191 = (cmd_0x191_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x191_0x191].state = MODULE_CMD_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x191(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x191_response_t *cmd_0x191_response = (cmd_0x191_response_t *)power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].output_voltage =
	    get_u16_from_u8_lh(cmd_0x191_response->output_voltage_l, cmd_0x191_response->output_voltage_h);

	power_modules_info->power_module_info[module_id].output_current =
	    get_u16_from_u8_lh(cmd_0x191_response->output_current_l, cmd_0x191_response->output_current_h);

	power_modules_info->power_module_info[module_id].power_module_status.poweroff = cmd_0x191_response->status.module_0191_status_bit0;
	power_modules_info->power_module_info[module_id].power_module_status.fault = cmd_0x191_response->status.module_0191_status_bit2;
	power_modules_info->power_module_info[module_id].power_module_status.output_state = cmd_0x191_response->status.module_0191_status_bit3;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_TOTAL_0x191_0x191].state = MODULE_CMD_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x191_0x191 = {
	.cmd = MODULE_CMD_TOTAL_0x191_0x191,
	.request_ext_id = POWER_ID_TX_CAN1_CONTROL_HUAWEI,
	.request_code = 0x191,
	.request_callback = request_0x191,
	.response_ext_id = POWER_ID_RX_CAN1_CONTROL_HUAWEI,
	.response_code = 0x191,
	.response_callback = response_0x191,
};

power_modules_handler_t power_modules_handler_huawei = {
	.power_module_type = POWER_MODULE_TYPE_HUAWEI,
	.set_out_voltage_current = set_out_voltage_current_huawei,
	.set_poweroff = set_poweroff_huawei,
	.query_status = query_status_huawei,
	.query_a_line_input_voltage = NULL,
	.query_b_line_input_voltage =  NULL,
	.query_c_line_input_voltage = NULL,
	.power_modules_init = power_modules_init_huawei,
	.power_modules_request = NULL,
	.power_modules_response = NULL,
};
