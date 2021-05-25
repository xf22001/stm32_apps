

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_pseudo.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 17时36分29秒
 *   修改日期：2021年05月25日 星期二 11时25分48秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_pseudo.h"
#include "os_utils.h"
#include <string.h>

#define LOG_DISABLE
#include "log.h"

static void _set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].output_voltage = power_modules_info->power_module_info[module_id].setting_voltage / 100;
	debug("module_id %d output voltage:%d", module_id, power_modules_info->power_module_info[module_id].setting_voltage / 100);

	power_modules_info->power_module_info[module_id].output_current = power_modules_info->power_module_info[module_id].setting_current / 100;
	debug("module_id %d output current:%d", module_id, power_modules_info->power_module_info[module_id].setting_current / 100);
}

static void _set_poweroff(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].power_module_status.poweroff = power_modules_info->power_module_info[module_id].poweroff;
	power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff = power_modules_info->power_module_info[module_id].poweroff;
	debug("module_id %d poweroff:%d", module_id, power_modules_info->power_module_info[module_id].poweroff);
}

static void _query_status(power_modules_info_t *power_modules_info, int module_id)
{
	power_modules_info->power_module_info[module_id].power_module_status.fault = 0;
	power_modules_info->power_module_info[module_id].power_module_status.output_state = 0;
	power_modules_info->power_module_info[module_id].power_module_status.fan_state = 0;
	power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage = 0;
	power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage = 0;
	power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage = 0;
	power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage = 0;

	power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent = 0;
	power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature = 0;

	debug("module_id %d poweroff:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.poweroff);
	debug("module_id %d fault:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.fault);
	debug("module_id %d output_state:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.output_state);
	debug("module_id %d fan_state:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.fan_state);
	debug("module_id %d input_overvoltage:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage);
	debug("module_id %d input_lowvoltage:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage);
	debug("module_id %d output_overvoltage:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage);
	debug("module_id %d output_lowvoltage:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage);
	debug("module_id %d protect_overcurrent:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent);
	debug("module_id %d protect_overtemperature:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature);
	debug("module_id %d setting_poweroff:%d", module_id, power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff);
}

static void _query_a_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
}

static void _query_b_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
}

static void _query_c_line_input_voltage(power_modules_info_t *power_modules_info, int module_id)
{
}

static void _power_modules_request(power_modules_info_t *power_modules_info)
{
	int module_id;

	for(module_id = 0; module_id < power_modules_info->power_module_number; module_id++) {
		power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
		connect_state_t *connect_state = &power_module_info->connect_state;

		update_connect_state(connect_state, 1);
	}
}

static int _power_modules_response(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
{
	int ret = 0;

	return ret;
}

power_modules_handler_t power_modules_handler_pseudo = {
	.power_module_type = POWER_MODULE_TYPE_PSEUDO,
	.cmd_size = 0,
	.set_out_voltage_current = _set_out_voltage_current,
	.set_poweroff = _set_poweroff,
	.query_status = _query_status,
	.query_a_line_input_voltage = _query_a_line_input_voltage,
	.query_b_line_input_voltage =  _query_b_line_input_voltage,
	.query_c_line_input_voltage = _query_c_line_input_voltage,
	.power_modules_request = _power_modules_request,
	.power_modules_response = _power_modules_response,
};
