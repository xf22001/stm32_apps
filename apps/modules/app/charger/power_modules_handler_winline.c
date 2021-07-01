

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_winline.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月22日 星期四 14时48分47秒
 *   修改日期：2021年06月17日 星期四 15时07分25秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_winline.h"

#include <string.h>

#define LOG_DISABLE
#include "log.h"

#define LOCAL_ADDR 0xf0

typedef struct {
	uint32_t group : 3;//组(0-7)
	uint32_t src : 8;//地址范围:00~63; 监控地址固定为:0xF0; 广播地址:0xFF; 组内广播地址:0xFE
	uint32_t dst : 8;//地址范围:00~63; 监控地址固定为:0xF0; 广播地址:0xFF; 组内广播地址:0xFE
	uint32_t ptp : 1;//PTP=1,点对点通信; PTP=0,广播通信;
	uint32_t protno : 9;//默认:PRONTO=0x060
	uint32_t unused : 3;
} winline_ext_id_t;

typedef union {
	winline_ext_id_t s;
	uint32_t v;
} u_winline_ext_id_t;

typedef enum {
	WINLINE_FN_SETTING = 0x03,
	WINLINE_FN_GETTING = 0x10,
} winline_fn_t;

typedef enum {
	WINLINE_REG_SETTING_VOLTAGE = 0x0021,
	WINLINE_REG_SETTING_CURRENT = 0x0022,
	WINLINE_REG_SETTING_CURRENT_ALT = 0x001b,
	WINLINE_REG_SETTING_POWEROFF = 0x0030,
	WINLINE_REG_GETTING_VOLTAGE = 0x0001,
	WINLINE_REG_GETTING_CURRENT = 0x0002,
	WINLINE_REG_GETTING_STATUS = 0x0040,
	WINLINE_REG_GETTING_PHASE_VOLTAGE_A = 0x000c,
	WINLINE_REG_GETTING_PHASE_VOLTAGE_B = 0x000d,
	WINLINE_REG_GETTING_PHASE_VOLTAGE_C = 0x000e,
} winline_reg_t;

typedef enum {
	WINLINE_DATA_FORMAT_FLOAT = 0x41,
	WINLINE_DATA_FORMAT_INT = 0x42,
} winline_data_format_t;

typedef enum {
	WINLINE_DATA_STATUS_OK = 0xf0,
	WINLINE_DATA_STATUS_FAIL = 0xf2,
} winline_data_status_t;

typedef union {
	uint32_t u32;
	float f;
} u_uint32_float_t;

typedef struct {
	uint32_t fault : 1;
	uint32_t protect : 1;
	uint32_t unused1 : 1;
	uint32_t sci : 1;
	uint32_t input : 1;
	uint32_t input_unmatch : 1;
	uint32_t unused2 : 1;
	uint32_t dcdc_low : 1;
	uint32_t pfc_voltage : 1;
	uint32_t input_over_voltage : 1;
	uint32_t unused3 : 1;
	uint32_t unused4 : 1;
	uint32_t unused5 : 1;
	uint32_t unused6 : 1;
	uint32_t input_under_voltage : 1;
	uint32_t unused7 : 1;
	uint32_t can : 1;
	uint32_t module_uneven_current : 1;
	uint32_t unused8 : 1;
	uint32_t unused9 : 1;
	uint32_t unused10 : 1;
	uint32_t unused11 : 1;
	uint32_t poweroff : 1;
	uint32_t current_limit : 1;
	uint32_t temperature_limit : 1;
	uint32_t input_limit : 1;
	uint32_t unused12 : 1;
	uint32_t fan : 1;
	uint32_t protect_over_current : 1;
	uint32_t unused13 : 1;
	uint32_t protect_over_temperature : 1;
	uint32_t output_over_voltage : 1;
} winline_module_stauts_t;

typedef union {
	winline_module_stauts_t s;
	uint32_t v;
} u_winline_module_stauts_t;

typedef enum {
	MODULE_COMMAND_SET_VOLTAGE = 0,
	MODULE_COMMAND_SET_CURRENT,
	MODULE_COMMAND_SET_POWEROFF,
	MODULE_COMMAND_GET_VOLTAGE,
	MODULE_COMMAND_GET_CURRENT,
	MODULE_COMMAND_GET_STATE,
	MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE,
	MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE,
	MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE,
} module_command_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_command_t cmd;
	uint16_t cmd_code;
	uint8_t request_type;
	module_request_callback_t request_callback;
	module_response_callback_t response_callback;
} module_command_item_t;

