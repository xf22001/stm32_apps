

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_increase.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 17时36分29秒
 *   修改日期：2020年07月17日 星期五 10时33分30秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_increase.h"
#include "os_utils.h"
#include <string.h>

#include "log.h"

#define  POWER_ID_TX_INCREASE 0x1307c080
#define  POWER_ID_RX_INCREASE 0x1207c080

#define  POWER_ID_TX_WINLINE 0x6080F80
#define  POWER_ID_RX_WINLINE 0x60F8008

typedef struct {
	uint32_t module_addr : 7;
	uint32_t id : 25;
} s_module_extid_t;

typedef union {
	s_module_extid_t s;
	uint32_t v;
} u_module_extid_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t fn;
} module_cmd_t;

typedef struct {
	uint8_t cmd;
	uint8_t current_b2;
	uint8_t current_b1;//mv
	uint8_t current_b0;
	uint8_t voltage_b3;//ma
	uint8_t voltage_b2;
	uint8_t voltage_b1;
	uint8_t voltage_b0;
} cmd_0_request_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[7];
} cmd_0_response_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[7];
} cmd_1_request_t;

typedef struct {
	uint16_t poweroff : 1;//1:模块关机 0:模块运行
	uint16_t fault : 1;//1:模块故障 0:模块正常
	uint16_t output_state : 1;//1:模块限流 0:模块恒压
	uint16_t fan_state : 1;//1:风扇故障 0:风扇正常
	uint16_t input_overvoltage : 1;//1:输入过压 0:输入正常
	uint16_t input_lowvoltage : 1;//1:输入欠压 0:输入正常
	uint16_t output_overvoltage : 1;//1:输出过压 0:输出正常
	uint16_t output_lowvoltage : 1;//1:输出欠压 0:输出正常
} status_0_t;

typedef struct {
	uint16_t protect_overcurrent : 1;//1:过流保护 0:正常
	uint16_t protect_overtemperature : 1;//1:过温保护 0:正常
	uint16_t setting_poweroff : 1;//1:设置关机 0:设置开机
} status_1_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused;
	uint8_t current_b1;//a * 10
	uint8_t current_b0;
	uint8_t voltage_b1;//v * 10
	uint8_t voltage_b0;
	status_1_t status_1;
	status_0_t status_0;
} cmd_1_response_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[6];
	uint8_t poweron;
} cmd_2_request_t;

typedef struct {
	uint8_t cmd;
	uint8_t result;//00-失败
	uint8_t unused[6];
} cmd_2_response_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t fn;
	uint8_t unused1[4];
} cmd_0x10_0x0c_request_t;

typedef struct {
	uint8_t cmd;
	uint8_t fault;//0xf0:正常
	uint8_t unused[1];
	uint8_t fn;
	uint8_t voltage_b3;//v * 10
	uint8_t voltage_b2;
	uint8_t voltage_b1;
	uint8_t voltage_b0;
} cmd_0x10_0x0c_response_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t fn;
	uint8_t unused1[4];
} cmd_0x10_0x0d_request_t;

typedef struct {
	uint8_t cmd;
	uint8_t fault;//0xf0:正常
	uint8_t unused[1];
	uint8_t fn;
	uint8_t voltage_b3;//v * 10
	uint8_t voltage_b2;
	uint8_t voltage_b1;
	uint8_t voltage_b0;
} cmd_0x10_0x0d_response_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t fn;
	uint8_t unused1[4];
} cmd_0x10_0x0e_request_t;

typedef struct {
	uint8_t cmd;
	uint8_t fault;//0xf0:正常
	uint8_t unused[1];
	uint8_t fn;
	uint8_t voltage_b3;//v * 10
	uint8_t voltage_b2;
	uint8_t voltage_b1;
	uint8_t voltage_b0;
} cmd_0x10_0x0e_response_t;

