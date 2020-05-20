

/*================================================================
 *
 *
 *   文件名称：power_modules.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 15时34分29秒
 *   修改日期：2020年05月20日 星期三 15时11分10秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules.h"

#include "os_utils.h"
#include "power_modules_handler_huawei.h"
#include "power_modules_handler_increase.h"

static LIST_HEAD(power_modules_info_list);
static osMutexId power_modules_info_list_mutex = NULL;

static power_modules_handler_t *power_modules_handler_sz[] = {
	&power_modules_handler_huawei,
	&power_modules_handler_increase,
};

power_modules_handler_t *get_power_modules_handler(power_module_type_t power_module_type)
{
	int i;
	power_modules_handler_t *power_modules_handler = NULL;
	power_modules_handler_t *power_modules_handler_item = NULL;

	for(i = 0; i < sizeof(power_modules_handler_sz) / sizeof(power_modules_handler_t *); i++) {
		power_modules_handler_item = power_modules_handler_sz[i];

		if(power_modules_handler_item->power_module_type == power_module_type) {
			power_modules_handler = power_modules_handler_item;
			break;
		}
	}

	return power_modules_handler;
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

	os_free(power_modules_info);
}

static int power_modules_set_channels_info_config(power_modules_info_t *power_modules_info, channels_info_config_t *channels_info_config)
{
	int ret = -1;

	power_modules_handler_t *power_modules_handler;

	power_modules_info->power_module_type = POWER_MODULE_TYPE_INCREASE;//test
	power_modules_handler = get_power_modules_handler(power_modules_info->power_module_type);

	if(power_modules_handler == NULL) {
		return ret;
	}

	power_modules_info->power_modules_handler = power_modules_handler;

	ret = 0;
	return ret;
}

power_modules_info_t *get_or_alloc_power_modules_info(channels_info_config_t *channels_info_config)
{
	power_modules_info_t *power_modules_info = NULL;
	osStatus os_status;

	if(channels_info_config == NULL) {
		return power_modules_info;
	}

	power_modules_info = get_power_modules_info(channels_info_config);

	if(power_modules_info != NULL) {
		return power_modules_info;
	}

	if(power_modules_info_list_mutex == NULL) {
		osMutexDef(power_modules_info_list_mutex);
		power_modules_info_list_mutex = osMutexCreate(osMutex(power_modules_info_list_mutex));

		if(power_modules_info_list_mutex == NULL) {
			return power_modules_info;
		}
	}

	power_modules_info = (power_modules_info_t *)os_alloc(sizeof(power_modules_info_t));

	if(power_modules_info == NULL) {
		return power_modules_info;
	}

	power_modules_info->channels_info_config = channels_info_config;

	os_status = osMutexWait(power_modules_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&power_modules_info->list, &power_modules_info_list);

	os_status = osMutexRelease(power_modules_info_list_mutex);

	if(os_status != osOK) {
	}

	if(power_modules_set_channels_info_config(power_modules_info, channels_info_config) != 0) {
		goto failed;
	}

	power_modules_info->rate_current = 21;

	return power_modules_info;
failed:

	if(power_modules_info) {
		free_power_modules_info(power_modules_info);
	}

	return power_modules_info;
}

void power_modules_handler_update(power_modules_info_t *power_modules_info)
{
	power_modules_info->power_modules_handler = power_modules_info->power_modules_handler;//test
}

void set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint16_t current)
{
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

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

	if(power_modules_handler == NULL) {
		return;
	}

	if(power_modules_handler->query_c_line_input_voltage == NULL) {
		return;
	}

	power_modules_handler->query_c_line_input_voltage(power_modules_info, module_id);
}

int power_modules_init(power_modules_info_t *power_modules_info)
{
	int ret = -1;
	power_modules_handler_t *power_modules_handler = (power_modules_handler_t *)power_modules_info->power_modules_handler;

	if(power_modules_handler == NULL) {
		return ret;
	}

	if(power_modules_handler->power_modules_init == NULL) {
		return ret;
	}

	power_modules_info->power_modules_valid = 0;

	ret = power_modules_handler->power_modules_init(power_modules_info);

	return ret;
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

	if(power_modules_info->power_modules_valid == 0) {
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

	if(power_modules_info->power_modules_valid == 0) {
		return ret;
	}

	ret = power_modules_handler->power_modules_response(power_modules_info, can_rx_msg);

	return ret;
}

