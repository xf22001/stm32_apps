

/*================================================================
 *
 *
 *   文件名称：power_modules.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 15时34分29秒
 *   修改日期：2021年03月27日 星期六 12时23分24秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules.h"

#include <string.h>

#include "os_utils.h"
#include "object_class.h"
#include "power_modules_handler_huawei.h"
#include "power_modules_handler_increase.h"
#include "power_modules_handler_pseudo.h"
#include "channels.h"
#include "can_data_task.h"

//#define LOG_NONE
#include "log.h"

static object_class_t *power_modules_class = NULL;

static power_modules_handler_t *power_modules_handler_sz[] = {
	&power_modules_handler_huawei,
	&power_modules_handler_increase,
	&power_modules_handler_pseudo,
};

static power_modules_handler_t *get_power_modules_handler(power_module_type_t power_module_type)
{
	int i;
	power_modules_handler_t *power_modules_handler = NULL;
	power_modules_handler_t *power_modules_handler_item = NULL;

	for(i = 0; i < ARRAY_SIZE(power_modules_handler_sz); i++) {
		power_modules_handler_item = power_modules_handler_sz[i];

		if(power_modules_handler_item->power_module_type == power_module_type) {
			power_modules_handler = power_modules_handler_item;
			break;
		}
	}

	return power_modules_handler;
}

static int get_power_modules_handler_max_cmd_size(void)
{
	int i;
	int max_cmd_size = 0;
	power_modules_handler_t *power_modules_handler_item = NULL;

	for(i = 0; i < ARRAY_SIZE(power_modules_handler_sz); i++) {
		power_modules_handler_item = power_modules_handler_sz[i];

		if(power_modules_handler_item->cmd_size > max_cmd_size) {
			max_cmd_size = power_modules_handler_item->cmd_size;
		}
	}

	return max_cmd_size;
}

int power_modules_set_type(power_modules_info_t *power_modules_info, power_module_type_t power_module_type)
{
	int ret = -1;
	int i;
	power_modules_handler_t *power_modules_handler;

	power_modules_handler = get_power_modules_handler(power_module_type);

	if(power_modules_handler == NULL) {
		debug("");
		return ret;
	}

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + i;

		memset(power_module_info->cmd_ctx, 0, sizeof(can_com_cmd_ctx_t) * power_modules_handler->cmd_size);
	}

	power_modules_info->power_module_type = power_module_type;
	power_modules_info->power_modules_handler = power_modules_handler;
	ret = 0;

	return ret;
}

void set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint32_t current)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(module_id >= power_modules_info->power_module_number) {
		debug("");
		return;
	}

	if(power_modules_handler == NULL) {
		return;
	}

	if(power_modules_handler->set_out_voltage_current == NULL) {
		return;
	}

	power_modules_handler->set_out_voltage_current(power_modules_info, module_id, voltage, current);
}

void set_poweroff(power_modules_info_t *power_modules_info, int module_id, uint8_t poweroff)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(module_id >= power_modules_info->power_module_number) {
		debug("");
		return;
	}

	if(power_modules_handler == NULL) {
		return;
	}

	if(power_modules_handler->set_poweroff == NULL) {
		return;
	}

	power_modules_handler->set_poweroff(power_modules_info, module_id, poweroff);
}

void query_status(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(module_id >= power_modules_info->power_module_number) {
		debug("");
		return;
	}

	if(power_modules_handler == NULL) {
		return;
	}

	if(power_modules_handler->query_status == NULL) {
		return;
	}

	power_modules_handler->query_status(power_modules_info, module_id);
}

void query_a_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(module_id >= power_modules_info->power_module_number) {
		debug("");
		return;
	}

	if(power_modules_handler == NULL) {
		return;
	}

	if(power_modules_handler->query_a_line_input_voltage == NULL) {
		return;
	}

	power_modules_handler->query_a_line_input_voltage(power_modules_info, module_id);
}

void query_b_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(module_id >= power_modules_info->power_module_number) {
		debug("");
		return;
	}

	if(power_modules_handler == NULL) {
		return;
	}

	if(power_modules_handler->query_b_line_input_voltage == NULL) {
		return;
	}

	power_modules_handler->query_b_line_input_voltage(power_modules_info, module_id);
}

void query_c_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(module_id >= power_modules_info->power_module_number) {
		debug("");
		return;
	}

	if(power_modules_handler == NULL) {
		return;
	}

	if(power_modules_handler->query_c_line_input_voltage == NULL) {
		return;
	}

	power_modules_handler->query_c_line_input_voltage(power_modules_info, module_id);
}

static void power_modules_request(power_modules_info_t *power_modules_info)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(power_modules_handler == NULL) {
		return;
	}

	if(power_modules_handler->power_modules_request == NULL) {
		return;
	}

	power_modules_handler->power_modules_request(power_modules_info);
}

static int power_modules_response(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
{
	int ret = -1;
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(power_modules_handler == NULL) {
		return ret;
	}

	if(power_modules_handler->power_modules_response == NULL) {
		return ret;
	}

	ret = power_modules_handler->power_modules_response(power_modules_info, can_rx_msg);

	return ret;
}

static void can_data_request(void *fn_ctx, void *chain_ctx)
{
	power_modules_info_t *power_modules_info = (power_modules_info_t *)fn_ctx;

	if(fn_ctx == NULL) {
		return;
	}

	power_modules_request(power_modules_info);
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	power_modules_info_t *power_modules_info = (power_modules_info_t *)fn_ctx;
	can_rx_msg_t *can_rx_msg = can_get_msg(power_modules_info->can_info);

	if(fn_ctx == NULL) {
		return;
	}

	power_modules_response(power_modules_info, can_rx_msg);
}

uint8_t get_module_connect_state(power_module_info_t *power_module_info)
{
	can_com_connect_state_t *connect_state = &power_module_info->connect_state;

	return can_com_get_connect_state(connect_state);
}

uint32_t get_module_connect_stamp(power_module_info_t *power_module_info)
{
	can_com_connect_state_t *connect_state = &power_module_info->connect_state;

	return can_com_get_connect_stamp(connect_state);
}

static void free_power_modules_info(power_modules_info_t *power_modules_info)
{
	int i;

	if(power_modules_info == NULL) {
		return;
	}

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + i;

		if(power_module_info->cmd_ctx != NULL) {
			os_free(power_module_info->cmd_ctx);
		}
	}

	if(power_modules_info->power_module_info != NULL) {
		os_free(power_modules_info->power_module_info);
	}

	os_free(power_modules_info);
}

static int power_modules_set_channels_config(power_modules_info_t *power_modules_info, channels_config_t *channels_config)
{
	int ret = -1;
	int i;

	can_info_t *can_info;
	power_module_info_t *power_module_info;
	int max_cmd_size = get_power_modules_handler_max_cmd_size();
	can_data_task_info_t *can_data_task_info;

	power_modules_info->channels_config = channels_config;

	debug("power_module_number:%d", channels_config->power_module_number);

	power_modules_info->power_module_number = channels_config->power_module_number;

	if(power_modules_info->power_module_number == 0) {
		debug("");
		return ret;
	}

	can_info = get_or_alloc_can_info(channels_config->hcan_power);

	if(can_info == NULL) {
		debug("");
		return ret;
	}

	power_modules_info->can_info = can_info;

	power_module_info = (power_module_info_t *)os_alloc(sizeof(power_module_info_t) * power_modules_info->power_module_number);

	if(power_module_info == NULL) {
		debug("");
		return ret;
	}

	memset(power_module_info, 0, sizeof(power_module_info_t) * power_modules_info->power_module_number);

	power_modules_info->power_module_info = power_module_info;

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + i;

		power_module_info->cmd_ctx = (can_com_cmd_ctx_t *)os_alloc(sizeof(can_com_cmd_ctx_t) * max_cmd_size);

		if(power_module_info->cmd_ctx == NULL) {
			debug("");
			return ret;
		}

		memset(power_module_info->cmd_ctx, 0, sizeof(can_com_cmd_ctx_t) * max_cmd_size);
	}

	can_data_task_info = get_or_alloc_can_data_task_info(power_modules_info->can_info->hcan);

	if(can_data_task_info == NULL) {
		app_panic();
	}

	power_modules_info->can_data_request_cb.fn = can_data_request;
	power_modules_info->can_data_request_cb.fn_ctx = power_modules_info;
	add_can_data_task_info_request_cb(can_data_task_info, &power_modules_info->can_data_request_cb);

	power_modules_info->can_data_response_cb.fn = can_data_response;
	power_modules_info->can_data_response_cb.fn_ctx = power_modules_info;
	add_can_data_task_info_response_cb(can_data_task_info, &power_modules_info->can_data_response_cb);

	ret = 0;
	return ret;
}

static power_modules_info_t *alloc_power_modules_info(channels_config_t *channels_config)
{
	power_modules_info_t *power_modules_info = NULL;

	power_modules_info = (power_modules_info_t *)os_alloc(sizeof(power_modules_info_t));

	if(power_modules_info == NULL) {
		debug("");
		return power_modules_info;
	}

	memset(power_modules_info, 0, sizeof(power_modules_info_t));

	if(power_modules_set_channels_config(power_modules_info, channels_config) != 0) {
		debug("");
		goto failed;
	}

	power_modules_info->rate_current = 21;

	return power_modules_info;
failed:

	free_power_modules_info(power_modules_info);
	power_modules_info = NULL;

	return power_modules_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	power_modules_info_t *power_modules_info = (power_modules_info_t *)o;
	channels_config_t *channels_config = (channels_config_t *)ctx;

	if(power_modules_info->channels_config == channels_config) {
		ret = 0;
	}

	return ret;
}

power_modules_info_t *get_or_alloc_power_modules_info(void *ctx)
{
	power_modules_info_t *power_modules_info = NULL;

	os_enter_critical();

	if(power_modules_class == NULL) {
		power_modules_class = object_class_alloc();
	}

	os_leave_critical();

	power_modules_info = (power_modules_info_t *)object_class_get_or_alloc_object(power_modules_class, object_filter, ctx, (object_alloc_t)alloc_power_modules_info, (object_free_t)free_power_modules_info);

	return power_modules_info;
}

