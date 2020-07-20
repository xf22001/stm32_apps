

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_huawei.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 17时23分55秒
 *   修改日期：2020年07月20日 星期一 10时39分36秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_huawei.h"
#include "os_utils.h"
#include <string.h>

#define LOG_NONE
#include "log.h"

#define POWER_ID_TX_CONTROL_HUAWEI 0x068081FC
#define POWER_ID_RX_CONTROL_HUAWEI 0x0680817C

#define POWER_ID_TX_SINGLE_QUERY_HUAWEI 0x068082FC
#define POWER_ID_RX_SINGLE_QUERY_HUAWEI 0x0680827C

#define POWER_ID_TX_BATCH_QUERY_HUAWEI 0x068040FC
#define POWER_ID_RX_BATCH_QUERY_HUAWEI_LAST 0x0680407C
#define POWER_ID_RX_BATCH_QUERY_HUAWEI_MIDDLE 0x0680407D

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
	uint16_t cmd_b1 : 4;
	uint16_t unused : 4;
	uint16_t cmd_b0 : 8;
} s_module_cmd_t;

typedef union {
	s_module_cmd_t s;
	uint16_t v;
} u_module_cmd_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[2];
	uint8_t voltage_b3;//电压(V) * 1024
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
	uint8_t input_voltage_b3;
	uint8_t input_voltage_b2;
	uint8_t input_voltage_b1;
	uint8_t input_voltage_b0;
} cmd_0x179_response_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[6];
} cmd_0x17a_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused1;
	uint8_t phase_voltage_status;
	uint8_t input_voltage_b3;
	uint8_t input_voltage_b2;
	uint8_t input_voltage_b1;
	uint8_t input_voltage_b0;
} cmd_0x17a_response_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused[6];
} cmd_0x17b_t;

typedef struct {
	u_module_cmd_t cmd;
	uint8_t unused1;
	uint8_t phase_voltage_status;
	uint8_t input_voltage_b3;
	uint8_t input_voltage_b2;
	uint8_t input_voltage_b1;
	uint8_t input_voltage_b0;
} cmd_0x17b_response_t;

typedef enum {
	MODULE_CMD_0x100_0x100 = 0,
	MODULE_CMD_0x103_0x103,
	MODULE_CMD_0x132_0x132,
	MODULE_CMD_0x14a_0x14a,
	MODULE_CMD_0x191_0x191,
	MODULE_CMD_0x183_0x183,
	MODULE_CMD_0x190_0x190,
	MODULE_CMD_0x179_0x179,
	MODULE_CMD_0x17a_0x17a,
	MODULE_CMD_0x17b_0x17b,
	MODULE_CMD_TOTAL,
} module_command_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_command_t cmd;
	uint32_t request_ext_id;
	uint16_t request_code;
	module_request_callback_t request_callback;
	uint32_t response_ext_id;
	uint16_t response_code;
	module_response_callback_t response_callback;
} module_command_item_t;

static char *get_power_module_cmd_des(module_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(MODULE_CMD_0x100_0x100);
			add_des_case(MODULE_CMD_0x103_0x103);
			add_des_case(MODULE_CMD_0x132_0x132);
			add_des_case(MODULE_CMD_0x14a_0x14a);
			add_des_case(MODULE_CMD_0x191_0x191);
			add_des_case(MODULE_CMD_0x183_0x183);
			add_des_case(MODULE_CMD_0x190_0x190);
			add_des_case(MODULE_CMD_0x179_0x179);
			add_des_case(MODULE_CMD_0x17a_0x17a);
			add_des_case(MODULE_CMD_0x17b_0x17b);

		default: {
		}
		break;
	}

	return des;
}

static void set_out_voltage(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage)//mv
{
	power_modules_info->power_module_info[module_id].setting_voltage = voltage * 1024 / 1000;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x100_0x100].state = CAN_COM_STATE_REQUEST;
}

static int request_0x100(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x100_t *cmd_0x100 = (cmd_0x100_t *)power_modules_info->can_tx_msg.Data;

	cmd_0x100->voltage_b0 = get_u8_b0_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0x100->voltage_b1 = get_u8_b1_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0x100->voltage_b2 = get_u8_b2_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0x100->voltage_b3 = get_u8_b3_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x100_0x100].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x100(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x100_0x100].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x100_0x100 = {
	.cmd = MODULE_CMD_0x100_0x100,
	.request_ext_id = POWER_ID_TX_CONTROL_HUAWEI,
	.request_code = 0x100,
	.request_callback = request_0x100,
	.response_ext_id = POWER_ID_RX_CONTROL_HUAWEI,
	.response_code = 0x100,
	.response_callback = response_0x100,
};

