

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_yyln.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月20日 星期二 11时42分56秒
 *   修改日期：2021年04月23日 星期五 09时36分56秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_yyln.h"

#include <string.h>

#include "log.h"

typedef struct {
	uint32_t serialnumberlowpart : 9;//0 (可选)
	uint32_t productionday : 5;//1~31 (可选)
	uint32_t moduleaddress : 7;//Module - (0x01~0x7F) BroadCast - 0
	uint32_t monitoraddress : 4;//Monitor - (0x01~0x0F) BroadCast - 0 default 1
	uint32_t protocol : 4;//0x01
	uint32_t unused : 3;
} yyln_ext_id_t;

typedef union {
	yyln_ext_id_t s;
	uint32_t v;
} u_yyln_ext_id_t;

#define LOCAL_ADDR 0x01

typedef enum {
	YYLN_FN_READ_VOLTAGE = 0,//模块输出电压 mV
	YYLN_FN_READ_CURRENT = 1,//模块输出电流 mA
	YYLN_FN_READ_CURRENT_ALT = 48,//模块输出电流 mA
	YYLN_FN_SET_VOLTAGE = 2,//模块输出电压设定值 mV
	YYLN_FN_SET_CURRENT = 3,//模块输出限流点设定值 mA
	YYLN_FN_SET_POWEROFF = 4,//DCDC 模块开关机(0 开机,1 关机)
	YYLN_FN_5 = 5,//模块序列号的低 32 位
	YYLN_FN_READ_STATE = 8,//模块状态标志位
	YYLN_FN_READ_LINE_AB = 20,//线电压 AB mV
	YYLN_FN_READ_LINE_BC = 21,//线电压 BC mV
	YYLN_FN_READ_LINE_CA = 22,//线电压 CA mV
	YYLN_FN_30 = 30,//进风口温度 毫度
	YYLN_FN_SET_GROUP_ADDR = 89,//组地址 R/W Y
	YYLN_FN_SET_OUT_MODE = 95,//设置模块高低压模式(1:1000V 2:500V) (只在关机状态下才可以设置成功)
	YYLN_FN_READ_SET_OUT_MODE = 96,//读模块高低压模式状态(1:1000V 2:500V)
	YYLN_FN_READ_OUT_MODE = 101,//读取模块实际的高低压档位状态(1:1000V 2:500V)
} yyln_fn_t;

typedef enum {
	MODULE_COMMAND_SET_VOLTAGE = 0,
	MODULE_COMMAND_SET_CURRENT,
	MODULE_COMMAND_SET_OUT_MODE,
	MODULE_COMMAND_GET_OUT_MODE,
	MODULE_COMMAND_SET_POWEROFF,
	MODULE_COMMAND_GET_VOLTAGE,
	MODULE_COMMAND_GET_CURRENT,
	MODULE_COMMAND_GET_STATE,
	MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE,
	MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE,
	MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE,
} module_command_t;

