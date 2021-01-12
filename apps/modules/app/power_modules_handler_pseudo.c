

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_pseudo.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 17时36分29秒
 *   修改日期：2021年01月12日 星期二 12时30分06秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_pseudo.h"
#include "os_utils.h"
#include <string.h>

//#define LOG_NONE
#include "log.h"

static void set_out_voltage_current_pseudo(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint32_t current)
{
	power_modules_info->power_module_info[module_id].setting_current = current;
	power_modules_info->power_module_info[module_id].setting_voltage = voltage;

	power_modules_info->power_module_info[module_id].output_current = current;
	power_modules_info->power_module_info[module_id].output_voltage = voltage;
	debug("module_id %d current:%d, voltage:%d\n", module_id, current, voltage);
}

static void set_poweroff_pseudo(power_modules_info_t *power_modules_info, int module_id, uint8_t poweroff)
{
	power_modules_info->power_module_info[module_id].poweroff = poweroff;
	power_modules_info->power_module_info[module_id].power_module_status.poweroff = poweroff;
	power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff = poweroff;
	debug("module_id %d poweroff:%d\n", module_id, poweroff);
}

static void query_status_pseudo(power_modules_info_t *power_modules_info, int module_id)
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

	debug("module_id %d poweroff:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.poweroff);
	debug("module_id %d fault:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.fault);
	debug("module_id %d output_state:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.output_state);
	debug("module_id %d fan_state:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.fan_state);
	debug("module_id %d input_overvoltage:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.input_overvoltage);
	debug("module_id %d input_lowvoltage:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.input_lowvoltage);
	debug("module_id %d output_overvoltage:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.output_overvoltage);
	debug("module_id %d output_lowvoltage:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.output_lowvoltage);
	debug("module_id %d protect_overcurrent:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.protect_overcurrent);
	debug("module_id %d protect_overtemperature:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.protect_overtemperature);
	debug("module_id %d setting_poweroff:%d\n", module_id, power_modules_info->power_module_info[module_id].power_module_status.setting_poweroff);
}

static void query_a_line_input_voltage_pseudo(power_modules_info_t *power_modules_info, int module_id)
{
}

static void query_b_line_input_voltage_pseudo(power_modules_info_t *power_modules_info, int module_id)
{
}

static void query_c_line_input_voltage_pseudo(power_modules_info_t *power_modules_info, int module_id)
{
}

static void power_modules_request_pseudo(power_modules_info_t *power_modules_info)
{
	int module_id;
	int i;
	int ret;

	for(module_id = 0; module_id < power_modules_info->power_module_number; module_id++) {
		//power_module_info_t *power_module_info = power_modules_info->power_module_info + module_id;
	}
}

static int power_modules_response_pseudo(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg)
{
	int ret = 0;

	return ret;
}

power_modules_handler_t power_modules_handler_pseudo = {
	.power_module_type = POWER_MODULE_TYPE_PSEUDO,
	.cmd_size = 0,
	.set_out_voltage_current = set_out_voltage_current_pseudo,
	.set_poweroff = set_poweroff_pseudo,
	.query_status = query_status_pseudo,
	.query_a_line_input_voltage = query_a_line_input_voltage_pseudo,
	.query_b_line_input_voltage =  query_b_line_input_voltage_pseudo,
	.query_c_line_input_voltage = query_c_line_input_voltage_pseudo,
	.power_modules_request = power_modules_request_pseudo,
	.power_modules_response = power_modules_response_pseudo,
};