static void set_out_current(power_modules_info_t *power_modules_info, int module_id, uint32_t current)//mv
{
	power_modules_info->power_module_info[module_id].setting_current = current * 1024 / (power_modules_info->rate_current * 1000);
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x103_0x103].state = CAN_COM_STATE_REQUEST;
}

static int request_0x103(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x103_t *cmd_0x103 = (cmd_0x103_t *)power_modules_info->can_tx_msg.Data;

	cmd_0x103->current_b0 = get_u8_b0_from_u32(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0x103->current_b1 = get_u8_b1_from_u32(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0x103->current_b2 = get_u8_b2_from_u32(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0x103->current_b3 = get_u8_b3_from_u32(power_modules_info->power_module_info[module_id].setting_current);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x103_0x103].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x103(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x103_0x103].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static void set_out_voltage_current_huawei(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint32_t current)
{
	set_out_voltage(power_modules_info, module_id, voltage);
	set_out_current(power_modules_info, module_id, current);
}

static module_command_item_t module_command_item_0x103_0x103 = {
	.cmd = MODULE_CMD_0x103_0x103,
	.request_ext_id = POWER_ID_TX_CONTROL_HUAWEI,
	.request_code = 0x103,
	.request_callback = request_0x103,
	.response_ext_id = POWER_ID_RX_CONTROL_HUAWEI,
	.response_code = 0x103,
	.response_callback = response_0x103,
};

static void set_poweroff_huawei(power_modules_info_t *power_modules_info, int module_id, uint8_t poweroff)
{
	power_modules_info->power_module_info[module_id].poweroff = poweroff;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x132_0x132].state = CAN_COM_STATE_REQUEST;

	power_modules_info->power_module_info[module_id].automode = 1;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x14a_0x14a].state = CAN_COM_STATE_REQUEST;
}

static int request_0x132(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x132_t *cmd_0x132 = (cmd_0x132_t *)power_modules_info->can_tx_msg.Data;

	cmd_0x132->poweroff = power_modules_info->power_module_info[module_id].poweroff;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x132_0x132].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x132(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x132_0x132].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x132_0x132 = {
	.cmd = MODULE_CMD_0x132_0x132,
	.request_ext_id = POWER_ID_TX_CONTROL_HUAWEI,
	.request_code = 0x132,
	.request_callback = request_0x132,
	.response_ext_id = POWER_ID_RX_CONTROL_HUAWEI,
	.response_code = 0x132,
	.response_callback = response_0x132,
};

static int request_0x14a(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x14a_t *cmd_0x14a = (cmd_0x14a_t *)power_modules_info->can_tx_msg.Data;

	cmd_0x14a->output_voltage_auto_adapt = power_modules_info->power_module_info[module_id].automode;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x14a_0x14a].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x14a(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x14a_0x14a].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x14a_0x14a = {
	.cmd = MODULE_CMD_0x14a_0x14a,
	.request_ext_id = POWER_ID_TX_CONTROL_HUAWEI,
	.request_code = 0x14a,
	.request_callback = request_0x14a,
	.response_ext_id = POWER_ID_RX_CONTROL_HUAWEI,
	.response_code = 0x14a,
	.response_callback = response_0x14a,
};

static void query_status_huawei(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x191_0x191].state = CAN_COM_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x183_0x183].state = CAN_COM_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x190_0x190].state = CAN_COM_STATE_REQUEST;
}

static int request_0x191(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x191_t *cmd_0x191 = (cmd_0x191_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x191_0x191].state = CAN_COM_STATE_RESPONSE;
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

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x191_0x191].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x191_0x191 = {
	.cmd = MODULE_CMD_0x191_0x191,
	.request_ext_id = POWER_ID_TX_SINGLE_QUERY_HUAWEI,
	.request_code = 0x191,
	.request_callback = request_0x191,
	.response_ext_id = POWER_ID_RX_SINGLE_QUERY_HUAWEI,
	.response_code = 0x191,
	.response_callback = response_0x191,
};