typedef enum {
	MODULE_CMD_0_0 = 0,
	MODULE_CMD_1_1,
	MODULE_CMD_2_2,
	MODULE_CMD_0x10_0x0c,
	MODULE_CMD_0x10_0x0d,
	MODULE_CMD_0x10_0x0e,
	MODULE_CMD_TOTAL,
} module_command_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_command_t cmd;
	uint8_t have_module_fn;
	uint32_t request_ext_id;
	uint8_t request_code;
	uint8_t request_fn;
	module_request_callback_t request_callback;
	uint32_t response_ext_id;
	uint8_t response_code;
	uint8_t response_fn;
	module_response_callback_t response_callback;
} module_command_item_t;

static char *get_power_module_cmd_des(module_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(MODULE_CMD_0_0);
			add_des_case(MODULE_CMD_1_1);
			add_des_case(MODULE_CMD_2_2);
			add_des_case(MODULE_CMD_0x10_0x0c);
			add_des_case(MODULE_CMD_0x10_0x0d);
			add_des_case(MODULE_CMD_0x10_0x0e);

		default: {
		}
		break;
	}

	return des;
}

static void set_out_voltage_current_increase(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint32_t current)
{
	power_modules_info->power_module_info[module_id].setting_current = current;
	power_modules_info->power_module_info[module_id].setting_voltage = voltage;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0_0].state = CAN_COM_STATE_REQUEST;
}

static int request_0(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0_request_t *cmd_0_request = (cmd_0_request_t *)power_modules_info->can_tx_msg.Data;

	cmd_0_request->current_b0 = get_u8_b0_from_u32(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0_request->current_b1 = get_u8_b1_from_u32(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0_request->current_b2 = get_u8_b2_from_u32(power_modules_info->power_module_info[module_id].setting_current);

	cmd_0_request->voltage_b0 = get_u8_b0_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0_request->voltage_b1 = get_u8_b1_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0_request->voltage_b2 = get_u8_b2_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0_request->voltage_b3 = get_u8_b3_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0_0].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0_0].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0_0 = {
	.cmd = MODULE_CMD_0_0,
	.have_module_fn = 0,
	.request_ext_id = POWER_ID_TX_INCREASE,
	.request_code = 0x00,
	.request_callback = request_0,
	.response_ext_id = POWER_ID_RX_INCREASE,
	.response_code = 0x00,
	.response_callback = response_0,
};

static void set_poweroff_increase(power_modules_info_t *power_modules_info, int module_id, uint8_t poweroff)
{
	power_modules_info->power_module_info[module_id].poweroff = poweroff;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_2_2].state = CAN_COM_STATE_REQUEST;
}

static int request_2(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_2_request_t *cmd_2_requeset = (cmd_2_request_t *)power_modules_info->can_tx_msg.Data;

	cmd_2_requeset->poweron = (power_modules_info->power_module_info[module_id].poweroff == 0) ? 1 : 0;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_2_2].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_2(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_2_2].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_2_2 = {
	.cmd = MODULE_CMD_2_2,
	.have_module_fn = 0,
	.request_ext_id = POWER_ID_TX_INCREASE,
	.request_code = 0x02,
	.request_callback = request_2,
	.response_ext_id = POWER_ID_RX_INCREASE,
	.response_code = 0x02,
	.response_callback = response_2,
};

static void query_status_increase(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_1_1].state = CAN_COM_STATE_REQUEST;
}