static char *get_power_module_cmd_des(module_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(MODULE_COMMAND_SET_VOLTAGE);
			add_des_case(MODULE_COMMAND_SET_CURRENT);
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

static int request_callback_set_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	u_uint32_float_t u32_f;

	u32_f.f = power_modules_info->power_module_info[module_id].setting_voltage * 1.0 / 1000;

	data[4] = get_u8_b3_from_u32(u32_f.u32);
	data[5] = get_u8_b2_from_u32(u32_f.u32);
	data[6] = get_u8_b1_from_u32(u32_f.u32);
	data[7] = get_u8_b0_from_u32(u32_f.u32);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_VOLTAGE].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_set_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_voltage = {
	.cmd = MODULE_COMMAND_SET_VOLTAGE,
	.cmd_code = WINLINE_REG_SETTING_VOLTAGE,
	.request_type = WINLINE_FN_SETTING,
	.request_callback = request_callback_set_voltage,
	.response_callback = response_callback_set_voltage,
};

static int request_callback_set_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint32_t current = power_modules_info->power_module_info[module_id].setting_current * 1024 / 1000;

	data[4] = get_u8_b3_from_u32(current);
	data[5] = get_u8_b2_from_u32(current);
	data[6] = get_u8_b1_from_u32(current);
	data[7] = get_u8_b0_from_u32(current);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_CURRENT].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_set_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_CURRENT].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_current = {
	.cmd = MODULE_COMMAND_SET_CURRENT,
	.cmd_code = WINLINE_REG_SETTING_CURRENT_ALT,
	.request_type = WINLINE_FN_SETTING,
	.request_callback = request_callback_set_current,
	.response_callback = response_callback_set_current,
};

static void _set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_VOLTAGE].state = COMMAND_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_CURRENT].state = COMMAND_STATE_REQUEST;
}

static int request_callback_set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint8_t poweroff = power_modules_info->power_module_info[module_id].poweroff;

	data[4] = 0;
	data[5] = poweroff;
	data[6] = 0;
	data[7] = 0;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff = data[5];
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_poweroff = {
	.cmd = MODULE_COMMAND_SET_POWEROFF,
	.cmd_code = WINLINE_REG_SETTING_POWEROFF,
	.request_type = WINLINE_FN_SETTING,
	.request_callback = request_callback_set_poweroff,
	.response_callback = response_callback_set_poweroff,
};

static void _set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_REQUEST;
}

static int request_callback_get_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_VOLTAGE].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_get_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	u_uint32_float_t u32_f;

	u32_f.u32 = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]);
	power_modules_info->power_module_info[module_id].output_voltage = u32_f.f * 10;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_voltage = {
	.cmd = MODULE_COMMAND_GET_VOLTAGE,
	.cmd_code = WINLINE_REG_GETTING_VOLTAGE,
	.request_type = WINLINE_FN_GETTING,
	.request_callback = request_callback_get_voltage,
	.response_callback = response_callback_get_voltage,
};

static int request_callback_get_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CURRENT].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_get_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	u_uint32_float_t u32_f;

	u32_f.u32 = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]);
	power_modules_info->power_module_info[module_id].output_current = u32_f.f * 10;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CURRENT].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_current = {
	.cmd = MODULE_COMMAND_GET_CURRENT,
	.cmd_code = WINLINE_REG_GETTING_CURRENT,
	.request_type = WINLINE_FN_GETTING,
	.request_callback = request_callback_get_current,
	.response_callback = response_callback_get_current,
};

static int request_callback_get_state(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_STATE].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_get_state(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	u_winline_module_stauts_t u_winline_module_stauts;
	u_winline_module_stauts.v = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]);

	power_modules_info->power_module_info[module_id].power_module_status.poweroff = u_winline_module_stauts.s.poweroff;
	power_modules_info->power_module_info[module_id].power_module_status.fault = u_winline_module_stauts.s.fault;
	power_modules_info->power_module_info[module_id].power_module_status.output_state = u_winline_module_stauts.s.current_limit;
	power_modules_info->power_module_info[module_id].power_module_status.fan_state = u_winline_module_stauts.s.fan;
	power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage = u_winline_module_stauts.s.input_over_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage = u_winline_module_stauts.s.input_under_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage = u_winline_module_stauts.s.output_over_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage = 0;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent = u_winline_module_stauts.s.protect_over_current;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature = u_winline_module_stauts.s.protect_over_temperature;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_STATE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_state = {
	.cmd = MODULE_COMMAND_GET_STATE,
	.cmd_code = WINLINE_REG_GETTING_STATUS,
	.request_type = WINLINE_FN_GETTING,
	.request_callback = request_callback_get_state,
	.response_callback = response_callback_get_state,
};

static void _query_status(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_VOLTAGE].state = COMMAND_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CURRENT].state = COMMAND_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_STATE].state = COMMAND_STATE_REQUEST;
}