typedef struct {
	uint32_t input_over_voltage : 1;//模块交流输入故障 交流过压 1:异常状态 0:正常状态
	uint32_t input_under_voltage : 1;//模块交流输入故障 交流欠压 1:异常状态 0:正常状态
	uint32_t bit2 : 1;//模块保护 交流过压脱离(交流过压关机) 1:异常状态 0:正常状态
	uint32_t bit3 : 1;//PFC 母线过压 PFC 母线过压 1:异常状态 0:正常状态
	uint32_t bit4 : 1;//PFC 母线欠压 PFC 母线欠压 1:异常状态 0:正常状态
	uint32_t bit5 : 1;//PFC 母线不平衡 PFC 母线不平衡 1:异常状态 0:正常状态
	uint32_t output_over_voltage : 1;//直流输出过压 直流输出过压 1:异常状态 0:正常状态
	uint32_t protect_over_current : 1;//模块保护 直流过压关机 1:异常状态 0:正常状态
	uint32_t output_under_voltage : 1;//直流输出欠压 直流输出欠压 1:异常状态 0:正常状态
	//uint32_t fan_1 : 1;//风扇故障 风扇不运行
	//uint32_t fan_2 1;//预留 预留
	//uint32_t fan_3 : 1;//风扇驱动电路损坏 风扇驱动电路损坏 1:异常状态 0:正常状态
	uint32_t fan : 3;//风扇故障
	//uint32_t protect_over_temperature_1 : 1;//过温保护 环境过温 1:异常状态 0:正常状态
	//uint32_t protect_over_temperature_2 : 1;//环境温度过低 环境温度过低 1:异常状态 0:正常状态
	//uint32_t protect_over_temperature_3 : 1;//过温保护 PFC 过温保护 1 1:异常状态 0:正常状态
	//uint32_t protect_over_temperature_4 : 1;//过温保护 输出继电器故障 1:异常状态 0:正常状态
	//uint32_t protect_over_temperature_5 : 1;//过温保护 DC 过温保护 1 1:异常状态 0:正常状态
	//uint32_t protect_over_temperature_6 : 1;//过温保护 预留 预留
	uint32_t protect_over_temperature : 6;//过温保护
	uint32_t fault18 : 1;//模块故障 PFC 与 DCDC 通信故障 1:异常状态 0:正常状态
	uint32_t bit19 : 1;//预留 预留 预留
	uint32_t fault20 : 1;//模块故障 PFC 故障 1:异常状态 0:正常状态
	uint32_t fault21 : 1;//模块故障 DCDC 故障 1:异常状态 0:正常状态
	uint32_t bit22 : 1;//预留 预留 预留
	uint32_t bit23 : 1;//预留 预留 预留
	uint32_t bit24 : 1;//预留 预留 预留
	uint32_t poweroff : 1;//模块状态 DCDC 不运行 1:关机 0:开机
	uint32_t current_limit : 2;//输出环路状态 输出环路状态 3:预留,2:电流环 1:预留 0:电压环
	uint32_t bit28 : 1;//DC 输出电压不平衡 DC 输出电压不平衡 1:异常状态 0:正常状态
	uint32_t bit29 : 1;//发现相同序列号的模块 发现相同序列号的模块 1:异常状态 0:正常状态
	uint32_t bit30 : 1;//预留 预留 预留
	uint32_t fault31 : 1;//模块故障 泄放电路异常 1:异常状态 0:正常状态
} yyln_state_t;

typedef union {
	yyln_state_t s;
	uint32_t v;
} u_yyln_state_t;

typedef enum {
	YYLN_OUTPUT_MODE_VOLTAGE_LIMIT = 0,
	YYLN_OUTPUT_MODE_UNUSED1,
	YYLN_OUTPUT_MODE_CURRENT_LIMIT,
	YYLN_OUTPUT_MODE_UNUSED2,
} yyln_output_mode_t;

typedef enum {
	YYLN_MSG_TYPE_SET_REQUEST = 0x00,//设置模块参数
	YYLN_MSG_TYPE_SET_RESPONSE = 0x01,//模块设置应答
	YYLN_MSG_TYPE_READ_REQUEST = 0x02,//读模块信息
	YYLN_MSG_TYPE_READ_RESPONSE = 0x03,//模块读信息应答
	YYLN_MSG_TYPE_SERIAL_RESPONSE = 0x04,//模块读序列号应答
} yyln_msg_type_t;

typedef struct {
	uint8_t msg_type : 4;
	uint8_t group_addr : 4;//default 1
} yyln_msg_byte0_t;

typedef union {
	yyln_msg_byte0_t s;
	uint8_t v;
} u_yyln_msg_byte0_t;

typedef struct {
	uint16_t module_type : 10;
	uint16_t default_output_voltage : 6;
} yyln_msg_byte2_byte3_t;

typedef union {
	yyln_msg_byte2_byte3_t s;
	uint16_t v;
} u_yyln_msg_byte2_byte3_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_command_t cmd;
	uint8_t cmd_code;
	uint8_t request_type;
	module_request_callback_t request_callback;
	uint8_t response_type;
	module_response_callback_t response_callback;
} module_command_item_t;