static int request_1(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_1_request_t *cmd_1_requeset = (cmd_1_request_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_1_1].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_1(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_1_response_t *cmd_1_response = (cmd_1_response_t *)power_modules_info->can_rx_msg->Data;

	power_modules_info->power_module_info[module_id].output_voltage = get_u16_from_u8_lh(cmd_1_response->voltage_b0, cmd_1_response->voltage_b1);
	power_modules_info->power_module_info[module_id].output_current = get_u16_from_u8_lh(cmd_1_response->current_b0, cmd_1_response->current_b1);
	power_modules_info->power_module_info[module_id].power_module_status.poweroff = cmd_1_response->status_0.poweroff;
	power_modules_info->power_module_info[module_id].power_module_status.fault = cmd_1_response->status_0.fault;
	power_modules_info->power_module_info[module_id].power_module_status.output_state = cmd_1_response->status_0.output_state;
	power_modules_info->power_module_info[module_id].power_module_status.fan_state = cmd_1_response->status_0.fan_state;
	power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage = cmd_1_response->status_0.input_overvoltage;
	power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage = cmd_1_response->status_0.input_lowvoltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage = cmd_1_response->status_0.output_overvoltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage = cmd_1_response->status_0.output_lowvoltage;

	power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent = cmd_1_response->status_1.protect_overcurrent;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature = cmd_1_response->status_1.protect_overtemperature;
	power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff = cmd_1_response->status_1.setting_poweroff;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_1_1].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_1_1 = {
	.cmd = MODULE_CMD_1_1,
	.have_module_fn = 0,
	.request_ext_id = POWER_ID_TX_INCREASE,
	.request_code = 0x01,
	.request_callback = request_1,
	.response_ext_id = POWER_ID_RX_INCREASE,
	.response_code = 0x01,
	.response_callback = response_1,
};

static void query_a_line_input_voltage_increase(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x10_0x0c].state = CAN_COM_STATE_REQUEST;
}

static int request_0x10_0x0c(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x10_0x0c_request_t *cmd_0x10_0x0c_requeset = (cmd_0x10_0x0c_request_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x10_0x0c].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x10_0x0c(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x10_0x0c_response_t *cmd_0x10_0x0c_response = (cmd_0x10_0x0c_response_t *)power_modules_info->can_rx_msg->Data;
	uint32_t input_aline_voltage = 0;
	power_module_info_t *power_module_info;
	int i;

	if(cmd_0x10_0x0c_response->fault == 0xf0) {
		input_aline_voltage = 10 * get_u32_from_u8_b0123(
		                          cmd_0x10_0x0c_response->voltage_b0,
		                          cmd_0x10_0x0c_response->voltage_b1,
		                          cmd_0x10_0x0c_response->voltage_b2,
		                          cmd_0x10_0x0c_response->voltage_b3);
	}

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info = power_modules_info->power_module_info + i;
		power_module_info->input_aline_voltage = input_aline_voltage;
	}

	power_module_info = power_modules_info->power_module_info + module_id;
	power_module_info->cmd_ctx[MODULE_CMD_0x10_0x0c].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x10_0x0c = {
	.cmd = MODULE_CMD_0x10_0x0c,
	.have_module_fn = 1,
	.request_fn = 0x0c,
	.request_ext_id = POWER_ID_TX_WINLINE,
	.request_code = 0x10,
	.request_callback = request_0x10_0x0c,
	.response_ext_id = POWER_ID_RX_WINLINE,
	.response_code = 0x10,
	.response_fn = 0x0c,
	.response_callback = response_0x10_0x0c,
};

static void query_b_line_input_voltage_increase(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x10_0x0d].state = CAN_COM_STATE_REQUEST;
}

static int request_0x10_0x0d(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x10_0x0d_request_t *cmd_0x10_0x0d_requeset = (cmd_0x10_0x0d_request_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x10_0x0d].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x10_0x0d(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x10_0x0d_response_t *cmd_0x10_0x0d_response = (cmd_0x10_0x0d_response_t *)power_modules_info->can_rx_msg->Data;
	uint32_t input_bline_voltage = 0;
	power_module_info_t *power_module_info;
	int i;

	if(cmd_0x10_0x0d_response->fault == 0xf0) {
		input_bline_voltage = 10 * get_u32_from_u8_b0123(
		                          cmd_0x10_0x0d_response->voltage_b0,
		                          cmd_0x10_0x0d_response->voltage_b1,
		                          cmd_0x10_0x0d_response->voltage_b2,
		                          cmd_0x10_0x0d_response->voltage_b3);
	}

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info = power_modules_info->power_module_info + i;
		power_module_info->input_bline_voltage = input_bline_voltage;
	}

	power_module_info = power_modules_info->power_module_info + module_id;
	power_module_info->cmd_ctx[MODULE_CMD_0x10_0x0d].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x10_0x0d = {
	.cmd = MODULE_CMD_0x10_0x0d,
	.have_module_fn = 1,
	.request_fn = 0x0d,
	.request_ext_id = POWER_ID_TX_WINLINE,
	.request_code = 0x10,
	.request_callback = request_0x10_0x0d,
	.response_ext_id = POWER_ID_RX_WINLINE,
	.response_code = 0x10,
	.response_fn = 0x0d,
	.response_callback = response_0x10_0x0d,
};

static void query_c_line_input_voltage_increase(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x10_0x0e].state = CAN_COM_STATE_REQUEST;
}

