

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_stategrid.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月16日 星期五 16时31分34秒
 *   修改日期：2021年06月03日 星期四 16时36分21秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_stategrid.h"

#include <string.h>

#include "log.h"

typedef struct {
	uint8_t sa;//源地址
	uint8_t ps;//目标地址
	uint8_t pf;//用来确定PDU的格式,以及数据域对应的参数组编号
	uint8_t dp : 1;//0
	uint8_t r : 1;//0
	uint8_t p : 1;//0-7
} stategrid_ext_id_t;

typedef union {
	stategrid_ext_id_t s;
	uint32_t v;
} u_stategrid_ext_id_t;

typedef struct {
	uint8_t pgn0;
	uint8_t pgn1;
	uint8_t pgn2;
} stategrid_pgn_t;

typedef union {
	stategrid_pgn_t s;
	uint32_t v;
} u_stategrid_pgn_t;

typedef enum {

	STATEGRID_CMD_REMOTE_CONTROL_REQUEST = 1280,//0x000500 6 8
	STATEGRID_CMD_REMOTE_CONTROL_RESPONSE = 1536,//0x000600 6 8

	STATEGRID_CMD_REMOTE_DATA_REQUEST = 8192,//0x002000 6 8
	STATEGRID_CMD_REMOTE_DATA_RESPONSE = 8192,//0x002000 6 8

	STATEGRID_CMD_HEARTBEAT_REQUEST = 16384,//0x004000 6 8
	STATEGRID_CMD_HEARTBEAT_RESPONSE = 16640,//0x041000 6 8

	STATEGRID_CMD_CONFIG_REQUEST = 32768,//0x008000 6 8//多包
	STATEGRID_CMD_CONFIG_RESPONSE = 33024,//0x008100 6 8//多包
} stategrid_cmd_t;

typedef enum {
	MODULE_COMMAND_REMOTE_CONTROL = 0,
	MODULE_COMMAND_HEARTBEAT,
	MODULE_COMMAND_REMOTE_DATA,
	MODULE_COMMAND_CONFIG,
} module_command_t;

typedef int (*module_request_callback_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*module_response_callback_t)(power_modules_info_t *power_modules_info, int module_id);

typedef struct {
	module_command_t cmd;
	stategrid_cmd_t request_code;
	uint8_t request_prio;
	stategrid_cmd_t response_code;
	uint8_t response_prio;
	module_request_callback_t request_callback;
	module_response_callback_t response_callback;
} module_command_item_t;

static char *get_power_module_cmd_des(module_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(MODULE_COMMAND_REMOTE_CONTROL);
			add_des_case(MODULE_COMMAND_HEARTBEAT);
			add_des_case(MODULE_COMMAND_REMOTE_DATA);
			add_des_case(MODULE_COMMAND_CONFIG);

		default: {
		}
		break;
	}

	return des;
}

typedef struct {
	//操作指令
	//0x01 --- 快速开机(绝缘检测阶段使用)
	//0x02 --- 停止充电(保留分组信息)
	//0x03 --- 软起开机(预启动阶段使用)
	//0x04 --- 显示地址(显示模块通信地址)
	//0x05 --- 参数修改
	//0x06 --- 停止充电(自动清除分组信息)
	uint8_t op_code : 4;

	//压输出范围选择
	//0x00 --- 低电压段(200V-500V)
	//0x01 --- 高电压段(500V-750V、 500V-950V)
	uint8_t output_mode : 1;

	//功率分配回路接触器状态
	//0x00 --- 分断
	//0x01 --- 闭合
	uint8_t power_assign_relay_state : 1;

	//充电回路主接触器状态
	//0x00 --- 分断
	//0x01 --- 闭合
	uint8_t main_relay_state : 1;

	//成功标识
	//0x00 -- 失败
	//0x01 --- 成功
	uint8_t result : 1;

	uint8_t group;//1-255
	uint8_t set_voltage_lo;//0.1v
	uint8_t set_voltage_hi;//0.1v
	uint8_t set_current_lo;//0.01a
	uint8_t set_current_hi;//0.01a
	uint8_t battery_voltage_lo;//0.1v
	uint8_t battery_voltage_hi;//0.1v
} remote_control_data_t;

static void _set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;

	if(power_module_info->power_module_status.poweroff == 0) {
		power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_REMOTE_CONTROL].state = COMMAND_STATE_REQUEST;
	}
}