static int request_0x183(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x183_t *cmd_0x183 = (cmd_0x183_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x183_0x183].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x183(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x183_response_t *cmd_0x183_response = (cmd_0x183_response_t *)power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].power_module_status.fan_state = cmd_0x183_response->status.module_0183_status_bit4;
	power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage = cmd_0x183_response->status.module_0183_status_bit22;
	power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage = cmd_0x183_response->status.module_0183_status_bit17;
	power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage = cmd_0x183_response->status.module_0183_status_bit0;
	power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage = cmd_0x183_response->status.module_0183_status_bit7;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent = cmd_0x183_response->status.module_0183_status_bit6;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature = cmd_0x183_response->status.module_0183_status_bit1;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x183_0x183].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x183_0x183 = {
	.cmd = MODULE_CMD_0x183_0x183,
	.request_ext_id = POWER_ID_TX_SINGLE_QUERY_HUAWEI,
	.request_code = 0x183,
	.request_callback = request_0x183,
	.response_ext_id = POWER_ID_RX_SINGLE_QUERY_HUAWEI,
	.response_code = 0x183,
	.response_callback = response_0x183,
};

static int request_0x190(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x190_t *cmd_0x190 = (cmd_0x190_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x190_0x190].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x190(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x190_response_t *cmd_0x190_response = (cmd_0x190_response_t *)power_modules_info->can_rx_msg->Data;

	//power_modules_info->power_module_info[module_id].setting_current =
	//    get_u16_from_u8_lh(cmd_0x190_response->setting_current_l, cmd_0x190_response->setting_current_h);

	//power_modules_info->power_module_info[module_id].setting_voltage =
	//    get_u16_from_u8_lh(cmd_0x190_response->setting_voltage_l, cmd_0x190_response->setting_voltage_h);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x190_0x190].state = CAN_COM_STATE_IDLE;

	power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff = cmd_0x190_response->status.module_0190_status_bit0;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x190_0x190 = {
	.cmd = MODULE_CMD_0x190_0x190,
	.request_ext_id = POWER_ID_TX_SINGLE_QUERY_HUAWEI,
	.request_code = 0x190,
	.request_callback = request_0x190,
	.response_ext_id = POWER_ID_RX_SINGLE_QUERY_HUAWEI,
	.response_code = 0x190,
	.response_callback = response_0x190,
};

static void query_a_line_input_voltage_huawei(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x179_0x179].state = CAN_COM_STATE_REQUEST;
}

static int request_0x179(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x179_t *cmd_0x179 = (cmd_0x179_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x179_0x179].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x179(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x179_response_t *cmd_0x179_response = (cmd_0x179_response_t *)power_modules_info->can_rx_msg->Data;
	uint32_t input_aline_voltage = 0;
	power_module_info_t *power_module_info;
	int i;

	input_aline_voltage = get_u32_from_u8_b0123(cmd_0x179_response->input_voltage_b0,
	                      cmd_0x179_response->input_voltage_b1,
	                      cmd_0x179_response->input_voltage_b2,
	                      cmd_0x179_response->input_voltage_b3);

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info = power_modules_info->power_module_info + i;
		power_module_info->input_aline_voltage = input_aline_voltage;
	}

	power_module_info = power_modules_info->power_module_info + module_id;

	power_module_info->cmd_ctx[MODULE_CMD_0x179_0x179].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x179_0x179 = {
	.cmd = MODULE_CMD_0x179_0x179,
	.request_ext_id = POWER_ID_TX_SINGLE_QUERY_HUAWEI,
	.request_code = 0x179,
	.request_callback = request_0x179,
	.response_ext_id = POWER_ID_RX_SINGLE_QUERY_HUAWEI,
	.response_code = 0x179,
	.response_callback = response_0x179,
};

static void query_b_line_input_voltage_huawei(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x17a_0x17a].state = CAN_COM_STATE_REQUEST;
}

static int request_0x17a(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x17a_t *cmd_0x17a = (cmd_0x17a_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x17a_0x17a].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x17a(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x17a_response_t *cmd_0x17a_response = (cmd_0x17a_response_t *)power_modules_info->can_rx_msg->Data;
	uint32_t input_bline_voltage = 0;
	power_module_info_t *power_module_info;
	int i;

	input_bline_voltage = get_u32_from_u8_b0123(cmd_0x17a_response->input_voltage_b0,
	                      cmd_0x17a_response->input_voltage_b1,
	                      cmd_0x17a_response->input_voltage_b2,
	                      cmd_0x17a_response->input_voltage_b3);

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info = power_modules_info->power_module_info + i;
		power_module_info->input_bline_voltage = input_bline_voltage;
	}

	power_module_info = power_modules_info->power_module_info + module_id;
	power_module_info->cmd_ctx[MODULE_CMD_0x17a_0x17a].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x17a_0x17a = {
	.cmd = MODULE_CMD_0x17a_0x17a,
	.request_ext_id = POWER_ID_TX_SINGLE_QUERY_HUAWEI,
	.request_code = 0x17a,
	.request_callback = request_0x17a,
	.response_ext_id = POWER_ID_RX_SINGLE_QUERY_HUAWEI,
	.response_code = 0x17a,
	.response_callback = response_0x17a,
};

static void query_c_line_input_voltage_huawei(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x17b_0x17b].state = CAN_COM_STATE_REQUEST;
}

static int request_0x17b(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x17b_t *cmd_0x17b = (cmd_0x17b_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x17b_0x17b].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x17b(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x17b_response_t *cmd_0x17b_response = (cmd_0x17b_response_t *)power_modules_info->can_rx_msg->Data;
	uint32_t input_cline_voltage = 0;
	power_module_info_t *power_module_info;
	int i;

	input_cline_voltage = get_u32_from_u8_b0123(cmd_0x17b_response->input_voltage_b0,
	                      cmd_0x17b_response->input_voltage_b1,
	                      cmd_0x17b_response->input_voltage_b2,
	                      cmd_0x17b_response->input_voltage_b3);

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info = power_modules_info->power_module_info + i;
		power_module_info->input_cline_voltage = input_cline_voltage;
	}

	power_module_info = power_modules_info->power_module_info + module_id;
	power_module_info->cmd_ctx[MODULE_CMD_0x17b_0x17b].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x17b_0x17b = {
	.cmd = MODULE_CMD_0x17b_0x17b,
	.request_ext_id = POWER_ID_TX_SINGLE_QUERY_HUAWEI,
	.request_code = 0x17b,
	.request_callback = request_0x17b,
	.response_ext_id = POWER_ID_RX_SINGLE_QUERY_HUAWEI,
	.response_code = 0x17b,
	.response_callback = response_0x17b,
};

static module_command_item_t *module_command_item_table[] = {
	&module_command_item_0x100_0x100,
	&module_command_item_0x103_0x103,
	&module_command_item_0x132_0x132,
	&module_command_item_0x14a_0x14a,
	&module_command_item_0x191_0x191,
	&module_command_item_0x183_0x183,
	&module_command_item_0x190_0x190,
	&module_command_item_0x179_0x179,
	&module_command_item_0x17a_0x17a,
	&module_command_item_0x17b_0x17b,
};

#define RESPONSE_TIMEOUT 200

static void power_modules_request_periodic(power_modules_info_t *power_modules_info)
{
	int module_id;
	int i;
	uint32_t ticks = osKernelSysTick();

	if(ticks - power_modules_info->periodic_stamp < 50) {
		return;
	}

	power_modules_info->periodic_stamp = ticks;

	for(module_id = 0; module_id < power_modules_info->power_module_number; module_id++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		can_com_cmd_ctx_t *module_cmd_ctx = power_module_info->cmd_ctx;
		can_com_connect_state_t *connect_state = &power_module_info->connect_state;

		for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
			module_command_item_t *item = module_command_item_table[i];
			can_com_cmd_ctx_t *cmd_ctx = module_cmd_ctx + item->cmd;

			if(cmd_ctx->state == CAN_COM_STATE_RESPONSE) {//超时
				if(ticks - cmd_ctx->send_stamp >= RESPONSE_TIMEOUT) {
					can_com_set_connect_state(connect_state, 0);
					debug("cmd %d(%s), module_id %d timeout, connect state:%d\n",
					      item->cmd,
					      get_power_module_cmd_des(item->cmd),
					      module_id,
					      can_com_get_connect_state(connect_state));
					cmd_ctx->state = CAN_COM_STATE_REQUEST;
				}
			}
		}
	}
}


static void power_modules_request_huawei(power_modules_info_t *power_modules_info)
{
	int module_id;
	int i;
	int ret;

	for(module_id = 0; module_id < power_modules_info->power_module_number; module_id++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		can_com_cmd_ctx_t *module_cmd_ctx = power_module_info->cmd_ctx;
		can_com_connect_state_t *connect_state = &power_module_info->connect_state;

		for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
			module_command_item_t *item = module_command_item_table[i];
			can_com_cmd_ctx_t *cmd_ctx = module_cmd_ctx + item->cmd;
			u_module_cmd_t *u_module_cmd = (u_module_cmd_t *)power_modules_info->can_tx_msg.Data;
			u_module_extid_t u_module_extid;
			uint32_t ticks = osKernelSysTick();

			power_modules_request_periodic(power_modules_info);

			if(cmd_ctx->state != CAN_COM_STATE_REQUEST) {
				continue;
			}

			u_module_extid.v = item->request_ext_id;
			u_module_extid.s.module_addr = module_id + 1;

			power_modules_info->can_tx_msg.ExtId = u_module_extid.v;
			power_modules_info->can_tx_msg.DLC = 8;

			memset(power_modules_info->can_tx_msg.Data, 0, 8);

			u_module_cmd->v = 0;
			u_module_cmd->s.cmd_b0 = get_u8_l_from_u16(item->request_code);
			u_module_cmd->s.cmd_b1 = get_u8_h_from_u16(item->request_code);

			ret = item->request_callback(power_modules_info, module_id);

			if(ret != 0) {
				debug("module_id %d cmd %d(%s) request error!\n",
				      module_id,
				      item->cmd,
				      get_power_module_cmd_des(item->cmd));
				continue;
			}

			ret = can_tx_data(power_modules_info->can_info, &power_modules_info->can_tx_msg, 10);

			if(ret != 0) {
				cmd_ctx->state = CAN_COM_STATE_REQUEST;
				can_com_set_connect_state(connect_state, 0);
				debug("send module_id %d cmd %d(%s) error!\n",
				      module_id,
				      item->cmd,
				      get_power_module_cmd_des(item->cmd));
			} else {
				cmd_ctx->send_stamp = ticks;
			}

			osDelay(5);
		}
	}
}

