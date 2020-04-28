

/*================================================================
 *
 *
 *   文件名称：bms.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分52秒
 *   修改日期：2020年04月28日 星期二 09时04分42秒
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

#include "app.h"

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

static bms_info_config_t *bms_info_config_sz[] = {
	&bms_info_config_can1,
	&bms_info_config_can2,
};

static bms_info_config_t *get_bms_info_config(can_info_t *can_info)
{
	int i;
	bms_info_config_t *bms_info_config = NULL;
	bms_info_config_t *bms_info_config_item = NULL;

	for(i = 0; i < sizeof(bms_info_config_sz) / sizeof(bms_info_config_t *); i++) {
		bms_info_config_item = bms_info_config_sz[i];

		if(can_info->hcan == bms_info_config_item->hcan) {
			bms_info_config = bms_info_config_item;
			break;
		}
	}

	if(bms_info_config->get_gun_connect_state == NULL) {
		app_panic();
	}

	if(bms_info_config->get_bms_power_enable_state == NULL) {
		app_panic();
	}

	if(bms_info_config->set_gun_on_off_state == NULL) {
		app_panic();
	}

	return bms_info_config;
}

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

static bms_info_t *get_bms_info(can_info_t *can_info)
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
		if(bms_info_item->can_info == can_info) {
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

	bms_info->bms_info_config->set_gun_on_off_state(on_off);
}

bms_info_t *get_or_alloc_bms_info(can_info_t *can_info)
{
	bms_info_t *bms_info = NULL;
	int index = -1;
	bms_info_config_t *bms_info_config = get_bms_info_config(can_info);
	osStatus os_status;

	osMutexDef(handle_mutex);

	if(bms_info_config == NULL) {
		return bms_info;
	}

	bms_info = get_bms_info(can_info);

	if(bms_info != NULL) {
		return bms_info;
	}

	if(bms_info_list_mutex == NULL) {
		osMutexDef(bms_info_list_mutex);
		bms_info_list_mutex = osMutexCreate(osMutex(bms_info_list_mutex));

		if(bms_info_list_mutex == NULL) {
			return bms_info;
		}
	}

	if(eeprom_modbus_data_bitmap == NULL) {
		eeprom_modbus_data_bitmap = alloc_bitmap(2);
	}

	if(eeprom_modbus_data_bitmap == NULL) {
		return bms_info;
	}

	bms_info = (bms_info_t *)os_alloc(sizeof(bms_info_t));

	if(bms_info == NULL) {
		return bms_info;
	}

	memset(bms_info, 0, sizeof(bms_info_t));

	index = get_first_value_index(eeprom_modbus_data_bitmap, 0);

	if(index == -1) {
		return bms_info;
	}

	set_bitmap_value(eeprom_modbus_data_bitmap, index, 1);

	bms_info->eeprom_modbus_data_index = index;

	bms_info->settings = bms_data_alloc_settings();

	if(bms_info->settings == NULL) {
		goto failed;
	}

	bms_info->can_info = can_info;
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


	return bms_info;

failed:

	if(bms_info != NULL) {
		os_free(bms_info);
		bms_info = NULL;
	}

	return bms_info;
}

static void modbus_slave_data_changed(void *fn_ctx, void *chain_ctx)
{
	//udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
	save_eeprom_modbus_data((bms_info_t *)fn_ctx);
}

static uint8_t modbus_addr_valid(void *ctx, uint16_t start, uint16_t number)
{
	uint8_t valid = 0;
	uint16_t end = start + number;//无效边界

	if(end <= start) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	if(start < 0) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	if(end > MODBUS_ADDR_INVALID) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return valid;
	}

	valid = 1;

	return valid;
}

static void modbus_data_value_copy(uint16_t *value, uint16_t *store, uint16_t size, uint16_t offset, modbus_data_op_t op)
{
	uint16_t copy_size = size - sizeof(uint16_t) * offset;
	uint16_t *from = NULL;
	uint16_t *to = NULL;

	if(offset * sizeof(uint16_t) >= size) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return;
	}

	if(copy_size > sizeof(uint16_t)) {
		copy_size = sizeof(uint16_t);
	}

	if(op == MODBUS_DATA_GET) {
		from = store + offset;
		to = value;
	} else if(op == MODBUS_DATA_SET) {
		from = value;
		to = store + offset;
	}

	if(from == NULL) {
		return;
	}

	if(to == NULL) {
		return;
	}

	memcpy(to, from, copy_size);
}

static void modbus_data_get_set(bms_info_t *bms_info, uint16_t addr, uint16_t *value, modbus_data_op_t op)
{
	switch(addr) {
		case MODBUS_ADDR_BRM_VERSION_1: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.brm_data.version_1;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.brm_data.version_1 = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_0: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.brm_data.version_0;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.brm_data.version_0 = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_TYPE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.brm_data.battery_type;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.brm_data.battery_type = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_TOTAL_BATTERY_RATE_CAPICITY: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.brm_data.total_battery_rate_capicity;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.brm_data.total_battery_rate_capicity = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_TOTAL_BATTERY_RATE_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.brm_data.total_battery_rate_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.brm_data.total_battery_rate_voltage = *value;
			}
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
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.battery_year + 1985;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.battery_year = *value - 1985;
			}
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_MONTH: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.battery_month;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.battery_month = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_DAY: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.battery_day;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.battery_day = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_CHARGE_TIMES_0:
		case MODBUS_ADDR_BRM_BATTERY_CHARGE_TIMES_1: {
			modbus_data_value_copy(value, (uint16_t *)bms_info->settings->brm_data.battery_charge_times, 3, addr - MODBUS_ADDR_BRM_BATTERY_CHARGE_TIMES_0, op);
		}
		break;

		case MODBUS_ADDR_BRM_BATTERY_PROPERTY: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.battery_property;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.battery_property = *value;
			}
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
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.version.serial;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.version.serial = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_DAY: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.version.day;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.version.day = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_MONTH: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.version.month;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.version.month = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRM_VERSION_YEAR: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->brm_data.version.year;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->brm_data.version.year = *value;
			}
		}
		break;

		case MODBUS_ADDR_BCP_MAX_CHARGE_VOLTAGE_SINGLE_BATTERY: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcp_data.max_charge_voltage_single_battery;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcp_data.max_charge_voltage_single_battery = *value;
			}
		}
		break;

		case MODBUS_ADDR_BCP_MAX_CHARGE_CURRENT: {
			if(op == MODBUS_DATA_GET) {
				*value = 4000 - bms_info->settings->bcp_data.max_charge_current;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcp_data.max_charge_current = 4000 - *value;
			}
		}
		break;

		case MODBUS_ADDR_BCP_RATE_TOTAL_POWER: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcp_data.rate_total_power;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcp_data.rate_total_power = *value;
			}
		}
		break;

		case MODBUS_ADDR_BCP_MAX_TEMPERATURE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcp_data.max_temperature - 50;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcp_data.max_temperature = *value + 50;
			}
		}
		break;

		case MODBUS_ADDR_BCP_TOTAL_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcp_data.total_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcp_data.total_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BCP_TRANSFER_TYPE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcp_data.transfer_type;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcp_data.transfer_type = *value;
			}
		}
		break;

		case MODBUS_ADDR_BRO_BRO_RESULT: {
			if(op == MODBUS_DATA_GET) {
				*value = (bms_info->settings->bro_data.bro_result == 0x00) ? 0x00 : (bms_info->settings->bro_data.bro_result == 0xaa) ? 0x01 : 0x02;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bro_data.bro_result = (*value == 0x00) ? 0x00 : (*value == 0x01) ? 0xaa : 0xff;
			}
		}
		break;

		case MODBUS_ADDR_BCL_REQUIRE_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcl_data.require_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcl_data.require_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BCL_REQUIRE_CURRENT: {
			if(op == MODBUS_DATA_GET) {
				*value = 4000 - bms_info->settings->bcl_data.require_current;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcl_data.require_current = 4000 - *value;
			}
		}
		break;

		case MODBUS_ADDR_BCL_CHARGE_MODE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcl_data.charge_mode - 1;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcl_data.charge_mode = *value + 1;
			}
		}
		break;

		case MODBUS_ADDR_BCS_CHARGE_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcs_data.charge_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcs_data.charge_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BCS_CHARGE_CURRENT: {
			if(op == MODBUS_DATA_GET) {
				*value = 4000 - bms_info->settings->bcs_data.charge_current;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcs_data.charge_current = 4000 - *value;
			}
		}
		break;

		case MODBUS_ADDR_BCS_SINGLE_BATTERY_MAX_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcs_data.u1.s.single_battery_max_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcs_data.u1.s.single_battery_max_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BCS_SINGLE_BATTERY_MAX_GROUP: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcs_data.u1.s.single_battery_max_group;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcs_data.u1.s.single_battery_max_group = *value;
			}
		}
		break;

		case MODBUS_ADDR_BCS_REMAIN_MIN: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcs_data.remain_min;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcs_data.remain_min = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_SINGLE_MAX_VOLTAGE_GROUP: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.single_max_voltage_group;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.single_max_voltage_group = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_MAX_TEMPERATURE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.battery_max_temperature - 50;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.battery_max_temperature = *value + 50;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_MAX_TEMPERATURE_SN: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.battery_max_temperature_sn;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.battery_max_temperature_sn = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_MIN_TEMPERATURE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.battery_min_temperature - 50;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.battery_min_temperature = *value + 50;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_MIN_TEMPERATURE_SN: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.battery_min_temperature_sn;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.battery_min_temperature_sn = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_SINGLE_VOLTAGE_STATE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.u1.s.single_voltage_state;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.u1.s.single_voltage_state = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_TOTAL_SOC_STATE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.u1.s.total_soc_state;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.u1.s.total_soc_state = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_CURRENT_STATE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.u1.s.battery_current_state;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.u1.s.battery_current_state = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_TEMPERATURE_STATE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.u1.s.battery_temperature_state;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.u1.s.battery_temperature_state = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_INSULATION_STATE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.u2.s.battery_insulation_state;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.u2.s.battery_insulation_state = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_CONNECTOR_STATE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.u2.s.battery_connector_state;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.u2.s.battery_connector_state = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSM_BATTERY_CHARGE_ENABLE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsm_data.u2.s.battery_charge_enable;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsm_data.u2.s.battery_charge_enable = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_REASON_SOC: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u1.s.stop_reason_soc;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u1.s.stop_reason_soc = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_REASON_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u1.s.stop_reason_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u1.s.stop_reason_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_REASON_SINGLE_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u1.s.stop_reason_single_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u1.s.stop_reason_single_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_REASON_CHARGER_STOP: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u1.s.stop_reason_charger_stop;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u1.s.stop_reason_charger_stop = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_INSULATION: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u2.s.stop_fault_reason_insulation;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u2.s.stop_fault_reason_insulation = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_CONNECTOR_TEMPERATURE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u2.s.stop_fault_reason_connector_temperature;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u2.s.stop_fault_reason_connector_temperature = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_BMS_CONNECTOR_TEMPERATURE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u2.s.stop_fault_reason_bms_connector_temperature;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u2.s.stop_fault_reason_bms_connector_temperature = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_CHARGER_CONNECTOR: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u2.s.stop_fault_reason_bms_connector_temperature;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u2.s.stop_fault_reason_bms_connector_temperature = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_BATTERY_TEMPERATURE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u2.s.stop_fault_reason_battery_temperature;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u2.s.stop_fault_reason_battery_temperature = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_RELAY: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u2.s.stop_fault_reason_relay;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u2.s.stop_fault_reason_relay = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_VOLTAGE_CHECK: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u2.s.stop_fault_reason_voltage_check;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u2.s.stop_fault_reason_voltage_check = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_FAULT_REASON_OTHER: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u2.s.stop_fault_reason_other;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u2.s.stop_fault_reason_other = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_ERROR_REASON_CURRENT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u3.s.stop_error_reason_current;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u3.s.stop_error_reason_current = *value;
			}
		}
		break;

		case MODBUS_ADDR_BST_STOP_ERROR_REASON_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bst_data.u3.s.stop_error_reason_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bst_data.u3.s.stop_error_reason_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSD_SINGLE_MIN_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsd_data.single_min_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsd_data.single_min_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSD_SINGLE_MAX_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsd_data.single_max_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsd_data.single_max_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BSD_BATTERY_MIN_TEMPERATURE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsd_data.battery_min_temperature - 50;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsd_data.battery_min_temperature = *value + 50;
			}
		}
		break;

		case MODBUS_ADDR_BSD_BATTERY_MAX_TEMPERATURE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bsd_data.battery_max_temperature - 50;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bsd_data.battery_max_temperature = *value + 50;
			}
		}
		break;

		case MODBUS_ADDR_BEM_CRM_00_TIMEOUT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bem_data.u1.s.crm_00_timeout;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bem_data.u1.s.crm_00_timeout = *value;
			}
		}
		break;

		case MODBUS_ADDR_BEM_CRM_AA_TIMEOUT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bem_data.u1.s.crm_aa_timeout;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bem_data.u1.s.crm_aa_timeout = *value;
			}
		}
		break;

		case MODBUS_ADDR_BEM_CTS_CML_TIMEOUT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bem_data.u2.s.cts_cml_timeout;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bem_data.u2.s.cts_cml_timeout = *value;
			}
		}
		break;

		case MODBUS_ADDR_BEM_CRO_TIMEOUT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bem_data.u2.s.cro_timeout;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bem_data.u2.s.cro_timeout = *value;
			}
		}
		break;

		case MODBUS_ADDR_BEM_CCS_TIMEOUT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bem_data.u3.s.ccs_timeout;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bem_data.u3.s.ccs_timeout = *value;
			}
		}
		break;

		case MODBUS_ADDR_BEM_CST_TIMEOUT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bem_data.u3.s.cst_timeout;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bem_data.u3.s.cst_timeout = *value;
			}
		}
		break;

		case MODBUS_ADDR_BEM_CSD_TIMEOUT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bem_data.u4.s.csd_timeout;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bem_data.u4.s.csd_timeout = *value;
			}
		}
		break;

		case MODBUS_ADDR_BEM_OTHER: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bem_data.u4.s.other;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bem_data.u4.s.other = *value;
			}
		}
		break;

		case MODBUS_ADDR_BMS_COMMON_MAX_CHARGE_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bhm_data.max_charge_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bhm_data.max_charge_voltage = *value;
				bms_info->settings->bcp_data.max_charge_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_BMS_COMMON_SOC: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->bcp_data.soc;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->bcp_data.soc = *value;
				bms_info->settings->bcs_data.soc = *value / 10;
				bms_info->settings->bsd_data.soc = *value / 10;
			}
		}
		break;

		case MODBUS_ADDR_CCS_OUTPUT_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->ccs_data.output_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->ccs_data.output_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_CCS_OUTPUT_CURRENT: {
			if(op == MODBUS_DATA_GET) {
				*value = 4000 - bms_info->settings->ccs_data.output_current;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->ccs_data.output_current = 4000 - *value;
			}
		}
		break;

		case MODBUS_ADDR_CCS_TOTAL_CHARGE_TIME: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->ccs_data.total_charge_time;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->ccs_data.total_charge_time = *value;
			}
		}
		break;

		case MODBUS_ADDR_CCS_CHARGE_ENABLE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->ccs_data.u1.s.charge_enable;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->ccs_data.u1.s.charge_enable = *value;
			}
		}
		break;

		case MODBUS_ADDR_CML_MAX_OUTPUT_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->cml_data.max_output_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->cml_data.max_output_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_CML_MIN_OUTPUT_VOLTAGE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->settings->cml_data.min_output_voltage;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->cml_data.min_output_voltage = *value;
			}
		}
		break;

		case MODBUS_ADDR_CML_MAX_OUTPUT_CURRENT: {
			if(op == MODBUS_DATA_GET) {
				*value = 4000 - bms_info->settings->cml_data.max_output_current;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->cml_data.max_output_current = 4000 - *value;
			}
		}
		break;

		case MODBUS_ADDR_CML_MIN_OUTPUT_CURRENT: {
			if(op == MODBUS_DATA_GET) {
				*value = 4000 - bms_info->settings->cml_data.min_output_current;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->settings->cml_data.min_output_current = 4000 - *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BHM: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bhm;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bhm = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BRM: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_brm;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_brm = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BCP: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bcp;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bcp = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BRO: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bro;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bro = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BCL: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bcl;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bcl = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BCS: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bcs;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bcs = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BSM: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bsm;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bsm = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BMV: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bmv;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bmv = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BMT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bmt;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bmt = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BSP: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bsp;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bsp = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BST: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bst;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bst = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BSD: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bsd;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bsd = *value;
			}
		}
		break;

		case MODBUS_ADDR_DISABLE_BEM: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.disable_bem;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.disable_bem = *value;
			}
		}
		break;

		case MODBUS_ADDR_STOP_BMS: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.stop_bms;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.stop_bms = *value;

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
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.reset_bms_configure;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.reset_bms_configure = *value;

				if(bms_info->configs.reset_bms_configure != 0) {
					bms_data_settings_default_init(bms_info->settings);
					memset(&bms_info->configs, 0, sizeof(bms_data_configs_t));
				}
			}
		}
		break;

		case MODBUS_ADDR_TOGGLE_GUN_ON_OFF: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->configs.toggle_gun_on_off;
			} else if(op == MODBUS_DATA_SET) {
				bms_info->configs.toggle_gun_on_off = *value;

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
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->state;
			} else if(op == MODBUS_DATA_SET) {
			}
		}
		break;

		case MODBUS_ADDR_BMS_GUN_CONNECT: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->bms_gun_connect;
			} else if(op == MODBUS_DATA_SET) {
			}
		}
		break;

		case MODBUS_ADDR_BMS_POWERON_ENABLE: {
			if(op == MODBUS_DATA_GET) {
				*value = bms_info->bms_poweron_enable;
			} else if(op == MODBUS_DATA_SET) {
			}
		}
		break;

		case MODBUS_ADDR_GUN_ON_OFF_STATE: {
			if(op == MODBUS_DATA_GET) {
				*value = get_gun_on_off(bms_info);
			} else if(op == MODBUS_DATA_SET) {
			}
		}
		break;

		case MODBUS_ADDR_VERSION_MAJOR: {
			if(op == MODBUS_DATA_GET) {
				*value = VER_MAJOR;
			} else if(op == MODBUS_DATA_SET) {
			}
		}
		break;

		case MODBUS_ADDR_VERSION_MINOR: {
			if(op == MODBUS_DATA_GET) {
				*value = VER_MINOR;
			} else if(op == MODBUS_DATA_SET) {
			}
		}
		break;

		case MODBUS_ADDR_VERSION_REV: {
			if(op == MODBUS_DATA_GET) {
				*value = VER_REV;
			} else if(op == MODBUS_DATA_SET) {
			}
		}
		break;

		default:
			udp_log_printf("error! op:%s, addr:%d\n",
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

void bms_set_modbus_slave_info(bms_info_t *bms_info, modbus_slave_info_t *modbus_slave_info)
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

void bms_set_eeprom_info(bms_info_t *bms_info, eeprom_info_t *eeprom_info)
{
	bms_info->eeprom_info = eeprom_info;
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
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	eeprom_read(bms_info->eeprom_info, offset, (uint8_t *)&eeprom_modbus_head, sizeof(eeprom_modbus_head_t));
	offset += sizeof(eeprom_modbus_head_t);

	if(eeprom_modbus_head.payload_size != sizeof(eeprom_modbus_data_t)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	crc += eeprom_modbus_head.payload_size;
	crc += (uint32_t)'b';
	crc += (uint32_t)'m';
	crc += (uint32_t)'s';

	if(crc != eeprom_modbus_head.crc) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	ret = 0;

	eeprom_read(bms_info->eeprom_info, offset, (uint8_t *)bms_info->settings, sizeof(bms_data_settings_t));
	offset += sizeof(bms_data_settings_t);

	eeprom_read(bms_info->eeprom_info, offset, (uint8_t *)&bms_info->configs, sizeof(bms_data_configs_t));
	offset += sizeof(bms_data_configs_t);

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
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
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

void bms_restore_data(bms_info_t *bms_info)
{
	int ret = -1;

	if(bms_info == NULL) {
		return;
	}

	ret = load_eeprom_modbus_data(bms_info);
	udp_log_printf("load_eeprom_modbus_data %s!\n", (ret == 0) ? "successfully" : "failed");

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

	udp_log_printf("change to state:%s!\n", get_bms_state_des(state));

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
	return bms_info->bms_info_config->get_gun_connect_state();
}

uint8_t is_bms_poweron_enable(bms_info_t *bms_info)
{
	return bms_info->bms_info_config->get_bms_power_enable_state();
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
