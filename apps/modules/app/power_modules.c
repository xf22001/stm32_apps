

/*================================================================
 *
 *
 *   文件名称：power_modules.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 15时34分29秒
 *   修改日期：2020年07月09日 星期四 12时48分57秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules.h"

#include "os_utils.h"
#include "power_modules_handler_huawei.h"
#include "power_modules_handler_increase.h"
#include <string.h>

#include "log.h"

static LIST_HEAD(power_modules_info_list);
static osMutexId power_modules_info_list_mutex = NULL;

static power_modules_handler_t *power_modules_handler_sz[] = {
	&power_modules_handler_huawei,
	&power_modules_handler_increase,
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

static power_modules_info_t *get_power_modules_info(channels_info_config_t *channels_info_config)
{
	power_modules_info_t *power_modules_info = NULL;
	power_modules_info_t *power_modules_info_item = NULL;
	osStatus os_status;

	if(power_modules_info_list_mutex == NULL) {
		return power_modules_info;
	}

	os_status = osMutexWait(power_modules_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(power_modules_info_item, &power_modules_info_list, power_modules_info_t, list) {
		if(power_modules_info_item->channels_info_config == channels_info_config) {
			power_modules_info = power_modules_info_item;
			break;
		}
	}

	os_status = osMutexRelease(power_modules_info_list_mutex);

	if(os_status != osOK) {
	}

	return power_modules_info;
}

void free_power_modules_info(power_modules_info_t *power_modules_info)
{
	osStatus os_status;
	int i;

	if(power_modules_info == NULL) {
		return;
	}

	if(power_modules_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(power_modules_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&power_modules_info->list);

	os_status = osMutexRelease(power_modules_info_list_mutex);

	if(os_status != osOK) {
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

static int power_modules_set_channels_info_config(power_modules_info_t *power_modules_info, channels_info_config_t *channels_info_config)
{
	int ret = -1;
	int i;

	can_info_t *can_info;
	power_module_info_t *power_module_info;
	int max_cmd_size = get_power_modules_handler_max_cmd_size();

	debug("power_module_number:%d\n", channels_info_config->power_module_number);

	power_modules_info->power_module_number = channels_info_config->power_module_number;

	if(power_modules_info->power_module_number == 0) {
		debug("\n");
		return ret;
	}

	can_info = get_or_alloc_can_info(channels_info_config->hcan_power);

	if(can_info == NULL) {
		debug("\n");
		return ret;
	}

	power_modules_info->can_info = can_info;

	power_module_info = (power_module_info_t *)os_alloc(sizeof(power_module_info_t) * power_modules_info->power_module_number);

	if(power_module_info == NULL) {
		debug("\n");
		return ret;
	}

	power_modules_info->power_module_info = power_module_info;

	for(i = 0; i < power_modules_info->power_module_number; i++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + i;

		power_module_info->cmd_ctx = (can_com_cmd_ctx_t *)os_alloc(sizeof(can_com_cmd_ctx_t) * max_cmd_size);

		if(power_module_info->cmd_ctx == NULL) {
			debug("\n");
			return ret;
		}
	}

	if(power_modules_set_type(power_modules_info, POWER_MODULE_TYPE_INCREASE) != 0) {
		debug("\n");
		return ret;
	}

	ret = 0;
	return ret;
}

power_modules_info_t *get_or_alloc_power_modules_info(channels_info_config_t *channels_info_config)
{
	power_modules_info_t *power_modules_info = NULL;
	osStatus os_status;

	power_modules_info = get_power_modules_info(channels_info_config);

	if(power_modules_info != NULL) {
		return power_modules_info;
	}

	if(power_modules_info_list_mutex == NULL) {
		osMutexDef(power_modules_info_list_mutex);
		power_modules_info_list_mutex = osMutexCreate(osMutex(power_modules_info_list_mutex));

		if(power_modules_info_list_mutex == NULL) {
			debug("\n");
			return power_modules_info;
		}
	}

	power_modules_info = (power_modules_info_t *)os_alloc(sizeof(power_modules_info_t));

	if(power_modules_info == NULL) {
		debug("\n");
		return power_modules_info;
	}

	memset(power_modules_info, 0, sizeof(power_modules_info_t));

	power_modules_info->channels_info_config = channels_info_config;

	os_status = osMutexWait(power_modules_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&power_modules_info->list, &power_modules_info_list);

	os_status = osMutexRelease(power_modules_info_list_mutex);

	if(os_status != osOK) {
	}

	if(power_modules_set_channels_info_config(power_modules_info, channels_info_config) != 0) {
		debug("\n");
		goto failed;
	}

	power_modules_info->rate_current = 21;

	return power_modules_info;
failed:

	free_power_modules_info(power_modules_info);
	power_modules_info = NULL;

	return power_modules_info;
}

int power_modules_set_type(power_modules_info_t *power_modules_info, power_module_type_t power_module_type)
{
	int ret = -1;
	int i;
	power_modules_handler_t *power_modules_handler;

	power_modules_handler = get_power_modules_handler(power_module_type);

	if(power_modules_handler == NULL) {
		debug("\n");
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

void set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint16_t current)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(module_id >= power_modules_info->power_module_number) {
		debug("\n");
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
		debug("\n");
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
		debug("\n");
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
		debug("\n");
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
		debug("\n");
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
		debug("\n");
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

void power_modules_request(power_modules_info_t *power_modules_info)
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

int power_modules_response(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
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