static int request_remote_control(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
	uint8_t battery_voltage = power_module_info->battery_voltage / 100;
	uint32_t voltage = power_module_info->setting_voltage / 100;
	uint32_t current = power_module_info->setting_current / 10;
	remote_control_data_t *remote_control_data = (remote_control_data_t *)data;
	uint8_t do_poweroff = 0;

	OS_ASSERT(sizeof(remote_control_data_t) == 8);

	if(power_module_info->poweroff != power_module_info->power_module_status.setting_poweroff) {
		do_poweroff = 1;
	}

	if(power_module_info->poweroff != power_module_info->power_module_status.poweroff) {
		do_poweroff = 1;
	}

	if(do_poweroff == 1) {
		if(power_module_info->poweroff == 1) {
			remote_control_data->op_code = 0x06;
			remote_control_data->power_assign_relay_state = 0x00;
			remote_control_data->main_relay_state = 0x00;
		} else {
			remote_control_data->op_code = 0x03;
			remote_control_data->power_assign_relay_state = 0x01;
			remote_control_data->main_relay_state = 0x01;
		}
	} else {
		remote_control_data->op_code = 0x05;
		remote_control_data->power_assign_relay_state = 0x01;
		remote_control_data->main_relay_state = 0x01;
	}

	remote_control_data->output_mode = (voltage >= 5000) ? 0x01 : 0x00;

	remote_control_data->group = power_module_info->channel_id;
	remote_control_data->set_voltage_lo = get_u8_l_from_u16(voltage);
	remote_control_data->set_voltage_hi = get_u8_h_from_u16(voltage);
	remote_control_data->set_current_lo = get_u8_l_from_u16(current);
	remote_control_data->set_current_hi = get_u8_h_from_u16(current);
	remote_control_data->battery_voltage_lo = get_u8_l_from_u16(battery_voltage);
	remote_control_data->battery_voltage_lo = get_u8_h_from_u16(battery_voltage);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_REMOTE_CONTROL].state = COMMAND_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_remote_control(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
	remote_control_data_t *remote_control_data = (remote_control_data_t *)data;

	OS_ASSERT(sizeof(remote_control_data_t) == 8);

	if(remote_control_data->result == 0x01) {
		switch(remote_control_data->op_code) {
			case 0x02:
			case 0x06: {//关机
				power_module_info->power_module_status.setting_poweroff = 1;
			}
			break;

			case 0x03: {//开机
				power_module_info->power_module_status.setting_poweroff = 0;
			}
			break;

			case 0x05: {
			}
			break;

			default: {
			}
			break;
		}
	}

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_REMOTE_CONTROL].state = COMMAND_STATE_IDLE;

	return ret;
}

static module_command_item_t module_command_item_remote_control = {
	.cmd = MODULE_COMMAND_REMOTE_CONTROL,
	.request_code = STATEGRID_CMD_REMOTE_CONTROL_REQUEST,
	.request_prio = 0x06,
	.response_code = STATEGRID_CMD_REMOTE_CONTROL_RESPONSE,
	.response_prio = 0x06,
	.request_callback = request_remote_control,
	.response_callback = response_remote_control,
};


static void _query_status(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_HEARTBEAT].state = COMMAND_STATE_REQUEST;
}

static int request_heartbeat(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_HEARTBEAT].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_heartbeat(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_HEARTBEAT].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_heartbeat = {
	.cmd = MODULE_COMMAND_HEARTBEAT,
	.request_code = STATEGRID_CMD_HEARTBEAT_REQUEST,
	.request_prio = 0x06,
	.response_code = STATEGRID_CMD_HEARTBEAT_RESPONSE,
	.response_prio = 0x06,
	.request_callback = request_heartbeat,
	.response_callback = response_heartbeat,
};

static int request_remote_data(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_REMOTE_DATA].state = COMMAND_STATE_RESPONSE;
	return ret;
}

typedef struct {
	uint8_t unused1 : 3;

	//分组模式
	//0x00 --- 固定分组
	//0x01 --- 动态分组
	uint8_t group_mode : 1;
	uint8_t fault : 1;
	uint8_t warn : 1;
	//工作状态
	//0x01 --- 待机状态
	//0x02 --- 工作状态
	uint8_t state : 2;

	uint8_t other_fault : 1;
	uint8_t discharge_fault : 1;
	uint8_t fan_state : 1;
	uint8_t protect_overcurrent : 1;
	uint8_t protect_overtemperature : 1;
	uint8_t output_lowvoltage : 1;
	uint8_t output_overvoltage : 1;
	uint8_t input_fault : 1;

	uint8_t output_voltage_lo;
	uint8_t output_voltage_hi;

	uint8_t output_current_lo;
	uint8_t output_current_hi;
	uint8_t group;
} stategrid_remote_data_t;