static char *get_power_module_cmd_des(module_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(MODULE_COMMAND_SET_VOLTAGE);
			add_des_case(MODULE_COMMAND_SET_CURRENT);
			add_des_case(MODULE_COMMAND_SET_OUT_MODE);
			add_des_case(MODULE_COMMAND_GET_OUT_MODE);
			add_des_case(MODULE_COMMAND_SET_POWEROFF);
			add_des_case(MODULE_COMMAND_GET_VOLTAGE);
			add_des_case(MODULE_COMMAND_GET_CURRENT);
			add_des_case(MODULE_COMMAND_GET_STATE);
			add_des_case(MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE);
			add_des_case(MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE);
			add_des_case(MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE);

		default: {
		}
		break;
	}

	return des;
}

static void _set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_VOLTAGE].state = COMMAND_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_CURRENT].state = COMMAND_STATE_REQUEST;
}

static int request_set_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint32_t voltage = power_modules_info->power_module_info[module_id].setting_voltage;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(voltage);
	data[5] = get_u8_b2_from_u32(voltage);
	data[6] = get_u8_b1_from_u32(voltage);
	data[7] = get_u8_b0_from_u32(voltage);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_VOLTAGE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_set_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_voltage = {
	.cmd = MODULE_COMMAND_SET_VOLTAGE,
	.cmd_code = YYLN_FN_SET_VOLTAGE,
	.request_type = YYLN_MSG_TYPE_SET_REQUEST,
	.request_callback = request_set_voltage,
	.response_type = YYLN_MSG_TYPE_SET_RESPONSE,
	.response_callback = response_set_voltage,
};

static int request_set_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint32_t current = power_modules_info->power_module_info[module_id].setting_current;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(current);
	data[5] = get_u8_b2_from_u32(current);
	data[6] = get_u8_b1_from_u32(current);
	data[7] = get_u8_b0_from_u32(current);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_CURRENT].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_set_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_CURRENT].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_current = {
	.cmd = MODULE_COMMAND_SET_CURRENT,
	.cmd_code = YYLN_FN_SET_CURRENT,
	.request_type = YYLN_MSG_TYPE_SET_REQUEST,
	.request_callback = request_set_current,
	.response_type = YYLN_MSG_TYPE_SET_RESPONSE,
	.response_callback = response_set_current,
};

static int request_get_out_mode(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_OUT_MODE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_get_out_mode(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	uint8_t out_mode = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]);
	uint32_t voltage = power_modules_info->power_module_info[module_id].setting_voltage;

	uint8_t require_output_mode = (voltage >= 500000) ? 1 : 2;

	if(require_output_mode == out_mode) {
		power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_REQUEST;
	} else {
		power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_OUT_MODE].state = COMMAND_STATE_REQUEST;
	}

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_OUT_MODE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_out_mode = {
	.cmd = MODULE_COMMAND_GET_OUT_MODE,
	.cmd_code = YYLN_FN_READ_SET_OUT_MODE,
	.request_type = YYLN_MSG_TYPE_READ_REQUEST,
	.request_callback = request_get_out_mode,
	.response_type = YYLN_MSG_TYPE_READ_RESPONSE,
	.response_callback = response_get_out_mode,
};

