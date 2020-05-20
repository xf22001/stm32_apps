

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_increase.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 17时36分29秒
 *   修改日期：2020年05月20日 星期三 15时41分20秒
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
} s_module_cmd_t;

typedef union {
	s_module_cmd_t s;
	uint16_t v;
} u_module_cmd_t;
typedef struct {
	uint8_t cmd;
	uint8_t unused;
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
	uint8_t cmd;
	uint8_t unused;
	uint8_t current_b1;//a * 10
	uint8_t current_b0;
	uint8_t voltage_b3;//v * 10
	uint8_t voltage_b2;
	uint8_t voltage_b1;
	uint8_t voltage_b0;
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
	uint8_t line;
	uint8_t unused1[4];
} cmd_0x10_0x0c_request_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t line;
	uint8_t voltage_b3;//v * 10
	uint8_t voltage_b2;
	uint8_t voltage_b1;
	uint8_t voltage_b0;
} cmd_0x10_0x0c_response_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t line;
	uint8_t unused1[4];
} cmd_0x10_0x0d_request_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t line;
	uint8_t voltage_b3;//v * 10
	uint8_t voltage_b2;
	uint8_t voltage_b1;
	uint8_t voltage_b0;
} cmd_0x10_0x0d_response_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t line;
	uint8_t unused1[4];
} cmd_0x10_0x0e_request_t;

typedef struct {
	uint8_t cmd;
	uint8_t unused[2];
	uint8_t line;
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
} module_cmd_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_cmd_t cmd;
	uint32_t request_ext_id;
	uint8_t request_code;
	module_request_callback_t request_callback;
	uint32_t response_ext_id;
	uint8_t response_code;
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

void set_out_voltage_current_increase(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint16_t current)
{
	power_modules_info->power_module_info[module_id].setting_current = current;
	power_modules_info->power_module_info[module_id].setting_voltage = voltage;
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0_0].state = MODULE_CMD_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0_0].retry = 0;
}

static int request_0(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_0_request_t *cmd_0_request = (cmd_0_request_t *)power_modules_info->can_tx_msg.Data;

	cmd_0_request->current_b0 = get_u8_l_from_u16(power_modules_info->power_module_info[module_id].setting_current);
	cmd_0_request->current_b1 = get_u8_h_from_u16(power_modules_info->power_module_info[module_id].setting_current);

	cmd_0_request->voltage_b0 = get_u8_b0_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0_request->voltage_b1 = get_u8_b1_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0_request->voltage_b2 = get_u8_b2_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);
	cmd_0_request->voltage_b3 = get_u8_b3_from_u32(power_modules_info->power_module_info[module_id].setting_voltage);

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0_0].state = MODULE_CMD_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_0(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0_0].state = MODULE_CMD_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_0x103_0x103 = {
	.cmd = MODULE_CMD_0_0,
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
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_2_2].state = MODULE_CMD_STATE_REQUEST;
	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_2_2].retry = 0;
}

static int request_2(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	cmd_2_request_t *cmd_2_requeset = (cmd_2_request_t *)power_modules_info->can_tx_msg.Data;

	cmd_2_requeset->poweron = (power_modules_info->power_module_info[module_id].poweroff == 0) ? 1 : 0;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_2_2].state = MODULE_CMD_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_2(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;

	power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_2_2].state = MODULE_CMD_STATE_IDLE;
	ret = 0;
	return ret;
}

static module_command_item_t module_command_item_2_2 = {
	.cmd = MODULE_CMD_2_2,
	.request_ext_id = POWER_ID_TX_INCREASE,
	.request_code = 0x02,
	.request_callback = request_2,
	.response_ext_id = POWER_ID_RX_INCREASE,
	.response_code = 0x02,
	.response_callback = response_2,
};

static void query_status_increase(power_modules_info_t *power_modules_info, int module_id)
{
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x191_0x191].state = MODULE_CMD_STATE_REQUEST;
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x191_0x191].retry = 0;
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x183_0x183].state = MODULE_CMD_STATE_REQUEST;
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x183_0x183].retry = 0;
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x190_0x190].state = MODULE_CMD_STATE_REQUEST;
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x190_0x190].retry = 0;
}

static void query_a_line_input_voltage_increase(power_modules_info_t *power_modules_info, int module_id)
{
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x179_0x179].state = MODULE_CMD_STATE_REQUEST;
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x179_0x179].retry = 0;
}

