

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_zte.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月23日 星期五 09时40分26秒
 *   修改日期：2021年04月23日 星期五 12时56分12秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_zte.h"
#include <string.h>
#include "log.h"

typedef struct {
	uint8_t dp : 1;
	uint8_t edp : 1;
	uint8_t p : 3;
	uint8_t unused : 3;
} p_edp_dp_t;

typedef union {
	p_edp_dp_t s;
	uint8_t v;
} u_p_edp_dp_t;

typedef struct {
	uint32_t src : 8;
	uint32_t dst : 8;
	uint32_t pf : 8;
	u_p_edp_dp_t u_p_edp_dp;
} zte_ext_id_t;

typedef union {
	zte_ext_id_t s;
	uint32_t v;
} u_zte_ext_id_t;

#define LOCAL_ADDR 0x80

typedef enum {
	ZTE_PF_MODULE_STATE_RESPONSE = 0x01,//充电模块状态报文(上行)
	ZTE_PF_MODULE_ON_OFF_RESPONSE = 0x02,//特定模块启停确认(上行)
	ZTE_PF_MODULE_QUERY_REQUEST = 0x05,//特定模块查询命令(下行)
	ZTE_PF_MODULE_ON_OFF_REQUEST = 0x06,//特定模块启停(下行)
	ZTE_PF_MODULE_STATUS2 = 0x0d,//模拟量 2 查询响应(上行)
} zte_pf_type_t;

typedef enum {
	ZTE_MODULE_QUERY_FN_STATE = 0x01,//查询充电模块状态
	ZTE_MODULE_QUERY_FN_STATUS2 = 0x07,//查询模拟量 2(三相输入电压—允许广播查询)
} zte_module_query_fn_t;

typedef enum {
	POWER_ON_OFF_ON = 0xaa,
	POWER_ON_OFF_OFF = 0x55,
} power_on_off_t;

typedef struct {
	uint8_t poweron : 1;//(开机)
	uint8_t poweroff : 1;//(关机)
	uint8_t starting : 1;//(缓启动)
	uint8_t sleeping : 1;//(休眠)
	uint8_t update : 1;//(更新)
	uint8_t powerlimit : 1;//(限功率)
	uint8_t currentlimit : 1;//(限电流)
	uint8_t onekey : 1;//(一键功能)
} module_state_t;

typedef union {
	module_state_t s;
	uint8_t v;
} u_module_state_t;

typedef struct {
	uint32_t input_under_voltage : 1;
	uint32_t bit1 : 1;
	uint32_t input_over_voltage : 1;
	uint32_t output_over_voltage : 1;
	uint32_t protect_over_current : 1;
	uint32_t protect_over_temperature : 1;
	uint32_t fan : 1;
	uint32_t bit7 : 1;
	uint32_t output_under_voltage : 1;
	uint32_t bit9 : 1;
	uint32_t bit10 : 1;
	uint32_t bit11 : 1;
	uint32_t bit12 : 1;
	uint32_t bit13 : 1;
	uint32_t bit14 : 1;
	uint32_t bit15 : 1;
	uint32_t bit16 : 1;
	uint32_t bit17 : 1;
	uint32_t bit18 : 1;
	uint32_t bit19 : 1;
	uint32_t bit20 : 1;
	uint32_t bit21 : 1;
	uint32_t bit22 : 1;
	uint32_t bit23 : 1;
} zte_warning_t;

typedef union {
	zte_warning_t s;
	uint32_t v;
} u_zte_warning_t;

typedef enum {
	MODULE_COMMAND_SET_POWEROFF_VOLTAGE_CURRENT = 0,
	MODULE_COMMAND_QUERY_VOLTAGE_CURRENT_STATE,
	MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE,
} module_command_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_command_t cmd;
	uint8_t edp;
	uint8_t request_pf;
	module_request_callback_t request_callback;
	uint8_t response_pf;
	module_response_callback_t response_callback;
} module_command_item_t;