static int power_modules_response_huawei(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int i;
	u_module_extid_t u_module_extid;
	int module_addr;
	int module_id;
	uint32_t response_ext_id;
	u_module_cmd_t *u_module_cmd;
	uint16_t response_code;

	power_modules_info->can_rx_msg = can_rx_msg;

	u_module_extid.v = power_modules_info->can_rx_msg->ExtId;

	module_addr = u_module_extid.s.module_addr;
	u_module_extid.s.module_addr = 0;

	if((module_addr >= 1) && (module_addr <= power_modules_info->power_module_number)) {
		module_id = module_addr - 1;
	} else {
		return ret;
	}

	response_ext_id = u_module_extid.v;

	u_module_cmd = (u_module_cmd_t *)power_modules_info->can_rx_msg->Data;
	u_module_cmd->s.unused = 0;
	response_code = get_u16_from_u8_lh(u_module_cmd->s.cmd_b0, u_module_cmd->s.cmd_b1);

	for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
		module_command_item_t *item = module_command_item_table[i];
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		can_com_connect_state_t *connect_state = &power_module_info->connect_state;

		if(response_ext_id != item->response_ext_id) {
			continue;
		}

		if(response_code != item->response_code) {
			continue;
		}

		ret = item->response_callback(power_modules_info, module_id);

		if(ret == 0) {
			can_com_set_connect_state(connect_state, 1);
		} else {
			debug("module_id %d cmd %d(%s) response error!\n",
			      module_id,
			      item->cmd,
			      get_power_module_cmd_des(item->cmd));
		}

		ret = 0;
		break;
	}

	return ret;
}

power_modules_handler_t power_modules_handler_huawei = {
	.power_module_type = POWER_MODULE_TYPE_HUAWEI,
	.cmd_size = ARRAY_SIZE(module_command_item_table),
	.set_out_voltage_current = set_out_voltage_current_huawei,
	.set_poweroff = set_poweroff_huawei,
	.query_status = query_status_huawei,
	.query_a_line_input_voltage = query_a_line_input_voltage_huawei,
	.query_b_line_input_voltage = query_b_line_input_voltage_huawei,
	.query_c_line_input_voltage = query_c_line_input_voltage_huawei,
	.power_modules_request = power_modules_request_huawei,
	.power_modules_response = power_modules_response_huawei,
};
