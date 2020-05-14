

/*================================================================
 *
 *
 *   文件名称：bms.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分52秒
 *   修改日期：2020年05月14日 星期四 14时42分36秒
 *   描    述：
 *
 *================================================================*/
#include "bms.h"
#include "bms_handler.h"

#include "os_utils.h"
#include "bitmap_ops.h"
#include <string.h>
#define UDP_LOG
#include "task_probe_tool.h"
#include "task_modbus_slave.h"
#include "eeprom.h"
#include "modbus_data_value.h"

#define UART_LOG
#include "uart_debug.h"

#include "app.h"
#include "log.h"

#ifndef BMS_VERSION_SERIAL
#define BMS_VERSION_SERIAL 0
#endif

#ifndef BMS_VERSION_DAY
#define BMS_VERSION_DAY 0
#endif

#ifndef BMS_VERSION_MONTH
#define BMS_VERSION_MONTH 0
#endif

#ifndef BMS_VERSION_YEAR
#define BMS_VERSION_YEAR 0
#endif

static LIST_HEAD(bms_info_list);
static osMutexId bms_info_list_mutex = NULL;

static bitmap_t *eeprom_modbus_data_bitmap = NULL;

static void bms_data_settings_default_init(bms_data_settings_t *settings)
{
	if(settings == NULL) {
		return;
	}

	memset(settings, 0, sizeof(bms_data_settings_t));

	settings->dst = CHARGER_ADDR;
	settings->src = BMS_ADDR;

	settings->bhm_data.max_charge_voltage = 750 * 10;

	settings->brm_data.brm_data.version_0 = 0x01;
	settings->brm_data.brm_data.version_1 = 0x01;
	settings->brm_data.brm_data.battery_type = 0x04;
	settings->brm_data.brm_data.total_battery_rate_capicity = 120 * 10;
	settings->brm_data.brm_data.total_battery_rate_voltage = 620 * 10;
	memset(settings->brm_data.battery_vendor, 'x', 4);
	settings->brm_data.battery_vendor_sn = 0;
	settings->brm_data.battery_year = (BMS_VERSION_YEAR - 1985) & 0xff;
	settings->brm_data.battery_month = BMS_VERSION_MONTH;
	settings->brm_data.battery_day = BMS_VERSION_DAY;
	settings->brm_data.battery_charge_times[0] = 0;
	settings->brm_data.battery_charge_times[1] = 0;
	settings->brm_data.battery_charge_times[2] = 0;
	settings->brm_data.battery_property = 0;
	settings->brm_data.reserved = 0;
	memcpy(settings->brm_data.vin, "ABCDEFGHI12345678", 17);
	settings->brm_data.version.serial = BMS_VERSION_SERIAL / 10;
	settings->brm_data.version.day = BMS_VERSION_DAY;
	settings->brm_data.version.month = BMS_VERSION_MONTH;
	settings->brm_data.version.year = BMS_VERSION_YEAR;

	settings->bcp_data.max_charge_voltage_single_battery = 420;
	settings->bcp_data.max_charge_current = (4000 - (250 * 10));
	settings->bcp_data.rate_total_power = 75 * 10;
	settings->bcp_data.max_charge_voltage = 750 * 10;
	settings->bcp_data.max_temperature = (85) + 50;
	settings->bcp_data.soc = 60 * 10;
	settings->bcp_data.total_voltage = 500 * 10;
	settings->bcp_data.transfer_type = 0xff;

	settings->bcl_data.require_voltage = 600 * 10;
	settings->bcl_data.require_current = (4000 - 30 * 10);
	settings->bcl_data.charge_mode = 0x02;
	settings->bcs_data.charge_voltage = 600 * 10;
	settings->bcs_data.charge_current = 4000 - 30 * 10;
	settings->bcs_data.u1.s.single_battery_max_voltage = 400;
	settings->bcs_data.u1.s.single_battery_max_group = 4;
	settings->bcs_data.soc = 60 * 10 / 10;
	settings->bcs_data.remain_min = 120;

	settings->bsm_data.single_max_voltage_group = 28;
	settings->bsm_data.battery_max_temperature = 50 + 50;
	settings->bsm_data.battery_max_temperature_sn = 2;
	settings->bsm_data.battery_min_temperature = 45 + 50;
	settings->bsm_data.battery_min_temperature_sn = 11;
	settings->bsm_data.u1.s.single_voltage_state = 0x00;
	settings->bsm_data.u1.s.total_soc_state = 0x00;
	settings->bsm_data.u1.s.battery_current_state = 0x00;
	settings->bsm_data.u1.s.battery_temperature_state = 0x00;
	settings->bsm_data.u2.s.battery_insulation_state = 0x00;
	settings->bsm_data.u2.s.battery_connector_state = 0x00;
	settings->bsm_data.u2.s.battery_charge_enable = 0x01;

	settings->bsd_data.soc = 60 * 10 / 10;
	settings->bsd_data.single_min_voltage = 400;
	settings->bsd_data.single_max_voltage = 400;
	settings->bsd_data.battery_min_temperature = 45 + 50;
	settings->bsd_data.battery_max_temperature = 50 + 50;
}