static void query_b_line_input_voltage_increase(power_modules_info_t *power_modules_info, int module_id)
{
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x17a_0x17a].state = MODULE_CMD_STATE_REQUEST;
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x17a_0x17a].retry = 0;
}

static void query_c_line_input_voltage_increase(power_modules_info_t *power_modules_info, int module_id)
{
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x17b_0x17b].state = MODULE_CMD_STATE_REQUEST;
	//power_modules_info->power_module_info[module_id].module_cmd_ctx[MODULE_CMD_0x17b_0x17b].retry = 0;
}

static module_command_item_t *module_command_item_table[] = {
};

static void power_modules_request_increase(power_modules_info_t *power_modules_info)
{
	int module_id;
	int cmd;
	int ret;

	for(module_id = 0; module_id < POWER_MODULES_SIZE; module_id++) {
		for(cmd = 0; cmd < sizeof(module_command_item_table) / sizeof(module_command_item_t *); cmd++) {
			module_command_item_t *item = module_command_item_table[cmd];
			int module_addr = module_id + 1;

			if(power_modules_info->power_module_info[module_id].module_cmd_ctx[cmd].state == MODULE_CMD_STATE_REQUEST) {
				u_module_cmd_t *u_module_cmd = (u_module_cmd_t *)power_modules_info->can_tx_msg.Data;
				u_module_extid_t u_module_extid;

				u_module_extid.v = item->request_ext_id;
				u_module_extid.s.module_addr = module_addr;

				power_modules_info->can_tx_msg.ExtId = u_module_extid.v;
				power_modules_info->can_tx_msg.DLC = 8;

				memset(power_modules_info->can_tx_msg.Data, 0, 8);

				u_module_cmd->v = 0;
				u_module_cmd->s.cmd = item->request_code;

				ret = item->request_callback(power_modules_info, module_id);

				if(ret != 0) {
					_printf("%s:%s:%d command need implement correctly!\n", __FILE__, __func__, __LINE__);
					continue;
				}

				power_modules_info->power_module_info[module_id].module_cmd_ctx[cmd].retry++;

				ret = can_tx_data(power_modules_info->can_info, &power_modules_info->can_tx_msg, 10);

				if(ret != 0) {
					if(power_modules_info->power_module_info[module_id].module_cmd_ctx[cmd].retry <= 3) {
						power_modules_info->power_module_info[module_id].module_cmd_ctx[cmd].state = MODULE_CMD_STATE_REQUEST;
					} else {
						power_modules_info->power_module_info[module_id].module_cmd_ctx[cmd].state = MODULE_CMD_STATE_ERROR;
					}
				}
			}

		}
	}
}

static int power_modules_response_increase(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int cmd;
	u_module_extid_t u_module_extid;
	int module_addr;
	int module_id;
	uint32_t response_ext_id;
	u_module_cmd_t *u_module_cmd;
	uint8_t response_code;

	power_modules_info->can_rx_msg = can_rx_msg;

	u_module_extid.v = power_modules_info->can_rx_msg->ExtId;
	module_addr = u_module_extid.s.module_addr;

	if((module_addr <= POWER_MODULES_SIZE) && (module_addr >= 1)) {
		module_id = module_addr - 1;
	} else {
		return ret;
	}

	u_module_extid.s.module_addr = 0;
	response_ext_id = u_module_extid.v;

	u_module_cmd = (u_module_cmd_t *)power_modules_info->can_rx_msg->Data;
	response_code = u_module_cmd->s.cmd;

	for(cmd = 0; cmd < sizeof(module_command_item_table) / sizeof(module_command_item_t *); cmd++) {
		module_command_item_t *item = module_command_item_table[cmd];

		if(response_ext_id != item->response_ext_id) {
			continue;
		}

		if(response_code != item->response_code) {
			continue;
		}

		ret = item->response_callback(power_modules_info, module_id);

		if(ret != 0) {
		}

		ret = 0;
		break;
	}

	return ret;
}

power_modules_handler_t power_modules_handler_increase = {
	.power_module_type = POWER_MODULE_TYPE_INCREASE,
	.set_out_voltage_current = NULL,
	.set_poweroff =  NULL,
	.query_status =  NULL,
	.query_a_line_input_voltage = NULL,
	.query_b_line_input_voltage =  NULL,
	.query_c_line_input_voltage = NULL,
	.power_modules_init = NULL,
	.power_modules_request = NULL,
	.power_modules_response = NULL,
};