static int request_callback_get_aline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_get_aline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	u_uint32_float_t u32_f;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	u32_f.u32 = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]);
	power_modules_info->power_module_info[module_id].input_aline_voltage = u32_f.f * 10 * LINE_TO_PHASE_COEFFICIENT;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_aline_input_voltage = {
	.cmd = MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE,
	.cmd_code = WINLINE_REG_GETTING_PHASE_VOLTAGE_A,
	.request_type = WINLINE_FN_GETTING,
	.request_callback = request_callback_get_aline_input_voltage,
	.response_callback = response_callback_get_aline_input_voltage,
};

static int request_callback_get_bline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_get_bline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	u_uint32_float_t u32_f;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	u32_f.u32 = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]);
	power_modules_info->power_module_info[module_id].input_bline_voltage = u32_f.f * 10 * LINE_TO_PHASE_COEFFICIENT;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_bline_input_voltage = {
	.cmd = MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE,
	.cmd_code = WINLINE_REG_GETTING_PHASE_VOLTAGE_B,
	.request_type = WINLINE_FN_GETTING,
	.request_callback = request_callback_get_bline_input_voltage,
	.response_callback = response_callback_get_bline_input_voltage,
};

static int request_callback_get_cline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	data[4] = get_u8_b3_from_u32(0);
	data[5] = get_u8_b2_from_u32(0);
	data[6] = get_u8_b1_from_u32(0);
	data[7] = get_u8_b0_from_u32(0);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_get_cline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	u_uint32_float_t u32_f;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	u32_f.u32 = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]);
	power_modules_info->power_module_info[module_id].input_cline_voltage = u32_f.f * 10 * LINE_TO_PHASE_COEFFICIENT;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_cline_input_voltage = {
	.cmd = MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE,
	.cmd_code = WINLINE_REG_GETTING_PHASE_VOLTAGE_C,
	.request_type = WINLINE_FN_GETTING,
	.request_callback = request_callback_get_cline_input_voltage,
	.response_callback = response_callback_get_cline_input_voltage,
};

void _query_aline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_ALINE_INPUT_VOLTAGE].state = COMMAND_STATE_REQUEST;
}

void _query_bline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_BLINE_INPUT_VOLTAGE].state = COMMAND_STATE_REQUEST;
}

void _query_cline_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_CLINE_INPUT_VOLTAGE].state = COMMAND_STATE_REQUEST;
}

static module_command_item_t *module_command_item_table[] = {
	&module_command_item_set_voltage,
	&module_command_item_set_current,
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
			u_winline_ext_id_t u_winline_ext_id;
			uint8_t *data;
			uint32_t ticks = osKernelSysTick();

			power_modules_request_periodic(power_modules_info);

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_winline_ext_id.v = 0;
			u_winline_ext_id.s.group = 0;
			u_winline_ext_id.s.src = LOCAL_ADDR;
			u_winline_ext_id.s.dst = module_id + 1;
			u_winline_ext_id.s.ptp = 1;
			u_winline_ext_id.s.protno = 0x060;

			power_modules_info->can_tx_msg.ExtId = u_winline_ext_id.v;
			power_modules_info->can_tx_msg.RTR = CAN_RTR_DATA;
			power_modules_info->can_tx_msg.IDE = CAN_ID_EXT;
			power_modules_info->can_tx_msg.DLC = 8;

			memset(power_modules_info->can_tx_msg.Data, 0, 8);

			data = power_modules_info->can_tx_msg.Data;
			data[0] = item->request_type;
			data[2] = get_u8_h_from_u16(item->cmd_code);
			data[3] = get_u8_l_from_u16(item->cmd_code);

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
	u_winline_ext_id_t u_winline_ext_id;
	int module_addr;
	int module_id;
	uint8_t *data = can_rx_msg->Data;
	uint16_t cmd_code = get_u16_from_u8_lh(data[3], data[2]);

	power_modules_info->can_rx_msg = can_rx_msg;

	u_winline_ext_id.v = power_modules_info->can_rx_msg->ExtId;

	module_addr = u_winline_ext_id.s.src;

	if((module_addr >= 1) && (module_addr <= power_modules_info->power_module_number)) {
		module_id = module_addr - 1;
	} else {
		return ret;
	}

	if(u_winline_ext_id.s.dst != LOCAL_ADDR) {
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
		module_command_item_t *item = module_command_item_table[i];
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		connect_state_t *connect_state = &power_module_info->connect_state;

		if(cmd_code != item->cmd_code) {
			continue;
		}

		update_connect_state(connect_state, 1);

		if((WINLINE_DATA_STATUS_OK != data[1]) && (WINLINE_DATA_STATUS_FAIL != data[1])) {
			_hexdump("response data", data, 8);
			debug("module_id %d cmd %d(%s) response failed!",
			      module_id,
			      item->cmd,
			      get_power_module_cmd_des(item->cmd));
			continue;
		}

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

power_modules_handler_t power_modules_handler_winline = {
	.power_module_type = POWER_MODULE_TYPE_WINLINE,
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