#define memset_0(data) do { \
	memset(&data, 0, sizeof(data)); \
} while(0)

static void reset_bms_data_settings_charger_data(bms_info_t *bms_info)
{
	memset_0(bms_info->settings->chm_data);
	memset_0(bms_info->settings->crm_data);
	memset_0(bms_info->settings->cts_data);
	memset_0(bms_info->settings->cml_data);
	bms_info->settings->cml_data.max_output_current = 4000;
	bms_info->settings->cml_data.min_output_current = 4000;
	memset_0(bms_info->settings->cro_data);
	memset_0(bms_info->settings->ccs_data);
	bms_info->settings->ccs_data.output_current = 4000;
	memset_0(bms_info->settings->cst_data);
	memset_0(bms_info->settings->csd_data);
	memset_0(bms_info->settings->cem_data);
}

static bms_data_settings_t *bms_data_alloc_settings(void)
{
	bms_data_settings_t *settings = (bms_data_settings_t *)os_alloc(sizeof(bms_data_settings_t));

	if(settings == NULL) {
		return settings;
	}

	bms_data_settings_default_init(settings);

	return settings;
}

static bms_info_t *get_bms_info(bms_info_config_t *bms_info_config)
{
	bms_info_t *bms_info = NULL;
	bms_info_t *bms_info_item = NULL;
	osStatus os_status;

	if(bms_info_list_mutex == NULL) {
		return bms_info;
	}

	os_status = osMutexWait(bms_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}


	list_for_each_entry(bms_info_item, &bms_info_list, bms_info_t, list) {
		if(bms_info_item->can_info->hcan == bms_info_config->hcan) {
			bms_info = bms_info_item;
			break;
		}
	}

	os_status = osMutexRelease(bms_info_list_mutex);

	if(os_status != osOK) {
	}


	return bms_info;
}

