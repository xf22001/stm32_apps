

/*================================================================
 *   
 *   
 *   文件名称：charger_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时33分30秒
 *   修改日期：2020年04月27日 星期一 14时08分38秒
 *   描    述：
 *
 *================================================================*/
#include "charger_config.h"
#include "charger_op_interface.h"
#include "os_utils.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

charger_info_config_t charger_info_config_sz[] = {
	{
		.hcan = &hcan1,
		.report_charger_status = report_charger_status,
		.set_auxiliary_power_state = set_auxiliary_power_state,
		.set_gun_lock_state = set_gun_lock_state,
		.set_power_output_enable = set_power_output_enable,
		.discharge = discharge,
		.precharge = precharge,
		.relay_endpoint_overvoltage_status = relay_endpoint_overvoltage_status,
		.insulation_check = insulation_check,
		.battery_voltage_status = battery_voltage_status,
		.wait_no_current = wait_no_current,
	},
	{
		.hcan = &hcan2,
		.report_charger_status = report_charger_status,
		.set_auxiliary_power_state = set_auxiliary_power_state,
		.set_gun_lock_state = set_gun_lock_state,
		.set_power_output_enable = set_power_output_enable,
		.discharge = discharge,
		.precharge = precharge,
		.relay_endpoint_overvoltage_status = relay_endpoint_overvoltage_status,
		.insulation_check = insulation_check,
		.battery_voltage_status = battery_voltage_status,
		.wait_no_current = wait_no_current,
	},
};

charger_info_config_t *get_charger_info_config(can_info_t *can_info)
{
	int i;
	charger_info_config_t *charger_info_config = NULL;
	charger_info_config_t *charger_info_config_item = NULL;

	for(i = 0; i < sizeof(charger_info_config_sz) / sizeof(charger_info_config_t); i++) {
		charger_info_config_item = charger_info_config_sz + i;

		if(can_info->hcan == charger_info_config_item->hcan) {
			charger_info_config = charger_info_config_item;
			break;
		}
	}

	if(charger_info_config != NULL) {
		if(charger_info_config->report_charger_status == NULL) {
			app_panic();
		}

		if(charger_info_config->set_auxiliary_power_state == NULL) {
			app_panic();
		}

		if(charger_info_config->set_gun_lock_state == NULL) {
			app_panic();
		}

		if(charger_info_config->set_power_output_enable == NULL) {
			app_panic();
		}

		if(charger_info_config->discharge == NULL) {
			app_panic();
		}

		if(charger_info_config->precharge == NULL) {
			app_panic();
		}

		if(charger_info_config->relay_endpoint_overvoltage_status == NULL) {
			app_panic();
		}

		if(charger_info_config->insulation_check == NULL) {
			app_panic();
		}

		if(charger_info_config->battery_voltage_status == NULL) {
			app_panic();
		}

		if(charger_info_config->wait_no_current == NULL) {
			app_panic();
		}
	}

	return charger_info_config;
}