static int request_set_out_mode(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint8_t output_mode;
	uint32_t voltage = power_modules_info->power_module_info[module_id].setting_voltage;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	output_mode = (voltage >= 500000) ? 1 : 2;

	data[1] = YYLN_FN_SET_OUT_MODE;
	data[4] = get_u8_b3_from_u32(output_mode);
	data[5] = get_u8_b2_from_u32(output_mode);
	data[6] = get_u8_b1_from_u32(output_mode);
	data[7] = get_u8_b0_from_u32(output_mode);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_OUT_MODE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_set_out_mode(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_OUT_MODE].state = COMMAND_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_OUT_MODE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_out_mode = {
	.cmd = MODULE_COMMAND_SET_OUT_MODE,
	.cmd_code = YYLN_FN_SET_OUT_MODE,
	.request_type = YYLN_MSG_TYPE_SET_REQUEST,
	.request_callback = request_set_out_mode,
	.response_type = YYLN_MSG_TYPE_SET_RESPONSE,
	.response_callback = response_set_out_mode,
};

static int request_set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint8_t poweroff = power_modules_info->power_module_info[module_id].power_module_status.poweroff;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(poweroff);
	data[5] = get_u8_b2_from_u32(poweroff);
	data[6] = get_u8_b1_from_u32(poweroff);
	data[7] = get_u8_b0_from_u32(poweroff);

	power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff = poweroff;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_poweroff = {
	.cmd = MODULE_COMMAND_SET_POWEROFF,
	.cmd_code = YYLN_FN_SET_POWEROFF,
	.request_type = YYLN_MSG_TYPE_SET_REQUEST,
	.request_callback = request_set_poweroff,
	.response_type = YYLN_MSG_TYPE_SET_RESPONSE,
	.response_callback = response_set_poweroff,
};

static void _set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	uint8_t poweroff = power_modules_info->power_module_info[module_id].poweroff;

	if(poweroff == 1) {
		power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_REQUEST;
	} else {
		power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_OUT_MODE].state = COMMAND_STATE_REQUEST;
	}
}

static void _query_status(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_VOLTAGE].state = COMMAND_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CURRENT].state = COMMAND_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_STATE].state = COMMAND_STATE_REQUEST;
}

static int request_get_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_VOLTAGE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_get_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].output_voltage = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]) / 100;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_voltage = {
	.cmd = MODULE_COMMAND_GET_VOLTAGE,
	.cmd_code = YYLN_FN_READ_VOLTAGE,
	.request_type = YYLN_MSG_TYPE_READ_REQUEST,
	.request_callback = request_get_voltage,
	.response_type = YYLN_MSG_TYPE_READ_RESPONSE,
	.response_callback = response_get_voltage,
};

static int request_get_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CURRENT].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_get_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].output_current = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]) / 100;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CURRENT].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_current = {
	.cmd = MODULE_COMMAND_GET_CURRENT,
	.cmd_code = YYLN_FN_READ_CURRENT_ALT,
	.request_type = YYLN_MSG_TYPE_READ_REQUEST,
	.request_callback = request_get_current,
	.response_type = YYLN_MSG_TYPE_READ_RESPONSE,
	.response_callback = response_get_current,
};

static int request_get_state(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_STATE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_get_state(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	power_module_status_t *power_module_status = &power_modules_info->power_module_info[module_id].power_module_status;
	u_yyln_state_t u_yyln_state;

	u_yyln_state.v = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]);

	power_module_status->poweroff = u_yyln_state.s.poweroff;
	power_module_status->fault = (u_yyln_state.s.fault18 != 0
	                              || u_yyln_state.s.fault20 != 0
	                              || u_yyln_state.s.fault21 != 0
	                              || u_yyln_state.s.fault31 != 0) ? 1 : 0;
	power_module_status->fan_state = u_yyln_state.s.fan;
	power_module_status->input_overvoltage = u_yyln_state.s.input_over_voltage;
	power_module_status->input_lowvoltage = u_yyln_state.s.input_under_voltage;
	power_module_status->output_overvoltage = u_yyln_state.s.output_over_voltage;
	power_module_status->output_lowvoltage = u_yyln_state.s.output_under_voltage;
	power_module_status->protect_overcurrent = u_yyln_state.s.protect_over_current;
	power_module_status->protect_overtemperature = (u_yyln_state.s.protect_over_temperature != 0) ? 1 : 0;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_STATE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_state = {
	.cmd = MODULE_COMMAND_GET_STATE,
	.cmd_code = YYLN_FN_READ_STATE,
	.request_type = YYLN_MSG_TYPE_READ_REQUEST,
	.request_callback = request_get_state,
	.response_type = YYLN_MSG_TYPE_READ_RESPONSE,
	.response_callback = response_get_state,
};

static void _query_aline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE].state = COMMAND_STATE_REQUEST;
}