void free_bms_info(bms_info_t *bms_info)
{
	osStatus os_status;

	if(bms_info == NULL) {
		return;
	}

	if(bms_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(bms_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&bms_info->list);

	os_status = osMutexRelease(bms_info_list_mutex);

	if(os_status != osOK) {
	}

	if(bms_info->modbus_slave_info != NULL) {
		set_modbus_slave_data_info(bms_info->modbus_slave_info, NULL);
		remove_modbus_slave_data_changed_cb(bms_info->modbus_slave_info, &bms_info->modbus_slave_data_changed_cb);
	}

	if(eeprom_modbus_data_bitmap != NULL) {
		set_bitmap_value(eeprom_modbus_data_bitmap, bms_info->eeprom_modbus_data_index, 0);
	}

	if(bms_info->handle_mutex) {
		os_status = osMutexDelete(bms_info->handle_mutex);

		if(osOK != os_status) {
		}
	}

	if(bms_info->settings != NULL) {
		os_free(bms_info->settings);
	}

	os_free(bms_info);
}

static uint16_t get_gun_on_off(bms_info_t *bms_info)
{
	return bms_info->gun_on_off_state;
}

void set_gun_on_off(bms_info_t *bms_info, uint8_t on_off)
{
	bms_info->gun_on_off_state = on_off;

	if(on_off == 0) {
		HAL_GPIO_WritePin(bms_info->bms_info_config->gpio_port_gun_on_off_state,
		                  bms_info->bms_info_config->gpio_pin_gun_on_off_state,
		                  GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(bms_info->bms_info_config->gpio_port_gun_on_off_state,
		                  bms_info->bms_info_config->gpio_pin_gun_on_off_state,
		                  GPIO_PIN_SET);
	}
}

static int detect_eeprom(eeprom_info_t *eeprom_info)
{
	int i;
	int ret = -1;
	uint8_t id;

	for(i = 0; i < 10; i++) {
		id = eeprom_id(eeprom_info);

		if(id == 0x29) {
			break;
		}

		osDelay(200);
	}

	if(id == 0x29) {
		ret = 0;
	}

	return ret;
}

int save_eeprom_modbus_data(bms_info_t *bms_info)
{
	int ret = -1;
	uint32_t offset;
	uint32_t crc = 0;
	eeprom_modbus_head_t eeprom_modbus_head;

	offset = sizeof(eeprom_modbus_data_t) * bms_info->eeprom_modbus_data_index;

	if(bms_info == NULL) {
		return ret;
	}

	if(detect_eeprom(bms_info->eeprom_info) != 0) {
		_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	eeprom_modbus_head.payload_size = sizeof(eeprom_modbus_data_t);

	crc += eeprom_modbus_head.payload_size;
	crc += (uint32_t)'b';
	crc += (uint32_t)'m';
	crc += (uint32_t)'s';

	eeprom_modbus_head.crc = crc;

	eeprom_write(bms_info->eeprom_info, offset, (uint8_t *)&eeprom_modbus_head, sizeof(eeprom_modbus_head_t));
	offset += sizeof(eeprom_modbus_head_t);

	eeprom_write(bms_info->eeprom_info, offset, (uint8_t *)bms_info->settings, sizeof(bms_data_settings_t));
	offset += sizeof(bms_data_settings_t);

	eeprom_write(bms_info->eeprom_info, offset, (uint8_t *)&bms_info->configs, sizeof(bms_data_configs_t));
	offset += sizeof(bms_data_configs_t);

	return 0;
}

static void modbus_slave_data_changed(void *fn_ctx, void *chain_ctx)
{
	//_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
	save_eeprom_modbus_data((bms_info_t *)fn_ctx);
}

static uint8_t modbus_addr_valid(void *ctx, uint16_t start, uint16_t number)
{
	uint8_t valid = 0;
	uint16_t end = start + number;//无效边界

	if(end <= start) {
		_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	if(start < 0) {
		_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	if(end > MODBUS_ADDR_INVALID) {
		_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	valid = 1;

	return valid;
}

static void modbus_data_get_set(bms_info_t *bms_info, uint16_t addr, uint16_t *value, modbus_data_op_t op)
{
	switch(addr) {
		case MODBUS_ADDR_BRM_VERSION_1: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.brm_data.version_1, op);
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_0: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.brm_data.version_0, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_TYPE: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.brm_data.battery_type, op);
		}
		break;

		case MODBUS_ADDR_BRM_TOTAL_BATTERY_RATE_CAPICITY: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.brm_data.total_battery_rate_capicity, op);
		}
		break;

		case MODBUS_ADDR_BRM_TOTAL_BATTERY_RATE_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.brm_data.total_battery_rate_voltage, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_VENDOR_0:
		case MODBUS_ADDR_BRM_BATTERY_VENDOR_1: {
			modbus_data_value_copy(value, (uint16_t *)bms_info->settings->brm_data.battery_vendor, 4, addr - MODBUS_ADDR_BRM_BATTERY_VENDOR_0, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_VENDOR_SN_0:
		case MODBUS_ADDR_BRM_BATTERY_VENDOR_SN_1: {
			modbus_data_value_copy(value, (uint16_t *)&bms_info->settings->brm_data.battery_vendor_sn, 4, addr - MODBUS_ADDR_BRM_BATTERY_VENDOR_SN_0, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_YEAR: {
			modbus_data_value_with_offset_rw(value, bms_info->settings->brm_data.battery_year, 1985, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_MONTH: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.battery_month, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_DAY: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.battery_day, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_CHARGE_TIMES_0:
		case MODBUS_ADDR_BRM_BATTERY_CHARGE_TIMES_1: {
			modbus_data_value_copy(value, (uint16_t *)bms_info->settings->brm_data.battery_charge_times, 3, addr - MODBUS_ADDR_BRM_BATTERY_CHARGE_TIMES_0, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_PROPERTY: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.battery_property, op);
		}
		break;

		case MODBUS_ADDR_BRM_VIN_0:
		case MODBUS_ADDR_BRM_VIN_1:
		case MODBUS_ADDR_BRM_VIN_2:
		case MODBUS_ADDR_BRM_VIN_3:
		case MODBUS_ADDR_BRM_VIN_4:
		case MODBUS_ADDR_BRM_VIN_5:
		case MODBUS_ADDR_BRM_VIN_6:
		case MODBUS_ADDR_BRM_VIN_7:
		case MODBUS_ADDR_BRM_VIN_8: {
			modbus_data_value_copy(value, (uint16_t *)bms_info->settings->brm_data.vin, 17, addr - MODBUS_ADDR_BRM_VIN_0, op);
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_SERIAL: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.version.serial, op);
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_DAY: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.version.day, op);
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_MONTH: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.version.month, op);
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_YEAR: {
			modbus_data_value_rw(value, bms_info->settings->brm_data.version.year, op);
		}
		break;

		case MODBUS_ADDR_BCP_MAX_CHARGE_VOLTAGE_SINGLE_BATTERY: {
			modbus_data_value_rw(value, bms_info->settings->bcp_data.max_charge_voltage_single_battery, op);
		}
		break;

		case MODBUS_ADDR_BCP_MAX_CHARGE_CURRENT: {
			modbus_data_value_with_base_rw(value, bms_info->settings->bcp_data.max_charge_current, 4000, op);
		}
		break;

		case MODBUS_ADDR_BCP_RATE_TOTAL_POWER: {
			modbus_data_value_rw(value, bms_info->settings->bcp_data.rate_total_power, op);
		}
		break;

		case MODBUS_ADDR_BCP_MAX_TEMPERATURE: {
			modbus_data_value_with_offset_rw(value, bms_info->settings->bcp_data.max_temperature, -50, op);
		}
		break;

		case MODBUS_ADDR_BCP_TOTAL_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bcp_data.total_voltage, op);
		}
		break;

		case MODBUS_ADDR_BCP_TRANSFER_TYPE: {
			modbus_data_value_rw(value, bms_info->settings->bcp_data.transfer_type, op);
		}
		break;

		case MODBUS_ADDR_BRO_BRO_RESULT: {
			if(op == MODBUS_DATA_GET) {
				*value = (bms_info->settings->bro_data.bro_result == 0x00) ? 0x00 :
				         (bms_info->settings->bro_data.bro_result == 0xaa) ? 0x01 :
				         0x02;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bro_data.bro_result = (*value == 0x00) ? 0x00 :
				        (*value == 0x01) ? 0xaa :
				        0xff;
			}
		}
		break;

		case MODBUS_ADDR_BCL_REQUIRE_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bcl_data.require_voltage, op);
		}
		break;

		case MODBUS_ADDR_BCL_REQUIRE_CURRENT: {
			modbus_data_value_with_base_rw(value, bms_info->settings->bcl_data.require_current, 4000, op);
		}
		break;

		case MODBUS_ADDR_BCL_CHARGE_MODE: {
			modbus_data_value_with_offset_rw(value, bms_info->settings->bcl_data.charge_mode, -1, op);
		}
		break;

		case MODBUS_ADDR_BCS_CHARGE_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bcs_data.charge_voltage, op);
		}
		break;

		case MODBUS_ADDR_BCS_CHARGE_CURRENT: {
			modbus_data_value_with_base_rw(value, bms_info->settings->bcs_data.charge_current, 4000, op);
		}
		break;

		case MODBUS_ADDR_BCS_SINGLE_BATTERY_MAX_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bcs_data.u1.s.single_battery_max_voltage, op);
		}
		break;

		case MODBUS_ADDR_BCS_SINGLE_BATTERY_MAX_GROUP: {
			modbus_data_value_rw(value, bms_info->settings->bcs_data.u1.s.single_battery_max_group, op);
		}
		break;

		case MODBUS_ADDR_BCS_REMAIN_MIN: {
			modbus_data_value_rw(value, bms_info->settings->bcs_data.remain_min, op);
		}
		break;

		case MODBUS_ADDR_BSM_SINGLE_MAX_VOLTAGE_GROUP: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.single_max_voltage_group, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_MAX_TEMPERATURE: {
			modbus_data_value_with_offset_rw(value, bms_info->settings->bsm_data.battery_max_temperature, -50, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_MAX_TEMPERATURE_SN: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.battery_max_temperature_sn, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_MIN_TEMPERATURE: {
			modbus_data_value_with_offset_rw(value, bms_info->settings->bsm_data.battery_min_temperature, -50, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_MIN_TEMPERATURE_SN: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.battery_min_temperature_sn, op);
		}
		break;

		case MODBUS_ADDR_BSM_SINGLE_VOLTAGE_STATE: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.u1.s.single_voltage_state, op);
		}
		break;

		case MODBUS_ADDR_BSM_TOTAL_SOC_STATE: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.u1.s.total_soc_state, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_CURRENT_STATE: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.u1.s.battery_current_state, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_TEMPERATURE_STATE: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.u1.s.battery_temperature_state, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_INSULATION_STATE: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.u2.s.battery_insulation_state, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_CONNECTOR_STATE: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.u2.s.battery_connector_state, op);
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_CHARGE_ENABLE: {
			modbus_data_value_rw(value, bms_info->settings->bsm_data.u2.s.battery_charge_enable, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_REASON_SOC: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u1.s.stop_reason_soc, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_REASON_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u1.s.stop_reason_voltage, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_REASON_SINGLE_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u1.s.stop_reason_single_voltage, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_REASON_CHARGER_STOP: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u1.s.stop_reason_charger_stop, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_INSULATION: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u2.s.stop_fault_reason_insulation, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_CONNECTOR_TEMPERATURE: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u2.s.stop_fault_reason_connector_temperature, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_BMS_CONNECTOR_TEMPERATURE: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u2.s.stop_fault_reason_bms_connector_temperature, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_CHARGER_CONNECTOR: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u2.s.stop_fault_reason_bms_connector_temperature, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_BATTERY_TEMPERATURE: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u2.s.stop_fault_reason_battery_temperature, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_RELAY: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u2.s.stop_fault_reason_relay, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_VOLTAGE_CHECK: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u2.s.stop_fault_reason_voltage_check, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_OTHER: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u2.s.stop_fault_reason_other, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_ERROR_REASON_CURRENT: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u3.s.stop_error_reason_current, op);
		}
		break;

		case MODBUS_ADDR_BST_STOP_ERROR_REASON_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bst_data.u3.s.stop_error_reason_voltage, op);
		}
		break;

		case MODBUS_ADDR_BSD_SINGLE_MIN_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bsd_data.single_min_voltage, op);
		}
		break;

		case MODBUS_ADDR_BSD_SINGLE_MAX_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->bsd_data.single_max_voltage, op);
		}
		break;

		case MODBUS_ADDR_BSD_BATTERY_MIN_TEMPERATURE: {
			modbus_data_value_with_offset_rw(value, bms_info->settings->bsd_data.battery_min_temperature, -50, op);
		}
		break;

		case MODBUS_ADDR_BSD_BATTERY_MAX_TEMPERATURE: {
			modbus_data_value_with_offset_rw(value, bms_info->settings->bsd_data.battery_max_temperature, -50, op);
		}
		break;

		case MODBUS_ADDR_BEM_CRM_00_TIMEOUT: {
			modbus_data_value_rw(value, bms_info->settings->bem_data.u1.s.crm_00_timeout, op);
		}
		break;

		case MODBUS_ADDR_BEM_CRM_AA_TIMEOUT: {
			modbus_data_value_rw(value, bms_info->settings->bem_data.u1.s.crm_aa_timeout, op);
		}
		break;

		case MODBUS_ADDR_BEM_CTS_CML_TIMEOUT: {
			modbus_data_value_rw(value, bms_info->settings->bem_data.u2.s.cts_cml_timeout, op);
		}
		break;

		case MODBUS_ADDR_BEM_CRO_TIMEOUT: {
			modbus_data_value_rw(value, bms_info->settings->bem_data.u2.s.cro_timeout, op);
		}
		break;

		case MODBUS_ADDR_BEM_CCS_TIMEOUT: {
			modbus_data_value_rw(value, bms_info->settings->bem_data.u3.s.ccs_timeout, op);
		}
		break;

		case MODBUS_ADDR_BEM_CST_TIMEOUT: {
			modbus_data_value_rw(value, bms_info->settings->bem_data.u3.s.cst_timeout, op);
		}
		break;

		case MODBUS_ADDR_BEM_CSD_TIMEOUT: {
			modbus_data_value_rw(value, bms_info->settings->bem_data.u4.s.csd_timeout, op);
		}
		break;

		case MODBUS_ADDR_BEM_OTHER: {
			modbus_data_value_rw(value, bms_info->settings->bem_data.u4.s.other, op);
		}
		break;

		case MODBUS_ADDR_BMS_COMMON_MAX_CHARGE_VOLTAGE: {
			modbus_data_value_r(value, bms_info->settings->bhm_data.max_charge_voltage, op);
			modbus_data_value_w(value, bms_info->settings->bhm_data.max_charge_voltage, op);
			modbus_data_value_w(value, bms_info->settings->bcp_data.max_charge_voltage, op);
		}
		break;

		case MODBUS_ADDR_BMS_COMMON_SOC: {
			modbus_data_value_r(value, bms_info->settings->bcp_data.soc, op);
			modbus_data_value_w(value, bms_info->settings->bcp_data.soc, op);
			modbus_data_value_w(value / 10, bms_info->settings->bcs_data.soc, op);
			modbus_data_value_w(value / 10, bms_info->settings->bsd_data.soc, op);
		}
		break;

		case MODBUS_ADDR_CCS_OUTPUT_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->ccs_data.output_voltage, op);
		}
		break;

		case MODBUS_ADDR_CCS_OUTPUT_CURRENT: {
			modbus_data_value_with_base_rw(value, bms_info->settings->ccs_data.output_current, 4000, op);
		}
		break;

		case MODBUS_ADDR_CCS_TOTAL_CHARGE_TIME: {
			modbus_data_value_rw(value, bms_info->settings->ccs_data.total_charge_time, op);
		}
		break;

		case MODBUS_ADDR_CCS_CHARGE_ENABLE: {
			modbus_data_value_rw(value, bms_info->settings->ccs_data.u1.s.charge_enable, op);
		}
		break;

		case MODBUS_ADDR_CML_MAX_OUTPUT_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->cml_data.max_output_voltage, op);
		}
		break;

		case MODBUS_ADDR_CML_MIN_OUTPUT_VOLTAGE: {
			modbus_data_value_rw(value, bms_info->settings->cml_data.min_output_voltage, op);
		}
		break;

		case MODBUS_ADDR_CML_MAX_OUTPUT_CURRENT: {
			modbus_data_value_with_base_rw(value, bms_info->settings->cml_data.max_output_current, 4000, op);
		}
		break;

		case MODBUS_ADDR_CML_MIN_OUTPUT_CURRENT: {
			modbus_data_value_with_base_rw(value, bms_info->settings->cml_data.min_output_current, 4000, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BHM: {
			modbus_data_value_rw(value, bms_info->configs.disable_bhm, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BRM: {
			modbus_data_value_rw(value, bms_info->configs.disable_brm, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BCP: {
			modbus_data_value_rw(value, bms_info->configs.disable_bcp, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BRO: {
			modbus_data_value_rw(value, bms_info->configs.disable_bro, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BCL: {
			modbus_data_value_rw(value, bms_info->configs.disable_bcl, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BCS: {
			modbus_data_value_rw(value, bms_info->configs.disable_bcs, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BSM: {
			modbus_data_value_rw(value, bms_info->configs.disable_bsm, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BMV: {
			modbus_data_value_rw(value, bms_info->configs.disable_bmv, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BMT: {
			modbus_data_value_rw(value, bms_info->configs.disable_bmt, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BSP: {
			modbus_data_value_rw(value, bms_info->configs.disable_bsp, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BST: {
			modbus_data_value_rw(value, bms_info->configs.disable_bst, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BSD: {
			modbus_data_value_rw(value, bms_info->configs.disable_bsd, op);
		}
		break;

		case MODBUS_ADDR_DISABLE_BEM: {
			modbus_data_value_rw(value, bms_info->configs.disable_bem, op);
		}
		break;

		case MODBUS_ADDR_STOP_BMS: {
			modbus_data_value_rw(value, bms_info->configs.stop_bms, op);

			if(op == MODBUS_DATA_SET) {
				if(bms_info->configs.stop_bms != 0) {
					bms_info->configs.stop_bms = 0;

					if(bms_info->state != BMS_STATE_BCL_BCS_BSM_BMV_BMT_BSP) {
						return;
					}

					bms_info->settings->bst_data.u1.s.stop_reason_soc = 1;
					set_bms_state_locked(bms_info, BMS_STATE_BST);
				}
			}
		}
		break;

		case MODBUS_ADDR_RESET_BMS_CONFIGURE: {
			modbus_data_value_rw(value, bms_info->configs.reset_bms_configure, op);

			if(op == MODBUS_DATA_SET) {
				if(bms_info->configs.reset_bms_configure != 0) {
					memset(&bms_info->configs, 0, sizeof(bms_data_configs_t));
					bms_data_settings_default_init(bms_info->settings);
				}
			}
		}
		break;

		case MODBUS_ADDR_TOGGLE_GUN_ON_OFF: {
			modbus_data_value_rw(value, bms_info->configs.toggle_gun_on_off, op);

			if(op == MODBUS_DATA_SET) {
				if(bms_info->configs.toggle_gun_on_off != 0) {
					uint8_t on_off = get_gun_on_off(bms_info);

					bms_info->configs.toggle_gun_on_off = 0;

					if(bms_info->state != BMS_STATE_IDLE) {
						return;
					}

					if(on_off == 0) {
						on_off = 1;
					} else {
						on_off = 0;
					}

					set_gun_on_off(bms_info, on_off);
				}
			}
		}
		break;

		case MODBUS_ADDR_BMS_STATE: {
			modbus_data_value_r(value, bms_info->state, op);
		}
		break;

		case MODBUS_ADDR_BMS_GUN_CONNECT: {
			modbus_data_value_r(value, bms_info->bms_gun_connect, op);
		}
		break;

		case MODBUS_ADDR_BMS_POWERON_ENABLE: {
			modbus_data_value_r(value, bms_info->bms_poweron_enable, op);
		}
		break;

		case MODBUS_ADDR_GUN_ON_OFF_STATE: {
			modbus_data_value_r(value, get_gun_on_off(bms_info), op);
		}
		break;

		case MODBUS_ADDR_VERSION_MAJOR: {
			modbus_data_value_r(value, VER_MAJOR, op);
		}
		break;

		case MODBUS_ADDR_VERSION_MINOR: {
			modbus_data_value_r(value, VER_MINOR, op);
		}
		break;

		case MODBUS_ADDR_VERSION_REV: {
			modbus_data_value_r(value, VER_REV, op);
		}
		break;

		default:
			_printf("error! op:%s, addr:%d\n",
			               (op == MODBUS_DATA_GET) ? "get" :
			               (op == MODBUS_DATA_SET) ? "set" :
			               "unknow",
			               addr);
			break;
	}
}

static uint16_t modbus_data_get(void *ctx, uint16_t addr)
{
	uint16_t value = 0;
	bms_info_t *bms_info = (bms_info_t *)ctx;

	modbus_data_get_set(bms_info, addr, &value, MODBUS_DATA_GET);

	return value;
}

static void modbus_data_set(void *ctx, uint16_t addr, uint16_t value)
{
	bms_info_t *bms_info = (bms_info_t *)ctx;

	modbus_data_get_set(bms_info, addr, &value, MODBUS_DATA_SET);
}

static void bms_set_modbus_slave_info(bms_info_t *bms_info, modbus_slave_info_t *modbus_slave_info)
{
	bms_info->modbus_slave_info = modbus_slave_info;

	bms_info->modbus_slave_data_info.ctx = bms_info;
	bms_info->modbus_slave_data_info.valid = modbus_addr_valid;
	bms_info->modbus_slave_data_info.get = modbus_data_get;
	bms_info->modbus_slave_data_info.set = modbus_data_set;
	set_modbus_slave_data_info(bms_info->modbus_slave_info, &bms_info->modbus_slave_data_info);

	bms_info->modbus_slave_data_changed_cb.fn = modbus_slave_data_changed;
	bms_info->modbus_slave_data_changed_cb.fn_ctx = bms_info;
	add_modbus_slave_data_changed_cb(bms_info->modbus_slave_info, &bms_info->modbus_slave_data_changed_cb);
}

static int bms_info_set_bms_info_config(bms_info_t *bms_info, bms_info_config_t *bms_info_config)
{
	int ret = -1;
	can_info_t *can_info;
	eeprom_info_t *eeprom_info;
	modbus_slave_info_t *modbus_slave_info;
	osThreadDef(task_modbus_slave, task_modbus_slave, osPriorityNormal, 0, 128 * 3);

	bms_info->bms_info_config = bms_info_config;

	can_info = get_or_alloc_can_info(bms_info_config->hcan);

	if(can_info == NULL) {
		return ret;
	}

	bms_info->can_info = can_info;

	eeprom_info = get_or_alloc_eeprom_info(bms_info_config->hspi,
	                                       bms_info_config->gpio_port_spi_cs,
	                                       bms_info_config->gpio_pin_spi_cs,
	                                       bms_info_config->gpio_port_spi_wp,
	                                       bms_info_config->gpio_pin_spi_wp);

	if(eeprom_info == NULL) {
		return ret;
	}

	bms_info->eeprom_info = eeprom_info;

	modbus_slave_info = get_or_alloc_modbus_slave_info(bms_info_config->huart);

	if(modbus_slave_info == NULL) {
		return ret;
	}

	bms_set_modbus_slave_info(bms_info, modbus_slave_info);

	osThreadCreate(osThread(task_modbus_slave), modbus_slave_info);

	ret = 0;
	return ret;
}

bms_info_t *get_or_alloc_bms_info(bms_info_config_t *bms_info_config)
{
	int index = -1;
	osStatus os_status;
	bms_info_t *bms_info = NULL;
	osMutexDef(handle_mutex);

	bms_info = get_bms_info(bms_info_config);

	if(bms_info != NULL) {
		return bms_info;
	}

	if(eeprom_modbus_data_bitmap == NULL) {
		eeprom_modbus_data_bitmap = alloc_bitmap(2);
	}

	if(eeprom_modbus_data_bitmap == NULL) {
		return bms_info;
	}

	if(bms_info_list_mutex == NULL) {
		osMutexDef(bms_info_list_mutex);
		bms_info_list_mutex = osMutexCreate(osMutex(bms_info_list_mutex));

		if(bms_info_list_mutex == NULL) {
			return bms_info;
		}
	}

	bms_info = (bms_info_t *)os_alloc(sizeof(bms_info_t));

	if(bms_info == NULL) {
		return bms_info;
	}

	memset(bms_info, 0, sizeof(bms_info_t));

	index = get_first_value_index(eeprom_modbus_data_bitmap, 0);

	if(index == -1) {
		goto failed;
	}

	set_bitmap_value(eeprom_modbus_data_bitmap, index, 1);

	bms_info->eeprom_modbus_data_index = index;

	bms_info->settings = bms_data_alloc_settings();

	if(bms_info->settings == NULL) {
		goto failed;
	}

	bms_info->state = BMS_STATE_IDLE;
	bms_info->handle_mutex = osMutexCreate(osMutex(handle_mutex));
	bms_info->bms_info_config = bms_info_config;

	bms_info->gun_on_off_state = 0;
	bms_info->bms_gun_connect = 0;
	bms_info->bms_poweron_enable = 0;

	memset(&bms_info->configs, 0, sizeof(bms_data_configs_t));

	bms_info->modbus_slave_info = NULL;

	set_gun_on_off(bms_info, 0);

	os_status = osMutexWait(bms_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&bms_info->list, &bms_info_list);

	os_status = osMutexRelease(bms_info_list_mutex);

	if(os_status != osOK) {
	}

	if(bms_info_set_bms_info_config(bms_info, bms_info_config) != 0) {
		goto failed;
	}

	return bms_info;

failed:

	free_bms_info(bms_info);
	bms_info = NULL;

	return bms_info;
}

int load_eeprom_modbus_data(bms_info_t *bms_info)
{
	int ret = -1;
	uint32_t offset;
	uint32_t crc = 0;
	eeprom_modbus_head_t eeprom_modbus_head;

	if(bms_info == NULL) {
		return ret;
	}

	offset = sizeof(eeprom_modbus_data_t) * bms_info->eeprom_modbus_data_index;

	if(detect_eeprom(bms_info->eeprom_info) != 0) {
		_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	eeprom_read(bms_info->eeprom_info, offset, (uint8_t *)&eeprom_modbus_head, sizeof(eeprom_modbus_head_t));
	offset += sizeof(eeprom_modbus_head_t);

	if(eeprom_modbus_head.payload_size != sizeof(eeprom_modbus_data_t)) {
		_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	crc += eeprom_modbus_head.payload_size;
	crc += (uint32_t)'b';
	crc += (uint32_t)'m';
	crc += (uint32_t)'s';

	if(crc != eeprom_modbus_head.crc) {
		_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	ret = 0;

	eeprom_read(bms_info->eeprom_info, offset, (uint8_t *)bms_info->settings, sizeof(bms_data_settings_t));
	offset += sizeof(bms_data_settings_t);

	eeprom_read(bms_info->eeprom_info, offset, (uint8_t *)&bms_info->configs, sizeof(bms_data_configs_t));
	offset += sizeof(bms_data_configs_t);

	return ret;
}

void bms_restore_data(bms_info_t *bms_info)
{
	int ret = -1;

	if(bms_info == NULL) {
		return;
	}

	ret = load_eeprom_modbus_data(bms_info);
	_printf("load_eeprom_modbus_data %s!\n", (ret == 0) ? "successfully" : "failed");

	reset_bms_data_settings_charger_data(bms_info);

	if(ret != 0) {
		bms_data_settings_default_init(bms_info->settings);
		memset(&bms_info->configs, 0, sizeof(bms_data_configs_t));
		save_eeprom_modbus_data(bms_info);
	}

	bms_info->bms_gun_connect = is_gun_connected(bms_info);
	bms_info->bms_poweron_enable = is_bms_poweron_enable(bms_info);
}

bms_state_t get_bms_state(bms_info_t *bms_info)
{
	return bms_info->state;
}

char *get_bms_state_des(bms_state_t state)
{
	char *des = NULL;

	switch(state) {
		case BMS_STATE_IDLE: {
			des = "BMS_STATE_IDLE";
		}
		break;

		case BMS_STATE_BHM: {
			des = "BMS_STATE_BHM";
		}
		break;

		case BMS_STATE_BRM: {
			des = "BMS_STATE_BRM";
		}
		break;

		case BMS_STATE_BCP: {
			des = "BMS_STATE_BCP";
		}
		break;

		case BMS_STATE_BRO: {
			des = "BMS_STATE_BRO";
		}
		break;

		case BMS_STATE_BCL_BCS_BSM_BMV_BMT_BSP: {
			des = "BMS_STATE_BCL_BCS_BSM_BMV_BMT_BSP";
		}
		break;

		case BMS_STATE_BST: {
			des = "BMS_STATE_BST";
		}
		break;

		case BMS_STATE_BSD_BEM: {
			des = "BMS_STATE_BSD_BEM";
		}
		break;

		default: {
			des = "unknow state";
		}
		break;
	}

	return des;
}

void set_bms_state(bms_info_t *bms_info, bms_state_t state)
{
	bms_state_handler_t *handler = NULL;

	if(is_gun_connected(bms_info) == 0) {
		state = BMS_STATE_IDLE;
	} else if(is_bms_poweron_enable(bms_info) == 0) {
		state = BMS_STATE_IDLE;
	}

	if(state == bms_info->state) {
		return;
	}

	if(state == BMS_STATE_IDLE) {
		reset_bms_data_settings_charger_data(bms_info);
	}

	handler = bms_get_state_handler(state);

	if((handler != NULL) && (handler->prepare != NULL)) {
		handler->prepare(bms_info);
	}

	_printf("change to state:%s!\n", get_bms_state_des(state));

	bms_info->state = state;
}

void set_bms_state_locked(bms_info_t *bms_info, bms_state_t state)
{
	osStatus os_status;

	if(bms_info->handle_mutex) {
		os_status = osMutexWait(bms_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	set_bms_state(bms_info, state);

	if(bms_info->handle_mutex) {
		os_status = osMutexRelease(bms_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

void bms_handle_request(bms_info_t *bms_info)
{
	bms_state_handler_t *handler = bms_get_state_handler(bms_info->state);
	osStatus os_status;
	int ret = 0;

	if(handler == NULL) {
		return;
	}

	if(bms_info->handle_mutex) {
		os_status = osMutexWait(bms_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	ret = handler->handle_request(bms_info);

	if(ret != 0) {
	}

	if(bms_info->handle_mutex) {
		os_status = osMutexRelease(bms_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

void bms_handle_response(bms_info_t *bms_info)
{
	bms_state_handler_t *handler = bms_get_state_handler(bms_info->state);
	osStatus os_status;
	int ret = 0;

	if(handler == NULL) {
		return;
	}

	if(bms_info->handle_mutex) {
		os_status = osMutexWait(bms_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	ret = handler->handle_response(bms_info);

	if(ret == 0) {
	}

	if(bms_info->handle_mutex) {
		os_status = osMutexRelease(bms_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

uint8_t is_gun_connected(bms_info_t *bms_info)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(bms_info->bms_info_config->gpio_port_gun_connect_state,
	                                       bms_info->bms_info_config->gpio_pin_gun_connect_state);

	if(state == GPIO_PIN_RESET) {
		//return 0;
		return 1;
	} else {
		return 1;
	}
}

uint8_t is_bms_poweron_enable(bms_info_t *bms_info)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(bms_info->bms_info_config->gpio_port_bms_power_enable_state,
	                                       bms_info->bms_info_config->gpio_pin_bms_power_enable_state);

	if(state == GPIO_PIN_RESET) {
		//return 0;
		return 1;
	} else {
		return 1;
	}
}

static void update_ui_data(bms_info_t *bms_info)
{
	bms_info->bms_gun_connect = is_gun_connected(bms_info);
	bms_info->bms_poweron_enable = is_bms_poweron_enable(bms_info);
}

void bms_periodic(bms_info_t *bms_info)
{
	update_ui_data(bms_info);
}