static int response_remote_data(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 0;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	stategrid_remote_data_t *stategrid_remote_data = (stategrid_remote_data_t *)data;

	power_modules_info->power_module_info[module_id].power_module_status.poweroff = (stategrid_remote_data->state == 0x02) ? 0 : 1;
	power_modules_info->power_module_info[module_id].power_module_status.fault = stategrid_remote_data->fault;
	power_modules_info->power_module_info[module_id].power_module_status.output_state = 1;
	power_modules_info->power_module_info[module_id].power_module_status.fan_state = stategrid_remote_data->fan_state;
	power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage = stategrid_remote_data->input_fault;
	power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage = stategrid_remote_data->input_fault;
	power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage = stategrid_remote_data->output_overvoltage;
	power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage = stategrid_remote_data->output_lowvoltage;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent = stategrid_remote_data->protect_overcurrent;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature = stategrid_remote_data->protect_overtemperature;

	power_modules_info->power_module_info[module_id].output_voltage = get_u16_from_u8_lh(stategrid_remote_data->output_voltage_lo, stategrid_remote_data->output_voltage_hi);
	power_modules_info->power_module_info[module_id].output_current = get_u16_from_u8_lh(stategrid_remote_data->output_current_lo, stategrid_remote_data->output_current_hi) / 10;

	if(stategrid_remote_data->group != power_modules_info->power_module_info[module_id].channel_id) {
		debug("");
	}

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_REMOTE_DATA].state = COMMAND_STATE_IDLE;
	return ret;
}

static module_command_item_t module_command_item_remote_data = {
	.cmd = MODULE_COMMAND_REMOTE_DATA,
	.request_code = STATEGRID_CMD_REMOTE_DATA_REQUEST,
	.request_prio = 0x06,
	.response_code = STATEGRID_CMD_REMOTE_DATA_RESPONSE,
	.response_prio = 0x06,
	.request_callback = request_remote_data,
	.response_callback = response_remote_data,
};

#pragma pack(push, 1)
typedef struct {
	uint8_t total_frame;
	uint8_t len_lo;
	uint8_t len_hi;
} stategrid_multi_frame_data_header_t;

typedef struct {
	uint8_t crc_lo;
	uint8_t crc_hi;
} stategrid_multi_frame_data_crc_t;

typedef struct {
	stategrid_multi_frame_data_header_t header;
	//一桩(机)多充时用来标记接口号。
	//一桩(机)一充时此项为 0,多个接口时顺序对每个
	//接口进行编号,范围 1~255。
	uint8_t interface;

	//0x01 --- 直流充电控制器
	//0x02 --- 交流充电控制器
	//0x03 --- 功率控制模块
	//0x04 --- 充电模块
	//0x05 --- 开关模块
	uint8_t type;//0x04
	uint8_t addr;//0x01
	uint8_t serial_lo;//定值序号
	uint8_t serial_hi;//定值序号
	uint8_t result;
	//0x01 --- 固定分组模式,充电模块按照固定分组模式下地址与分组的对应关系响应遥控命令(PF:0x00)。
	//0x02 --- 动态分组模式,充电模块按照直流充电控制器下发的分组信息,响应遥控命令(PF:0x05)
	uint8_t group_mode;
	stategrid_multi_frame_data_crc_t crc;
} stategrid_set_group_mode_t;

#pragma pack(pop)

typedef struct {
	stategrid_set_group_mode_t set_group_mode_request;
	uint8_t request_index;
	stategrid_set_group_mode_t set_group_mode_response;
} stategrid_set_group_mode_ctx_t;

typedef struct {
	stategrid_set_group_mode_ctx_t stategrid_set_group_mode_ctx;
} stategrid_data_ctx_t;

static stategrid_data_ctx_t *stategrid_data_ctx = NULL;

static void _init(power_modules_info_t *power_modules_info)
{
	if(stategrid_data_ctx == NULL) {
		stategrid_data_ctx = os_calloc(power_modules_info->power_module_number, sizeof(stategrid_data_ctx_t));
		OS_ASSERT(stategrid_data_ctx != NULL);
	}
}