static int request_get_aline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_get_aline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].input_cline_voltage = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]) * LINE_TO_PHASE_COEFFICIENT / 100;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_aline_input_voltage = {
	.cmd = MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE,
	.cmd_code = YYLN_FN_READ_LINE_AB,
	.request_type = YYLN_MSG_TYPE_READ_REQUEST,
	.request_callback = request_get_aline_input_voltage,
	.response_type = YYLN_MSG_TYPE_READ_RESPONSE,
	.response_callback = response_get_aline_input_voltage,
};

static void _query_bline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE].state = COMMAND_STATE_REQUEST;
}

static int request_get_bline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_get_bline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].input_bline_voltage = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]) * LINE_TO_PHASE_COEFFICIENT / 100;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_bline_input_voltage = {
	.cmd = MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE,
	.cmd_code = YYLN_FN_READ_LINE_BC,
	.request_type = YYLN_MSG_TYPE_READ_REQUEST,
	.request_callback = request_get_bline_input_voltage,
	.response_type = YYLN_MSG_TYPE_READ_RESPONSE,
	.response_callback = response_get_bline_input_voltage,
};

static void _query_cline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE].state = COMMAND_STATE_REQUEST;
}

static int request_get_cline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	u_yyln_msg_byte2_byte3_t *u_yyln_msg_byte2_byte3 = (u_yyln_msg_byte2_byte3_t *)&data[2];

	u_yyln_msg_byte2_byte3->v = 0;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_get_cline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].input_cline_voltage = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]) * LINE_TO_PHASE_COEFFICIENT / 100;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_cline_input_voltage = {
	.cmd = MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE,
	.cmd_code = YYLN_FN_READ_LINE_CA,
	.request_type = YYLN_MSG_TYPE_READ_REQUEST,
	.request_callback = request_get_cline_input_voltage,
	.response_type = YYLN_MSG_TYPE_READ_RESPONSE,
	.response_callback = response_get_cline_input_voltage,
};

static module_command_item_t *module_command_item_table[] = {
	&module_command_item_set_voltage,
	&module_command_item_set_current,
	&module_command_item_get_out_mode,
	&module_command_item_set_out_mode,
	&module_command_item_set_poweroff,
	&module_command_item_get_voltage,
	&module_command_item_get_current,
	&module_command_item_get_state,
	&module_command_item_get_aline_input_voltage,
	&module_command_item_get_bline_input_voltage,
	&module_command_item_get_cline_input_voltage,
};

#define RESPONSE_TIMEOUT 500

static void power_modules_request_periodic(power_modules_info_t *power_modules_info)
{
	int module_id;
	int i;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, power_modules_info->periodic_stamp) < 50) {
		return;
	}

	power_modules_info->periodic_stamp = ticks;

	for(module_id = 0; module_id < power_modules_info->power_module_number; module_id++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		command_status_t *module_cmd_ctx = power_module_info->cmd_ctx;
		connect_state_t *connect_state = &power_module_info->connect_state;

		for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
			module_command_item_t *item = module_command_item_table[i];
			command_status_t *cmd_ctx = module_cmd_ctx + item->cmd;

			if(cmd_ctx->state == COMMAND_STATE_RESPONSE) {//超时
				if(ticks_duration(ticks, cmd_ctx->send_stamp) >= RESPONSE_TIMEOUT) {
					update_connect_state(connect_state, 0);
					debug("module_id %d cmd %d(%s) timeout, connect state:%d",
					      module_id,
					      item->cmd,
					      get_power_module_cmd_des(item->cmd),
					      get_connect_state(connect_state));
					cmd_ctx->state = COMMAND_STATE_REQUEST;
				}
			}
		}
	}
}