static char *get_power_module_cmd_des(module_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(MODULE_COMMAND_SET_POWEROFF_VOLTAGE_CURRENT);
			add_des_case(MODULE_COMMAND_QUERY_VOLTAGE_CURRENT_STATE);
			add_des_case(MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE);

		default: {
		}
		break;
	}

	return des;
}

static int request_set_poweroff_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint32_t poweroff = power_modules_info->power_module_info[module_id].poweroff;
	uint32_t voltage = power_modules_info->power_module_info[module_id].setting_voltage;
	uint32_t current = power_modules_info->power_module_info[module_id].setting_current;

	if(poweroff == 1) {
		poweroff = POWER_ON_OFF_OFF;
	} else {
		poweroff = POWER_ON_OFF_ON;
	}

	data[0] = poweroff;
	data[1] = 0;
	data[2] = get_u8_l_from_u16(voltage);
	data[3] = get_u8_h_from_u16(voltage);
	data[4] = get_u8_l_from_u16(current);
	data[5] = get_u8_h_from_u16(current);
	data[6] = 0;//输出过压值可以不设置,不设置的话为软件默认过压值(500V 模块为 520V,750V 模块为 770V);过压值必须比设定电压值大 20V 以上,否则无效;
	data[7] = 0;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF_VOLTAGE_CURRENT].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_set_poweroff_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	uint8_t poweroff = (data[0] == 0xaa) ? 0 : 1;
	power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff = poweroff;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_VOLTAGE_CURRENT_STATE].state = COMMAND_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF_VOLTAGE_CURRENT].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_poewroff_voltage_current = {
	.cmd = MODULE_COMMAND_SET_POWEROFF_VOLTAGE_CURRENT,
	.request_pf = ZTE_PF_MODULE_ON_OFF_REQUEST,
	.edp = 0x08,
	.request_callback = request_set_poweroff_voltage_current,
	.response_pf = ZTE_PF_MODULE_ON_OFF_RESPONSE,
	.response_callback = response_set_poweroff_voltage_current,
};

static void _set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF_VOLTAGE_CURRENT].state = COMMAND_STATE_REQUEST;
}

static void _set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF_VOLTAGE_CURRENT].state = COMMAND_STATE_REQUEST;
}

static int request_query_voltage_current_state(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	data[0] = ZTE_MODULE_QUERY_FN_STATE;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_VOLTAGE_CURRENT_STATE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_query_voltage_current_state(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	u_module_state_t *u_module_state = (u_module_state_t *)&data[0];
	uint8_t poweroff = (u_module_state->s.poweron == 0) ? 1 : 0;
	u_zte_warning_t u_zte_warning;

	power_modules_info->power_module_info[module_id].power_module_status.output_state = u_module_state->s.currentlimit;

	power_modules_info->power_module_info[module_id].output_voltage = get_u16_from_u8_lh(data[1], data[2]);

	power_modules_info->power_module_info[module_id].output_current = get_u16_from_u8_lh(data[3], data[4]) / 10;

	u_zte_warning.v = get_u32_from_u8_b0123(data[5], data[6], data[7], 0);

	power_modules_info->power_module_info[module_id].power_module_status.fault = (u_zte_warning.v == 0) ? 0 : 1;
	power_modules_info->power_module_info[module_id].power_module_status.fan_state = u_zte_warning.s.fan;
	power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage = u_zte_warning.s.input_over_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage = u_zte_warning.s.input_under_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage = u_zte_warning.s.output_over_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage = u_zte_warning.s.output_under_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent = u_zte_warning.s.protect_over_current;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature = u_zte_warning.s.protect_over_temperature;

	power_modules_info->power_module_info[module_id].power_module_status.poweroff = poweroff;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_VOLTAGE_CURRENT_STATE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_query_voltage_current_state = {
	.cmd = MODULE_COMMAND_QUERY_VOLTAGE_CURRENT_STATE,
	.request_pf = ZTE_PF_MODULE_QUERY_REQUEST,
	.edp = 0x18,
	.request_callback = request_query_voltage_current_state,
	.response_pf = ZTE_PF_MODULE_STATE_RESPONSE,
	.response_callback = response_query_voltage_current_state,
};

static void _query_status(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_VOLTAGE_CURRENT_STATE].state = COMMAND_STATE_REQUEST;
}

static int request_query_abc_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	data[0] = ZTE_MODULE_QUERY_FN_STATUS2;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_query_abc_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

        power_modules_info->power_module_info[module_id].input_aline_voltage = get_u16_from_u8_lh(data[1], data[2]) * LINE_TO_PHASE_COEFFICIENT;
        power_modules_info->power_module_info[module_id].input_bline_voltage = get_u16_from_u8_lh(data[3], data[4]) * LINE_TO_PHASE_COEFFICIENT;
        power_modules_info->power_module_info[module_id].input_cline_voltage = get_u16_from_u8_lh(data[5], data[6]) * LINE_TO_PHASE_COEFFICIENT;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_query_abc_line_input_voltage = {
	.cmd = MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE,
	.request_pf = ZTE_PF_MODULE_QUERY_REQUEST,
	.edp = 0x18,
	.request_callback = request_query_abc_line_input_voltage,
	.response_pf = ZTE_PF_MODULE_STATUS2,
	.response_callback = response_query_abc_line_input_voltage,
};

static void _query_abc_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE].state = COMMAND_STATE_REQUEST;
}