static void _set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	uint16_t crc;
	stategrid_set_group_mode_t *stategrid_set_group_mode;

	OS_ASSERT(stategrid_data_ctx != NULL);

	stategrid_set_group_mode = &stategrid_data_ctx[module_id].stategrid_set_group_mode_ctx.set_group_mode_request;

	stategrid_set_group_mode->header.total_frame = (sizeof(stategrid_set_group_mode_t) + 7 - 1) / 7;
	stategrid_set_group_mode->header.len_lo = get_u8_l_from_u16(sizeof(stategrid_set_group_mode_t));
	stategrid_set_group_mode->header.len_hi = get_u8_h_from_u16(sizeof(stategrid_set_group_mode_t));

	stategrid_set_group_mode->interface = 0;
	stategrid_set_group_mode->type = 0x04;
	stategrid_set_group_mode->addr = 0x01;
	stategrid_set_group_mode->serial_lo = get_u8_l_from_u16(13);
	stategrid_set_group_mode->serial_hi = get_u8_h_from_u16(13);
	stategrid_set_group_mode->result = 0x00;
	stategrid_set_group_mode->group_mode = 0x02;
	crc = sum_crc16(stategrid_set_group_mode, sizeof(stategrid_set_group_mode_t) - sizeof(stategrid_multi_frame_data_crc_t));
	stategrid_set_group_mode->crc.crc_lo = get_u8_l_from_u16(crc);
	stategrid_set_group_mode->crc.crc_hi = get_u8_h_from_u16(crc);

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_CONFIG].state = COMMAND_STATE_REQUEST;
}

static int request_config(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = 1;
	uint8_t *data = power_modules_info->can_tx_msg.Data;
	uint16_t sent;
	uint8_t len;
	stategrid_set_group_mode_t *stategrid_set_group_mode;
	uint8_t *buffer;

	OS_ASSERT(stategrid_data_ctx != NULL);
	stategrid_set_group_mode = &stategrid_data_ctx[module_id].stategrid_set_group_mode_ctx.set_group_mode_request;
	sent = stategrid_data_ctx[module_id].stategrid_set_group_mode_ctx.request_index * 7;
	buffer = (uint8_t *)stategrid_set_group_mode;

	len = sizeof(stategrid_set_group_mode_t) - sent;

	if(len > 7) {
		len = 7;
	}

	data[0] = stategrid_data_ctx[module_id].stategrid_set_group_mode_ctx.request_index + 1;//帧号
	memcpy(data + 1, buffer + sent, len);

	if(stategrid_data_ctx[module_id].stategrid_set_group_mode_ctx.request_index + 1 >= stategrid_set_group_mode->header.total_frame) {
		stategrid_data_ctx[module_id].stategrid_set_group_mode_ctx.request_index = 0;
		power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_CONFIG].state = COMMAND_STATE_RESPONSE;
		ret = 0;
	} else {
		stategrid_data_ctx[module_id].stategrid_set_group_mode_ctx.request_index++;
	}

	return ret;
}

static int response_config(power_modules_info_t *power_modules_info, int module_id)
{
	int ret = -1;
	uint8_t *data = power_modules_info->can_rx_msg->Data;
	uint8_t index = data[0] - 1;
	uint16_t received = index * 7;
	uint8_t *buffer;
	uint8_t len = 0;
	stategrid_set_group_mode_t *stategrid_set_group_mode;

	OS_ASSERT(stategrid_data_ctx != NULL);
	stategrid_set_group_mode = &stategrid_data_ctx[module_id].stategrid_set_group_mode_ctx.set_group_mode_response;
	buffer = (uint8_t *)stategrid_set_group_mode;

	if(received >= sizeof(stategrid_set_group_mode_t)) {
		debug("");
		return ret;
	}

	len = sizeof(stategrid_set_group_mode_t) - received;

	if(len > 7) {
		len = 7;
	}

	memcpy(buffer + received, data + 1, len);

	if(received + len == sizeof(stategrid_set_group_mode_t)) {
		if(stategrid_set_group_mode->header.total_frame != (sizeof(stategrid_set_group_mode_t) + 7 - 1) / 7) {
			return ret;
		}

		if(get_u16_from_u8_lh(stategrid_set_group_mode->header.len_lo, stategrid_set_group_mode->header.len_hi) != get_u8_l_from_u16(sizeof(stategrid_set_group_mode_t))) {
			return ret;
		}

		if(get_u16_from_u8_lh(stategrid_set_group_mode->serial_lo, stategrid_set_group_mode->serial_hi) != 13) {
			return ret;
		}

		if(get_u16_from_u8_lh(stategrid_set_group_mode->crc.crc_lo, stategrid_set_group_mode->crc.crc_hi) != sum_crc16(stategrid_set_group_mode, sizeof(stategrid_set_group_mode_t) - sizeof(stategrid_multi_frame_data_crc_t))) {
			return ret;
		}
	}

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_CONFIG].state = COMMAND_STATE_IDLE;

	power_modules_info->power_module_info[module_id].cmd_ctx[MODULE_COMMAND_REMOTE_CONTROL].state = COMMAND_STATE_REQUEST;

	ret = 0;

	return ret;
}