static void _power_modules_request(power_modules_info_t *power_modules_info)
{
	int module_id;
	int i;
	int ret;

	for(module_id = 0; module_id < power_modules_info->power_module_number; module_id++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		command_status_t *module_cmd_ctx = power_module_info->cmd_ctx;
		connect_state_t *connect_state = &power_module_info->connect_state;

		for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
			module_command_item_t *item = module_command_item_table[i];
			command_status_t *cmd_ctx = module_cmd_ctx + item->cmd;
			u_yyln_ext_id_t u_yyln_ext_id;
			uint32_t ticks = osKernelSysTick();
			u_yyln_msg_byte0_t *u_yyln_msg_byte0;

			power_modules_request_periodic(power_modules_info);

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_yyln_ext_id.v = 0;
			u_yyln_ext_id.s.monitoraddress = LOCAL_ADDR;
			u_yyln_ext_id.s.moduleaddress = module_id + 1;
			u_yyln_ext_id.s.protocol = 1;

			power_modules_info->can_tx_msg.ExtId = u_yyln_ext_id.v;
			power_modules_info->can_tx_msg.RTR = CAN_RTR_DATA;
			power_modules_info->can_tx_msg.IDE = CAN_ID_EXT;
			power_modules_info->can_tx_msg.DLC = 8;

			memset(power_modules_info->can_tx_msg.Data, 0, 8);

			u_yyln_msg_byte0 = (u_yyln_msg_byte0_t *)&power_modules_info->can_tx_msg.Data[0];
			u_yyln_msg_byte0->s.msg_type = item->request_type;
			u_yyln_msg_byte0->s.group_addr = 1;
			power_modules_info->can_tx_msg.Data[1] = item->cmd_code;

			ret = item->request_callback(power_modules_info, module_id);

			if(ret != 0) {
				debug("module_id %d cmd %d(%s) request error!",
				      module_id,
				      item->cmd,
				      get_power_module_cmd_des(item->cmd));
				continue;
			}

			ret = can_tx_data(power_modules_info->can_info, &power_modules_info->can_tx_msg, 10);

			if(ret != 0) {
				cmd_ctx->state = COMMAND_STATE_REQUEST;

				update_connect_state(connect_state, 0);
				debug("module_id %d cmd %d(%s) send error!",
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

static int _power_modules_response(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int i;
	u_yyln_ext_id_t u_yyln_ext_id;
	int module_addr;
	int module_id;
	u_yyln_msg_byte0_t *u_yyln_msg_byte0 = (u_yyln_msg_byte0_t *)&can_rx_msg->Data[0];

	power_modules_info->can_rx_msg = can_rx_msg;

	u_yyln_ext_id.v = power_modules_info->can_rx_msg->ExtId;

	module_addr = u_yyln_ext_id.s.moduleaddress;

	if((module_addr >= 1) && (module_addr <= power_modules_info->power_module_number)) {
		module_id = module_addr - 1;
	} else {
		return ret;
	}

	if(u_yyln_ext_id.s.monitoraddress != LOCAL_ADDR) {
		return ret;
	}

	if(u_yyln_msg_byte0->s.msg_type == YYLN_MSG_TYPE_SET_RESPONSE
	   || u_yyln_msg_byte0->s.msg_type == YYLN_MSG_TYPE_READ_RESPONSE
	   || u_yyln_msg_byte0->s.msg_type == YYLN_MSG_TYPE_SERIAL_RESPONSE) {
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
		module_command_item_t *item = module_command_item_table[i];
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		connect_state_t *connect_state = &power_module_info->connect_state;

		if(u_yyln_msg_byte0->s.msg_type != item->response_type) {
			continue;
		}

		if(can_rx_msg->Data[1] != item->cmd_code) {
			continue;
		}

		update_connect_state(connect_state, 1);

		ret = item->response_callback(power_modules_info, module_id);

		if(ret != 0) {
			debug("module_id %d cmd %d(%s) response error!",
			      module_id,
			      item->cmd,
			      get_power_module_cmd_des(item->cmd));
		}

		ret = 0;
		break;
	}

	return ret;
}

power_modules_handler_t power_modules_handler_yyln = {
	.power_module_type = POWER_MODULE_TYPE_YYLN,
	.cmd_size = ARRAY_SIZE(module_command_item_table),
	.set_out_voltage_current = _set_out_voltage_current,
	.set_poweroff = _set_poweroff,
	.query_status = _query_status,
	.query_a_line_input_voltage = _query_aline_input_voltage,
	.query_b_line_input_voltage = _query_bline_input_voltage,
	.query_c_line_input_voltage = _query_cline_input_voltage,
	.power_modules_request = _power_modules_request,
	.power_modules_response = _power_modules_response,
};