static module_command_item_t *module_command_item_table[] = {
	&module_command_item_set_poewroff_voltage_current,
	&module_command_item_query_voltage_current_state,
	&module_command_item_query_abc_line_input_voltage,
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
			u_zte_ext_id_t u_zte_ext_id;
			uint32_t ticks = osKernelSysTick();

			power_modules_request_periodic(power_modules_info);

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_zte_ext_id.v = 0;
			u_zte_ext_id.s.src = LOCAL_ADDR;
			u_zte_ext_id.s.dst = module_id + 1;
			u_zte_ext_id.s.pf = item->request_pf;
			u_zte_ext_id.s.u_p_edp_dp.v = item->edp;

			power_modules_info->can_tx_msg.ExtId = u_zte_ext_id.v;
			power_modules_info->can_tx_msg.RTR = CAN_RTR_DATA;
			power_modules_info->can_tx_msg.IDE = CAN_ID_EXT;
			power_modules_info->can_tx_msg.DLC = 8;

			memset(power_modules_info->can_tx_msg.Data, 0, 8);

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
	u_zte_ext_id_t u_zte_ext_id;
	int module_addr;
	int module_id;

	power_modules_info->can_rx_msg = can_rx_msg;

	u_zte_ext_id.v = power_modules_info->can_rx_msg->ExtId;

	module_addr = u_zte_ext_id.s.src;

	if((module_addr >= 1) && (module_addr <= power_modules_info->power_module_number)) {
		module_id = module_addr - 1;
	} else {
		return ret;
	}

	if(u_zte_ext_id.s.dst != LOCAL_ADDR) {
		return ret;
	}

	for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
		module_command_item_t *item = module_command_item_table[i];
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		connect_state_t *connect_state = &power_module_info->connect_state;

		if(u_zte_ext_id.s.pf != item->response_pf) {
			continue;
		}

		//if(u_zte_ext_id.s.u_p_edp_dp.v != item->response_pf) {
		//	continue;
		//}

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

power_modules_handler_t power_modules_handler_zte = {
	.power_module_type = POWER_MODULE_TYPE_ZTE,
	.cmd_size = ARRAY_SIZE(module_command_item_table),
	.set_out_voltage_current = _set_out_voltage_current,
	.set_poweroff = _set_poweroff,
	.query_status = _query_status,
	.query_a_line_input_voltage = _query_abc_line_input_voltage,
	.query_b_line_input_voltage = NULL,
	.query_c_line_input_voltage = NULL,
	.power_modules_request = _power_modules_request,
	.power_modules_response = _power_modules_response,
};