static int request_0x10_0x0e(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	//cmd_0x10_0x0e_request_t *cmd_0x10_0x0e_requeset = (cmd_0x10_0x0e_request_t *)power_modules_info->can_tx_msg.Data;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_CMD_0x10_0x0e].state = CAN_COM_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0x10_0x0e(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0x10_0x0e_response_t *cmd_0x10_0x0e_response = (cmd_0x10_0x0e_response_t *)power_modules_info->can_rx_msg->Data;
	uint32_t input_cline_voltage = 0;
	power_module_info_t *power_module_info;
	int i;

	if(cmd_0x10_0x0e_response->fault == 0xf0) {
		input_cline_voltage = 10 * get_u32_from_u8_b0123(
		                          cmd_0x10_0x0e_response->voltage_b0,
		                          cmd_0x10_0x0e_response->voltage_b1,
		                          cmd_0x10_0x0e_response->voltage_b2,
		                          cmd_0x10_0x0e_response->voltage_b3);
	}

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info = power_modules_info->power_module_info + i;
		power_module_info->input_cline_voltage = input_cline_voltage;
	}

	power_module_info = power_modules_info->power_module_info + module_id;
	power_module_info->cmd_ctx[MODULE_CMD_0x10_0x0e].state = CAN_COM_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x10_0x0e = {
	.cmd = MODULE_CMD_0x10_0x0e,
	.have_module_fn = 1,
	.request_fn = 0x0e,
	.request_ext_id = POWER_ID_TX_WINLINE,
	.request_code = 0x10,
	.request_callback = request_0x10_0x0e,
	.response_ext_id = POWER_ID_RX_WINLINE,
	.response_code = 0x10,
	.response_fn = 0x0e,
	.response_callback = response_0x10_0x0e,
};

static module_command_item_t *module_command_item_table[] = {
	&module_command_item_0_0,
	&module_command_item_2_2,
	&module_command_item_1_1,
	&module_command_item_0x10_0x0c,
	&module_command_item_0x10_0x0d,
	&module_command_item_0x10_0x0e,
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

static void power_modules_request_increase(power_modules_info_t *power_modules_info)
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
			module_cmd_t *module_cmd = (module_cmd_t *)power_modules_info->can_tx_msg.Data;
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

			module_cmd->cmd = item->request_code;

			if(item->have_module_fn == 1) {
				module_cmd->fn = item->request_fn;
			}

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

static int power_modules_response_increase(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int i;
	u_module_extid_t u_module_extid;
	int module_addr;
	int module_id;
	uint32_t response_ext_id;
	module_cmd_t *module_cmd;
	uint8_t response_code;

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

	module_cmd = (module_cmd_t *)power_modules_info->can_rx_msg->Data;
	response_code = module_cmd->cmd;

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

		if(item->have_module_fn == 1) {
			if(module_cmd->fn != item->response_fn) {
				continue;
			}
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

power_modules_handler_t power_modules_handler_increase = {
	.power_module_type = POWER_MODULE_TYPE_INCREASE,
	.cmd_size = ARRAY_SIZE(module_command_item_table),
	.set_out_voltage_current = set_out_voltage_current_increase,
	.set_poweroff = set_poweroff_increase,
	.query_status = query_status_increase,
	.query_a_line_input_voltage = query_a_line_input_voltage_increase,
	.query_b_line_input_voltage =  query_b_line_input_voltage_increase,
	.query_c_line_input_voltage = query_c_line_input_voltage_increase,
	.power_modules_request = power_modules_request_increase,
	.power_modules_response = power_modules_response_increase,
};
