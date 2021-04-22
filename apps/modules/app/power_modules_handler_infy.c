

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_infy.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月15日 星期四 10时25分30秒
 *   修改日期：2021年04月22日 星期四 11时25分51秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_infy.h"

#include <string.h>

#include "os_utils.h"
#define LOG_NONE
#include "log.h"

#define LOCAL_ADDR 0xf0
#define DEV_SINGLE 0x0a
#define DEV_GROUP 0x0b

typedef enum {
	INFY_NORMAL = 0x00,
	INFY_CMD_ERR = 0x02,
	INFY_DATA_ERR = 0x03,
	INFY_ADDR_ERR = 0x04,
	INFY_STARTING = 0x07,
} infy_err_t;

typedef struct {
	uint32_t src : 8;
	uint32_t dst : 8;
	uint32_t cmd : 6;
	uint32_t dev : 4;
	uint32_t err : 3;
} infy_ext_id_t;

typedef union {
	infy_ext_id_t s;
	uint32_t v;
} u_infy_ext_id_t;

typedef enum {
	INFY_CMD_GET_STATUS = 0x04,
	INFY_CMD_GET_ABC_LINE_INPUT_VOLTAGE = 0x06,
	INFY_CMD_GET_VOLTAGE_CURRENT = 0x09,
	INFY_CMD_SET_POWEROFF = 0x1a,
	INFY_CMD_SET_VOLTAGE_CURRENT = 0x1c,
	INFY_CMD_SET_INIT_ADDR = 0x1f,
	INFY_CMD_SET_INIT_GROUP = 0x16,
} infy_cmd_t;

typedef enum {
	MODULE_COMMAND_SET_OUT_VOLTAGE_CURRENT = 0,
	MODULE_COMMAND_SET_POWEROFF,
	MODULE_COMMAND_QUERY_STATUS,
	MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE,
	MODULE_COMMAND_GET_OUT_VOLTAGE_CURRENT,
} module_command_t;

typedef struct {
	uint8_t unused;

	uint8_t power_limit : 1;//1:模块处于限功率状态
	uint8_t id_overlap : 1;//1:模块 ID 重复
	uint8_t current_uneven : 1;//1:模块严重不均流
	uint8_t phase_loss : 1;//1:三相输入缺相告警
	uint8_t phase_uneven : 1;//1:三相输入缺相告警
	uint8_t input_under_voltage : 1;//1:输入欠压告警
	uint8_t input_over_voltage : 1;//1:输入过压告警
	uint8_t fpc_poweroff : 1;//1:模块 PFC 侧处于关机状态

	uint8_t dc_poweroff : 1;//1:模块 DC 侧处于关机状态
	uint8_t fault : 1;//1:模块故障告警
	uint8_t protect : 1;//1:模块保护告警
	uint8_t fan : 1;//1:风扇故障告警
	uint8_t over_temperature : 1;//1:过温告警
	uint8_t output_over_voltage : 1;//1:输出过压告警
	uint8_t walk_in : 1;//1:WALK-IN 使能
	uint8_t interrupt : 1;//1:模块通信中断告警

	uint8_t short_circuit : 1;//1:输出短路
	uint8_t unused2 : 3;
	uint8_t sleep : 1;//1:模块休眠
	uint8_t discharge : 1;//1:模块放电异常
} infy_module_status_t;

typedef union {
	infy_module_status_t s;
	uint32_t v;
} u_infy_module_status_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_command_t cmd;
	infy_cmd_t infy_cmd;
	module_request_callback_t request_callback;
	module_response_callback_t response_callback;
} module_command_item_t;

static char *get_power_module_cmd_des(module_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(MODULE_COMMAND_SET_OUT_VOLTAGE_CURRENT);
			add_des_case(MODULE_COMMAND_SET_POWEROFF);
			add_des_case(MODULE_COMMAND_QUERY_STATUS);
			add_des_case(MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE);
			add_des_case(MODULE_COMMAND_GET_OUT_VOLTAGE_CURRENT);

		default: {
		}
		break;
	}

	return des;
}

static int request_get_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_OUT_VOLTAGE_CURRENT].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_get_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].output_voltage = get_u32_from_u8_b0123(data[3], data[2], data[1], data[0]) / 100;
	power_modules_info->power_module_info[module_id].output_current = get_u32_from_u8_b0123(data[7], data[6], data[5], data[4]) / 100;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_OUT_VOLTAGE_CURRENT].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_get_voltage_current = {
	.cmd = MODULE_COMMAND_GET_OUT_VOLTAGE_CURRENT,
	.infy_cmd = INFY_CMD_GET_VOLTAGE_CURRENT,
	.request_callback = request_get_voltage_current,
	.response_callback = response_get_voltage_current,
};

static void _set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_OUT_VOLTAGE_CURRENT].state = COMMAND_STATE_REQUEST;
}

static int request_set_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint32_t voltage = power_modules_info->power_module_info[module_id].setting_voltage;
	uint32_t current = power_modules_info->power_module_info[module_id].setting_current;

	data[0] = get_u8_b3_from_u32(voltage);
	data[1] = get_u8_b2_from_u32(voltage);
	data[2] = get_u8_b1_from_u32(voltage);
	data[3] = get_u8_b0_from_u32(voltage);

	data[4] = get_u8_b3_from_u32(current);
	data[5] = get_u8_b2_from_u32(current);
	data[6] = get_u8_b1_from_u32(current);
	data[7] = get_u8_b0_from_u32(current);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_OUT_VOLTAGE_CURRENT].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_set_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_OUT_VOLTAGE_CURRENT].state = COMMAND_STATE_IDLE;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_GET_OUT_VOLTAGE_CURRENT].state = COMMAND_STATE_REQUEST;
	return ret;
}