static module_command_item_t module_command_item_config = {
	.cmd = MODULE_COMMAND_CONFIG,
	.request_code = STATEGRID_CMD_CONFIG_REQUEST,
	.request_prio = 0x06,
	.response_code = STATEGRID_CMD_CONFIG_RESPONSE,
	.response_prio = 0x06,
	.request_callback = request_config,
	.response_callback = response_config,
};

static module_command_item_t *module_command_item_table[] = {
	&module_command_item_remote_control,
	&module_command_item_heartbeat,
	&module_command_item_remote_data,
	&module_command_item_config,
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
			u_stategrid_ext_id_t u_stategrid_ext_id;
			u_stategrid_pgn_t u_stategrid_pgn;
			uint32_t ticks = osKernelSysTick();
			uint8_t send_loop = 0;

			power_modules_request_periodic(power_modules_info);

			if(cmd_ctx->state != COMMAND_STATE_REQUEST) {
				continue;
			}

			u_stategrid_ext_id.v = 0;
			u_stategrid_ext_id.s.sa = 0xa0;
			u_stategrid_ext_id.s.ps = module_id + 0x80;
			u_stategrid_pgn.v = item->request_code;
			u_stategrid_ext_id.s.pf = u_stategrid_pgn.s.pgn1;
			u_stategrid_ext_id.s.dp = 0;
			u_stategrid_ext_id.s.r = 0;
			u_stategrid_ext_id.s.p = item->request_prio;

			power_modules_info->can_tx_msg.ExtId = u_stategrid_ext_id.v;
			power_modules_info->can_tx_msg.RTR = CAN_RTR_DATA;
			power_modules_info->can_tx_msg.IDE = CAN_ID_EXT;
			power_modules_info->can_tx_msg.DLC = 8;

			memset(power_modules_info->can_tx_msg.Data, 0, 8);

			do {
				ret = item->request_callback(power_modules_info, module_id);

				if(ret == 1) {
					ret = 0;
					send_loop = 1;
				} else {
					send_loop = 0;
				}

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

			} while(send_loop == 1);

			osDelay(5);
		}
	}
}

static int _power_modules_response(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	int i;
	u_stategrid_ext_id_t u_stategrid_ext_id;
	int module_addr;
	int module_id;
	uint8_t response_code;

	power_modules_info->can_rx_msg = can_rx_msg;

	u_stategrid_ext_id.v = power_modules_info->can_rx_msg->ExtId;

	module_addr = u_stategrid_ext_id.s.sa;

	if((module_addr >= 0x80) && (module_addr < (0x80 + power_modules_info->power_module_number))) {
		module_id = module_addr - 0x80;
	} else {
		return ret;
	}

	if(u_stategrid_ext_id.s.ps != 0xa0) {
		return ret;
	}

	response_code = u_stategrid_ext_id.s.pf;

	for(i = 0; i < ARRAY_SIZE(module_command_item_table); i++) {
		module_command_item_t *item = module_command_item_table[i];
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		connect_state_t *connect_state = &power_module_info->connect_state;

		if(response_code != item->response_code) {
			continue;
		}

		if(u_stategrid_ext_id.s.p != item->response_prio) {
			return ret;
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


power_modules_handler_t power_modules_handler_stategrid = {
	.power_module_type = POWER_MODULE_TYPE_STATEGRID,
	.cmd_size = ARRAY_SIZE(module_command_item_table),
	.init = _init,
	.set_out_voltage_current = _set_out_voltage_current,
	.set_poweroff = _set_poweroff,
	.query_status = _query_status,
	.query_a_line_input_voltage = NULL,
	.query_b_line_input_voltage = NULL,
	.query_c_line_input_voltage = NULL,
	.power_modules_request = _power_modules_request,
	.power_modules_response = _power_modules_response,
};