static module_command_item_t module_command_item_set_voltage_current = {
	.cmd = MODULE_COMMAND_SET_OUT_VOLTAGE_CURRENT,
	.infy_cmd = INFY_CMD_SET_VOLTAGE_CURRENT,
	.request_callback = request_set_voltage_current,
	.response_callback = response_set_voltage_current,
};

static void _set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_REQUEST;
}

static int request_set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint8_t poweroff = power_modules_info->power_module_info[module_id].poweroff;

	data[0] = poweroff;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff = data[0];

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_SET_POWEROFF].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_set_poweroff = {
	.cmd = MODULE_COMMAND_SET_POWEROFF,
	.infy_cmd = INFY_CMD_SET_VOLTAGE_CURRENT,
	.request_callback = request_set_poweroff,
	.response_callback = response_set_poweroff,
};

static void _query_status(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_STATUS].state = COMMAND_STATE_REQUEST;
}

static int request_query_status(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_STATUS].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_query_status(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	u_infy_module_status_t u_infy_module_status;

	u_infy_module_status.v = get_u32_from_u8_b0123(data[4], data[5], data[6], data[7]);

	power_modules_info->power_module_info[module_id].power_module_status.poweroff = (u_infy_module_status.s.sleep == 1 ||
	        u_infy_module_status.s.dc_poweroff == 1 ||
	        u_infy_module_status.s.fpc_poweroff == 1) ? 1 : 0;
	power_modules_info->power_module_info[module_id].power_module_status.fault = u_infy_module_status.s.fault;
	power_modules_info->power_module_info[module_id].power_module_status.output_state = u_infy_module_status.s.power_limit;
	power_modules_info->power_module_info[module_id].power_module_status.fan_state = u_infy_module_status.s.fan;
	power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage = u_infy_module_status.s.input_over_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage = u_infy_module_status.s.input_under_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage = u_infy_module_status.s.output_over_voltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage = 0;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent = u_infy_module_status.s.protect;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature = u_infy_module_status.s.over_temperature;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_STATUS].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_query_status = {
	.cmd = MODULE_COMMAND_QUERY_STATUS,
	.infy_cmd = INFY_CMD_GET_STATUS,
	.request_callback = request_query_status,
	.response_callback = response_query_status,
};

static void query_abc_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE].state = COMMAND_STATE_REQUEST;
}

static int request_query_abc_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_query_abc_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].input_aline_voltage = get_u16_from_u8_lh(data[1], data[0]) * LINE_TO_PHASE_COEFFICIENT;
	power_modules_info->power_module_info[module_id].input_bline_voltage = get_u16_from_u8_lh(data[3], data[2]) * LINE_TO_PHASE_COEFFICIENT;
	power_modules_info->power_module_info[module_id].input_cline_voltage = get_u16_from_u8_lh(data[5], data[4]) * LINE_TO_PHASE_COEFFICIENT;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_query_abc_line_input_voltage = {
	.cmd = MODULE_COMMAND_QUERY_ABC_LINE_INPUT_VOLTAGE,
	.infy_cmd = INFY_CMD_GET_ABC_LINE_INPUT_VOLTAGE,
	.request_callback = request_query_abc_line_input_voltage,
	.response_callback = response_query_abc_line_input_voltage,
};

static module_command_item_t *module_command_item_table[] = {
	&module_command_item_set_voltage_current,
	&module_command_item_set_poweroff,
	&module_command_item_query_status,
	&module_command_item_query_abc_line_input_voltage,
	&module_command_item_get_voltage_current,
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
			u_infy_ext_id_t u_infy_ext_id;
			uint32_t ticks = osKernelSysTick();

			power_modules_request_periodic(power_modules_info);

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_infy_ext_id.v = 0;
			u_infy_ext_id.s.src = LOCAL_ADDR;
			u_infy_ext_id.s.dst = module_id + 1;
			u_infy_ext_id.s.cmd = item->infy_cmd;
			u_infy_ext_id.s.dev = DEV_SINGLE;
			u_infy_ext_id.s.err = 0;

			power_modules_info->can_tx_msg.ExtId = u_infy_ext_id.v;
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
	u_infy_ext_id_t u_infy_ext_id;
	int module_addr;
	int module_id;
	uint8_t infy_cmd;

	power_modules_info->can_rx_msg = can_rx_msg;

	u_infy_ext_id.v = power_modules_info->can_rx_msg->ExtId;

	module_addr = u_infy_ext_id.s.src;

	if((module_addr >= 1) && (module_addr <= power_modules_info->power_module_number)) {
		module_id = module_addr - 1;
	} else {
		return ret;
	}

	if(u_infy_ext_id.s.dst != LOCAL_ADDR) {
		return ret;
	}

	if(u_infy_ext_id.s.dev != DEV_SINGLE) {
		return ret;
	}

	if(u_infy_ext_id.s.err != 0) {
		return ret;
	}

	infy_cmd = u_infy_ext_id.s.cmd;

	for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
		module_command_item_t *item = module_command_item_table[i];
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		connect_state_t *connect_state = &power_module_info->connect_state;

		if(infy_cmd != item->infy_cmd) {
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

power_modules_handler_t power_modules_handler_infy = {
	.power_module_type = POWER_MODULE_TYPE_INFY,
	.cmd_size = ARRAY_SIZE(module_command_item_table),
	.set_out_voltage_current = _set_out_voltage_current,
	.set_poweroff = _set_poweroff,
	.query_status = _query_status,
	.query_a_line_input_voltage = query_abc_line_input_voltage,
	.query_b_line_input_voltage = NULL,
	.query_c_line_input_voltage = NULL,
	.power_modules_request = _power_modules_request,
	.power_modules_response = _power_modules_response,
};
