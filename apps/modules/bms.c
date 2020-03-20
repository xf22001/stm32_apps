

/*================================================================
 *
 *
 *   文件名称：bms.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分52秒
 *   修改日期：2020年03月20日 星期五 10时33分24秒
 *   描    述：
 *
 *================================================================*/
#include "bms.h"
#include "bms_handler.h"

#include "os_utils.h"
#include "bitmap_ops.h"
#include "main.h"
#include <string.h>
#define UDP_LOG
#include "task_probe_tool.h"

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

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

static LIST_HEAD(bms_info_list);

static bitmap_t *eeprom_modbus_data_bitmap = NULL;

static callback_item_t modbus_data_changed_cb;

typedef struct {
	CAN_HandleTypeDef *hcan;
	GPIO_TypeDef *gun_connect_gpio;
	uint16_t gun_connect_pin;
	GPIO_TypeDef *bms_poweron_enable_gpio;
	uint16_t bms_poweron_enable_pin;
	GPIO_TypeDef *gun_on_off_gpio;
	uint16_t gun_on_off_pin;
} bms_info_config_t;

bms_info_config_t bms_info_config_sz[] = {
	{
		.hcan = &hcan1,
		.gun_connect_gpio = in_a_cc1_GPIO_Port,
		.gun_connect_pin = in_a_cc1_Pin,
		.bms_poweron_enable_gpio = in_7_GPIO_Port,
		.bms_poweron_enable_pin = in_7_Pin,
		.gun_on_off_gpio = relay_8_GPIO_Port,
		.gun_on_off_pin = relay_8_Pin,
	},
	{
		.hcan = &hcan2,
		.gun_connect_gpio = in_a_cc1_GPIO_Port,
		.gun_connect_pin = in_a_cc1_Pin,
		.bms_poweron_enable_gpio = in_7_GPIO_Port,
		.bms_poweron_enable_pin = in_7_Pin,
		.gun_on_off_gpio = relay_8_GPIO_Port,
		.gun_on_off_pin = relay_8_Pin,
	},
};

static bms_info_config_t *get_bms_info_config(can_info_t *can_info)
{
	int i;
	bms_info_config_t *bms_info_config = NULL;
	bms_info_config_t *bms_info_config_item = NULL;

	for(i = 0; i < sizeof(bms_info_config_sz) / sizeof(bms_info_config_t); i++) {
		bms_info_config_item = bms_info_config_sz + i;

		if(can_info->hcan == bms_info_config_item->hcan) {
			bms_info_config = bms_info_config_item;
			break;
		}
	}

	return bms_info_config;
}

static void bms_data_settings_default_init(bms_data_settings_t *settings)
{
	if(settings == NULL) {
		return;
	}

	memset(settings, 0, sizeof(bms_data_settings_t));

	settings->bms_data_common.common_max_charge_voltage = 750 * 10;
	settings->bms_data_common.common_soc = 60 * 10;

	settings->bhm_data.max_charge_voltage = settings->bms_data_common.common_max_charge_voltage;

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
	settings->bcp_data.max_charge_voltage = settings->bms_data_common.common_max_charge_voltage;
	settings->bcp_data.max_temperature = (85) + 50;
	settings->bcp_data.soc = settings->bms_data_common.common_soc;
	settings->bcp_data.total_voltage = 500 * 10;
	settings->bcp_data.transfer_type = 0xff;

	settings->bcl_data.require_voltage = 600 * 10;
	settings->bcl_data.require_current = (4000 - 30 * 10);
	settings->bcl_data.charge_mode = 0x02;
	settings->bcs_data.charge_voltage = 600 * 10;
	settings->bcs_data.charge_current = 4000 - 30 * 10;
	settings->bcs_data.u1.s.single_battery_max_voltage = 400;
	settings->bcs_data.u1.s.single_battery_max_group = 4;
	settings->bcs_data.soc = settings->bms_data_common.common_soc / 10;
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

	settings->bsd_data.soc = settings->bms_data_common.common_soc / 10;
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
	memset_0(bms_info->settings->cro_data);
	memset_0(bms_info->settings->ccs_data);
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

bms_info_t *get_bms_info(can_info_t *can_info)
{
	bms_info_t *bms_info = NULL;
	bms_info_t *bms_info_item = NULL;

	list_for_each_entry(bms_info_item, &bms_info_list, bms_info_t, list) {
		if(bms_info_item->can_info == can_info) {
			bms_info = bms_info_item;
			break;
		}
	}

	return bms_info;
}

void free_bms_info(bms_info_t *bms_info)
{
	osStatus os_status;

	if(bms_info == NULL) {
		return;
	}

	list_del(&bms_info->list);
	set_modbus_data(bms_info->modbus_info, NULL, 0, 0);
	remove_modbus_data_changed_cb(bms_info->modbus_info, &modbus_data_changed_cb);

	set_bitmap_value(eeprom_modbus_data_bitmap, bms_info->eeprom_modbus_data_index, 0);

	os_free(bms_info->eeprom_modbus_data);

	if(bms_info->settings != NULL) {
		os_free(bms_info->settings);
	}

	if(bms_info->handle_mutex) {
		os_status = osMutexDelete(bms_info->handle_mutex);

		if(osOK != os_status) {
		}
	}

	if(bms_info->bms_data_mutex) {
		os_status = osMutexDelete(bms_info->bms_data_mutex);

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
		HAL_GPIO_WritePin(bms_info->gun_on_off_gpio, bms_info->gun_on_off_pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(bms_info->gun_on_off_gpio, bms_info->gun_on_off_pin, GPIO_PIN_SET);
	}
}

bms_info_t *alloc_bms_info(can_info_t *can_info)
{
	bms_info_t *bms_info = NULL;
	int index = -1;
	bms_info_config_t *bms_info_config = get_bms_info_config(can_info);

	osMutexDef(handle_mutex);
	osMutexDef(bms_data_mutex);

	if(bms_info_config == NULL) {
		return bms_info;
	}

	bms_info = get_bms_info(can_info);

	if(bms_info != NULL) {
		return bms_info;
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
		goto failed;
	}

	set_bitmap_value(eeprom_modbus_data_bitmap, index, 1);

	bms_info->eeprom_modbus_data_index = index;

	bms_info->eeprom_modbus_data = (eeprom_modbus_data_t *)os_alloc(sizeof(eeprom_modbus_data_t));

	if(bms_info->eeprom_modbus_data == NULL) {
		goto failed;
	}

	memset(bms_info->eeprom_modbus_data, 0, sizeof(eeprom_modbus_data_t));

	bms_info->can_info = can_info;
	bms_info->state = BMS_STATE_IDLE;
	bms_info->handle_mutex = osMutexCreate(osMutex(handle_mutex));
	bms_info->bms_data_mutex = osMutexCreate(osMutex(bms_data_mutex));
	bms_info->settings = bms_data_alloc_settings();
	bms_info->gun_connect_gpio = bms_info_config->gun_connect_gpio;
	bms_info->gun_connect_pin = bms_info_config->gun_connect_pin;
	bms_info->bms_poweron_enable_gpio = bms_info_config->bms_poweron_enable_gpio;
	bms_info->bms_poweron_enable_pin = bms_info_config->bms_poweron_enable_pin;
	bms_info->gun_on_off_gpio = bms_info_config->gun_on_off_gpio;
	bms_info->gun_on_off_pin = bms_info_config->gun_on_off_pin;

	bms_info->modbus_info = NULL;
	bms_info->modbus_data = &bms_info->eeprom_modbus_data->modbus_data;

	set_gun_on_off(bms_info, 0);

	list_add_tail(&bms_info->list, &bms_info_list);

	return bms_info;

failed:

	if(bms_info != NULL) {
		if(bms_info->eeprom_modbus_data != NULL) {
			os_free(bms_info->eeprom_modbus_data);
		}

		os_free(bms_info);
		bms_info = NULL;
	}

	return bms_info;
}

static void modbus_data_changed(void *fn_ctx, void *chain_ctx)
{
	//udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
	modbus_data_to_bms_data((bms_info_t *)fn_ctx);
	save_eeprom_modbus_data((bms_info_t *)fn_ctx);
}


void bms_set_modbus_info(bms_info_t *bms_info, modbus_info_t *modbus_info)
{
	bms_info->modbus_info = modbus_info;

	set_modbus_data(bms_info->modbus_info, (uint16_t *)bms_info->modbus_data, 0, sizeof(modbus_data_t));

	modbus_data_changed_cb.fn = modbus_data_changed;
	modbus_data_changed_cb.fn_ctx = bms_info;

	add_modbus_data_changed_cb(bms_info->modbus_info, &modbus_data_changed_cb);
}

void bms_set_eeprom_info(bms_info_t *bms_info, eeprom_info_t *eeprom_info)
{
	bms_info->eeprom_info = eeprom_info;
}

#define p_offset(member) do { \
	udp_log_printf("%-50s:%d\n", #member, (uint16_t *)&data->member - (uint16_t *)data); \
} while(0)

void show_modbus_data_offset(void)
{
	modbus_data_t *data = 0;

	//p_offset(bhm_max_charge_voltage);

	p_offset(brm_version_1);
	p_offset(brm_version_0);
	p_offset(brm_battery_type);
	p_offset(brm_total_battery_rate_capicity);
	p_offset(brm_total_battery_rate_voltage);
	p_offset(brm_battery_vendor[0]);
	p_offset(brm_battery_vendor_sn[0]);
	p_offset(brm_battery_year);
	p_offset(brm_battery_month);
	p_offset(brm_battery_day);
	p_offset(brm_battery_charge_times[0]);
	p_offset(brm_battery_property);
	p_offset(brm_vin[0]);
	p_offset(brm_version_serial);
	p_offset(brm_version_day);
	p_offset(brm_version_month);
	p_offset(brm_version_year);

	p_offset(bcp_max_charge_voltage_single_battery);
	p_offset(bcp_max_charge_current);
	p_offset(bcp_rate_total_power);
	//p_offset(bcp_max_charge_voltage);
	p_offset(bcp_max_temperature);
	//p_offset(bcp_soc);
	p_offset(bcp_total_voltage);
	p_offset(bcp_transfer_type);

	p_offset(bro_bro_result);

	p_offset(bcl_require_voltage);
	p_offset(bcl_require_current);
	p_offset(bcl_charge_mode);

	p_offset(bcs_charge_voltage);
	p_offset(bcs_charge_current);
	p_offset(bcs_single_battery_max_voltage);
	p_offset(bcs_single_battery_max_group);
	//p_offset(bcs_soc);
	p_offset(bcs_remain_min);

	p_offset(bsm_single_max_voltage_group);
	p_offset(bsm_battery_max_temperature);
	p_offset(bsm_battery_max_temperature_sn);
	p_offset(bsm_battery_min_temperature);
	p_offset(bsm_battery_min_temperature_sn);
	p_offset(bsm_single_voltage_state);
	p_offset(bsm_total_soc_state);
	p_offset(bsm_battery_current_state);
	p_offset(bsm_battery_temperature_state);
	p_offset(bsm_battery_insulation_state);
	p_offset(bsm_battery_connector_state);
	p_offset(bsm_battery_charge_enable);

	p_offset(bst_stop_reason_soc);
	p_offset(bst_stop_reason_voltage);
	p_offset(bst_stop_reason_single_voltage);
	p_offset(bst_stop_reason_charger_stop);
	p_offset(bst_stop_fault_reason_insulation);
	p_offset(bst_stop_fault_reason_connector_temperature);
	p_offset(bst_stop_fault_reason_bms_connector_temperature);
	p_offset(bst_stop_fault_reason_charger_connector);
	p_offset(bst_stop_fault_reason_battery_temperature);
	p_offset(bst_stop_fault_reason_relay);
	p_offset(bst_stop_fault_reason_voltage_check);
	p_offset(bst_stop_fault_reason_other);
	p_offset(bst_stop_error_reason_current);
	p_offset(bst_stop_error_reason_voltage);

	//p_offset(bsd_soc);
	p_offset(bsd_single_min_voltage);
	p_offset(bsd_single_max_voltage);
	p_offset(bsd_battery_min_temperature);
	p_offset(bsd_battery_max_temperature);

	p_offset(bem_crm_00_timeout);
	p_offset(bem_crm_aa_timeout);
	p_offset(bem_cts_cml_timeout);
	p_offset(bem_cro_timeout);
	p_offset(bem_ccs_timeout);
	p_offset(bem_cst_timeout);
	p_offset(bem_csd_timeout);
	p_offset(bem_other);

	p_offset(bms_common_max_charge_voltage);
	p_offset(bms_common_soc);

	p_offset(ccs_output_voltage);
	p_offset(ccs_output_current);
	p_offset(ccs_total_charge_time);
	p_offset(ccs_charge_enable);

	p_offset(cml_max_output_voltage);
	p_offset(cml_min_output_voltage);
	p_offset(cml_max_output_current);
	p_offset(cml_min_output_current);

	p_offset(disable_bhm);
	p_offset(disable_brm);
	p_offset(disable_bcp);
	p_offset(disable_bro);
	p_offset(disable_bcl);
	p_offset(disable_bcs);
	p_offset(disable_bsm);
	p_offset(disable_bmv);
	p_offset(disable_bmt);
	p_offset(disable_bsp);
	p_offset(disable_bst);
	p_offset(disable_bsd);
	p_offset(disable_bem);
	p_offset(stop_bms);
	p_offset(reset_bms_configure);
	p_offset(toggle_gun_on_off);

	p_offset(bms_state);
	p_offset(bms_gun_connect);
	p_offset(bms_poweron_enable);
	p_offset(gun_on_off_state);
}

void bms_data_to_modbus_data(bms_info_t *bms_info, uint8_t do_init)
{
	osStatus os_status;

	if(bms_info->bms_data_mutex) {
		os_status = osMutexWait(bms_info->bms_data_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	//bms_info->modbus_data->bhm_max_charge_voltage = bms_info->settings->bhm_data.max_charge_voltage;

	bms_info->modbus_data->brm_version_1 = bms_info->settings->brm_data.brm_data.version_1;
	bms_info->modbus_data->brm_version_0 = bms_info->settings->brm_data.brm_data.version_0;
	bms_info->modbus_data->brm_battery_type = bms_info->settings->brm_data.brm_data.battery_type;
	bms_info->modbus_data->brm_total_battery_rate_capicity = bms_info->settings->brm_data.brm_data.total_battery_rate_capicity;
	bms_info->modbus_data->brm_total_battery_rate_voltage = bms_info->settings->brm_data.brm_data.total_battery_rate_voltage;
	memcpy(bms_info->modbus_data->brm_battery_vendor, bms_info->settings->brm_data.battery_vendor, 4);
	memcpy((void *)bms_info->modbus_data->brm_battery_vendor_sn, (void *)&bms_info->settings->brm_data.battery_vendor_sn, 4);
	bms_info->modbus_data->brm_battery_year = bms_info->settings->brm_data.battery_year + 1985;
	bms_info->modbus_data->brm_battery_month = bms_info->settings->brm_data.battery_month;
	bms_info->modbus_data->brm_battery_day = bms_info->settings->brm_data.battery_day;
	memcpy(bms_info->modbus_data->brm_battery_charge_times, bms_info->settings->brm_data.battery_charge_times, 3);
	bms_info->modbus_data->brm_battery_property = bms_info->settings->brm_data.battery_property;
	memcpy(bms_info->modbus_data->brm_vin, bms_info->settings->brm_data.vin, 17);
	bms_info->modbus_data->brm_version_serial = bms_info->settings->brm_data.version.serial;
	bms_info->modbus_data->brm_version_day = bms_info->settings->brm_data.version.day;
	bms_info->modbus_data->brm_version_month = bms_info->settings->brm_data.version.month;
	bms_info->modbus_data->brm_version_year = bms_info->settings->brm_data.version.year;

	bms_info->modbus_data->bcp_max_charge_voltage_single_battery = bms_info->settings->bcp_data.max_charge_voltage_single_battery;
	bms_info->modbus_data->bcp_max_charge_current = 4000 - bms_info->settings->bcp_data.max_charge_current;
	bms_info->modbus_data->bcp_rate_total_power = bms_info->settings->bcp_data.rate_total_power;
	//bms_info->modbus_data->bcp_max_charge_voltage = bms_info->settings->bcp_data.max_charge_voltage;
	bms_info->modbus_data->bcp_max_temperature = bms_info->settings->bcp_data.max_temperature - 50;
	//bms_info->modbus_data->bcp_soc = bms_info->settings->bcp_data.soc;
	bms_info->modbus_data->bcp_total_voltage = bms_info->settings->bcp_data.total_voltage;
	bms_info->modbus_data->bcp_transfer_type = bms_info->settings->bcp_data.transfer_type;

	bms_info->modbus_data->bro_bro_result = (bms_info->settings->bro_data.bro_result == 0x00) ? 0x00 : (bms_info->settings->bro_data.bro_result == 0xaa) ? 0x01 : 0x02;

	bms_info->modbus_data->bcl_require_voltage = bms_info->settings->bcl_data.require_voltage;
	bms_info->modbus_data->bcl_require_current = 4000 - bms_info->settings->bcl_data.require_current;
	bms_info->modbus_data->bcl_charge_mode = bms_info->settings->bcl_data.charge_mode - 1;

	bms_info->modbus_data->bcs_charge_voltage = bms_info->settings->bcs_data.charge_voltage;
	bms_info->modbus_data->bcs_charge_current = 4000 - bms_info->settings->bcs_data.charge_current;
	bms_info->modbus_data->bcs_single_battery_max_voltage = bms_info->settings->bcs_data.u1.s.single_battery_max_voltage;
	bms_info->modbus_data->bcs_single_battery_max_group = bms_info->settings->bcs_data.u1.s.single_battery_max_group;
	//bms_info->modbus_data->bcs_soc = bms_info->settings->bcs_data.soc;
	bms_info->modbus_data->bcs_remain_min = bms_info->settings->bcs_data.remain_min;

	bms_info->modbus_data->bsm_single_max_voltage_group = bms_info->settings->bsm_data.single_max_voltage_group;
	bms_info->modbus_data->bsm_battery_max_temperature = bms_info->settings->bsm_data.battery_max_temperature - 50;
	bms_info->modbus_data->bsm_battery_max_temperature_sn = bms_info->settings->bsm_data.battery_max_temperature_sn;
	bms_info->modbus_data->bsm_battery_min_temperature = bms_info->settings->bsm_data.battery_min_temperature - 50;
	bms_info->modbus_data->bsm_battery_min_temperature_sn = bms_info->settings->bsm_data.battery_min_temperature_sn;
	bms_info->modbus_data->bsm_single_voltage_state = bms_info->settings->bsm_data.u1.s.single_voltage_state;
	bms_info->modbus_data->bsm_total_soc_state = bms_info->settings->bsm_data.u1.s.total_soc_state;
	bms_info->modbus_data->bsm_battery_current_state = bms_info->settings->bsm_data.u1.s.battery_current_state;
	bms_info->modbus_data->bsm_battery_temperature_state = bms_info->settings->bsm_data.u1.s.battery_temperature_state;
	bms_info->modbus_data->bsm_battery_insulation_state = bms_info->settings->bsm_data.u2.s.battery_insulation_state;
	bms_info->modbus_data->bsm_battery_connector_state = bms_info->settings->bsm_data.u2.s.battery_connector_state;
	bms_info->modbus_data->bsm_battery_charge_enable = bms_info->settings->bsm_data.u2.s.battery_charge_enable;

	bms_info->modbus_data->bst_stop_reason_soc = bms_info->settings->bst_data.u1.s.stop_reason_soc;
	bms_info->modbus_data->bst_stop_reason_voltage = bms_info->settings->bst_data.u1.s.stop_reason_voltage;
	bms_info->modbus_data->bst_stop_reason_single_voltage = bms_info->settings->bst_data.u1.s.stop_reason_single_voltage;
	bms_info->modbus_data->bst_stop_reason_charger_stop = bms_info->settings->bst_data.u1.s.stop_reason_charger_stop;
	bms_info->modbus_data->bst_stop_fault_reason_insulation = bms_info->settings->bst_data.u2.s.stop_fault_reason_insulation;
	bms_info->modbus_data->bst_stop_fault_reason_connector_temperature = bms_info->settings->bst_data.u2.s.stop_fault_reason_connector_temperature;
	bms_info->modbus_data->bst_stop_fault_reason_bms_connector_temperature = bms_info->settings->bst_data.u2.s.stop_fault_reason_bms_connector_temperature;
	bms_info->modbus_data->bst_stop_fault_reason_charger_connector = bms_info->settings->bst_data.u2.s.stop_fault_reason_charger_connector;
	bms_info->modbus_data->bst_stop_fault_reason_battery_temperature = bms_info->settings->bst_data.u2.s.stop_fault_reason_battery_temperature;
	bms_info->modbus_data->bst_stop_fault_reason_relay = bms_info->settings->bst_data.u2.s.stop_fault_reason_relay;
	bms_info->modbus_data->bst_stop_fault_reason_voltage_check = bms_info->settings->bst_data.u2.s.stop_fault_reason_voltage_check;
	bms_info->modbus_data->bst_stop_fault_reason_other = bms_info->settings->bst_data.u2.s.stop_fault_reason_other;
	bms_info->modbus_data->bst_stop_error_reason_current = bms_info->settings->bst_data.u3.s.stop_error_reason_current;
	bms_info->modbus_data->bst_stop_error_reason_voltage = bms_info->settings->bst_data.u3.s.stop_error_reason_voltage;

	//bms_info->modbus_data->bsd_soc = bms_info->settings->bsd_data.soc;
	bms_info->modbus_data->bsd_single_min_voltage = bms_info->settings->bsd_data.single_min_voltage;
	bms_info->modbus_data->bsd_single_max_voltage = bms_info->settings->bsd_data.single_max_voltage;
	bms_info->modbus_data->bsd_battery_min_temperature = bms_info->settings->bsd_data.battery_min_temperature - 50;
	bms_info->modbus_data->bsd_battery_max_temperature = bms_info->settings->bsd_data.battery_max_temperature - 50;

	bms_info->modbus_data->bem_crm_00_timeout = bms_info->settings->bem_data.u1.s.crm_00_timeout;
	bms_info->modbus_data->bem_crm_aa_timeout = bms_info->settings->bem_data.u1.s.crm_aa_timeout;
	bms_info->modbus_data->bem_cts_cml_timeout = bms_info->settings->bem_data.u2.s.cts_cml_timeout;
	bms_info->modbus_data->bem_cro_timeout = bms_info->settings->bem_data.u2.s.cro_timeout;
	bms_info->modbus_data->bem_ccs_timeout = bms_info->settings->bem_data.u3.s.ccs_timeout;
	bms_info->modbus_data->bem_cst_timeout = bms_info->settings->bem_data.u3.s.cst_timeout;
	bms_info->modbus_data->bem_csd_timeout = bms_info->settings->bem_data.u4.s.csd_timeout;
	bms_info->modbus_data->bem_other = bms_info->settings->bem_data.u4.s.other;

	bms_info->modbus_data->bms_common_max_charge_voltage = bms_info->settings->bms_data_common.common_max_charge_voltage;
	bms_info->modbus_data->bms_common_soc = bms_info->settings->bms_data_common.common_soc;

	bms_info->modbus_data->ccs_output_voltage = bms_info->settings->ccs_data.output_voltage;
	bms_info->modbus_data->ccs_output_current = 4000 - bms_info->settings->ccs_data.output_current;
	bms_info->modbus_data->ccs_total_charge_time = bms_info->settings->ccs_data.total_charge_time;
	bms_info->modbus_data->ccs_charge_enable = bms_info->settings->ccs_data.u1.s.charge_enable;

	bms_info->modbus_data->cml_max_output_voltage = bms_info->settings->cml_data.max_output_voltage;
	bms_info->modbus_data->cml_min_output_voltage = bms_info->settings->cml_data.min_output_voltage;
	bms_info->modbus_data->cml_max_output_current = 4000 - bms_info->settings->cml_data.max_output_current;
	bms_info->modbus_data->cml_min_output_current = 4000 - bms_info->settings->cml_data.min_output_current;

	if(do_init != 0) {
		bms_info->modbus_data->disable_bhm = 0;
		bms_info->modbus_data->disable_brm = 0;
		bms_info->modbus_data->disable_bcp = 0;
		bms_info->modbus_data->disable_bro = 0;
		bms_info->modbus_data->disable_bcl = 0;
		bms_info->modbus_data->disable_bcs = 0;
		bms_info->modbus_data->disable_bsm = 0;
		bms_info->modbus_data->disable_bmv = 0;
		bms_info->modbus_data->disable_bmt = 0;
		bms_info->modbus_data->disable_bsp = 0;
		bms_info->modbus_data->disable_bst = 0;
		bms_info->modbus_data->disable_bsd = 0;
		bms_info->modbus_data->disable_bem = 0;
		bms_info->modbus_data->stop_bms = 0;
		bms_info->modbus_data->reset_bms_configure = 0;
	}

	bms_info->modbus_data->bms_state = bms_info->state;
	bms_info->modbus_data->bms_gun_connect = is_gun_connected(bms_info);
	bms_info->modbus_data->bms_poweron_enable = is_bms_poweron_enable(bms_info);
	bms_info->modbus_data->gun_on_off_state = get_gun_on_off(bms_info);

	if(bms_info->bms_data_mutex) {
		os_status = osMutexRelease(bms_info->bms_data_mutex);

		if(os_status != osOK) {
		}
	}
}

void modbus_data_to_bms_data(bms_info_t *bms_info)
{
	osStatus os_status;

	if(bms_info->bms_data_mutex) {
		os_status = osMutexWait(bms_info->bms_data_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	bms_info->settings->bms_data_common.common_max_charge_voltage = bms_info->modbus_data->bms_common_max_charge_voltage;
	bms_info->settings->bms_data_common.common_soc = bms_info->modbus_data->bms_common_soc;

	//bms_info->settings->bhm_data.max_charge_voltage = bms_info->modbus_data->bhm_max_charge_voltage;
	bms_info->settings->bhm_data.max_charge_voltage = bms_info->settings->bms_data_common.common_max_charge_voltage;


	bms_info->settings->brm_data.brm_data.version_1 = bms_info->modbus_data->brm_version_1;
	bms_info->settings->brm_data.brm_data.version_0 = bms_info->modbus_data->brm_version_0;
	bms_info->settings->brm_data.brm_data.battery_type = bms_info->modbus_data->brm_battery_type;
	bms_info->settings->brm_data.brm_data.total_battery_rate_capicity = bms_info->modbus_data->brm_total_battery_rate_capicity;
	bms_info->settings->brm_data.brm_data.total_battery_rate_voltage = bms_info->modbus_data->brm_total_battery_rate_voltage;
	memcpy(bms_info->settings->brm_data.battery_vendor, bms_info->modbus_data->brm_battery_vendor, 4);
	memcpy((void *)&bms_info->settings->brm_data.battery_vendor_sn, (void *)bms_info->modbus_data->brm_battery_vendor_sn, 4);
	bms_info->settings->brm_data.battery_year = bms_info->modbus_data->brm_battery_year - 1985;
	bms_info->settings->brm_data.battery_month = bms_info->modbus_data->brm_battery_month;
	bms_info->settings->brm_data.battery_day = bms_info->modbus_data->brm_battery_day;
	memcpy(bms_info->settings->brm_data.battery_charge_times, bms_info->modbus_data->brm_battery_charge_times, 3);
	bms_info->settings->brm_data.battery_property = bms_info->modbus_data->brm_battery_property;
	memcpy(bms_info->settings->brm_data.vin, bms_info->modbus_data->brm_vin, 17);
	bms_info->settings->brm_data.version.serial = bms_info->modbus_data->brm_version_serial;
	bms_info->settings->brm_data.version.day = bms_info->modbus_data->brm_version_day;
	bms_info->settings->brm_data.version.month = bms_info->modbus_data->brm_version_month;
	bms_info->settings->brm_data.version.year = bms_info->modbus_data->brm_version_year;

	bms_info->settings->bcp_data.max_charge_voltage_single_battery = bms_info->modbus_data->bcp_max_charge_voltage_single_battery;
	bms_info->settings->bcp_data.max_charge_current = 4000 - bms_info->modbus_data->bcp_max_charge_current;
	bms_info->settings->bcp_data.rate_total_power = bms_info->modbus_data->bcp_rate_total_power;
	//bms_info->settings->bcp_data.max_charge_voltage = bms_info->modbus_data->bcp_max_charge_voltage;
	bms_info->settings->bcp_data.max_charge_voltage = bms_info->settings->bms_data_common.common_max_charge_voltage;
	bms_info->settings->bcp_data.max_temperature = bms_info->modbus_data->bcp_max_temperature + 50;
	//bms_info->settings->bcp_data.soc = bms_info->modbus_data->bcp_soc;
	bms_info->settings->bcp_data.soc = bms_info->settings->bms_data_common.common_soc;
	bms_info->settings->bcp_data.total_voltage = bms_info->modbus_data->bcp_total_voltage;
	bms_info->settings->bcp_data.transfer_type = bms_info->modbus_data->bcp_transfer_type;

	bms_info->settings->bro_data.bro_result = (bms_info->modbus_data->bro_bro_result == 0x00) ? 0x00 : (bms_info->modbus_data->bro_bro_result == 0x01) ? 0xaa : 0xff;

	bms_info->settings->bcl_data.require_voltage = bms_info->modbus_data->bcl_require_voltage;
	bms_info->settings->bcl_data.require_current = 4000 - bms_info->modbus_data->bcl_require_current;
	bms_info->settings->bcl_data.charge_mode = bms_info->modbus_data->bcl_charge_mode + 1;

	bms_info->settings->bcs_data.charge_voltage = bms_info->modbus_data->bcs_charge_voltage;
	bms_info->settings->bcs_data.charge_current = 4000 - bms_info->modbus_data->bcs_charge_current;
	bms_info->settings->bcs_data.u1.s.single_battery_max_voltage = bms_info->modbus_data->bcs_single_battery_max_voltage;
	bms_info->settings->bcs_data.u1.s.single_battery_max_group = bms_info->modbus_data->bcs_single_battery_max_group;
	//bms_info->settings->bcs_data.soc = bms_info->modbus_data->bcs_soc / 10;
	bms_info->settings->bcs_data.soc = bms_info->settings->bms_data_common.common_soc / 10;
	bms_info->settings->bcs_data.remain_min = bms_info->modbus_data->bcs_remain_min;

	bms_info->settings->bsm_data.single_max_voltage_group = bms_info->modbus_data->bsm_single_max_voltage_group;
	bms_info->settings->bsm_data.battery_max_temperature = bms_info->modbus_data->bsm_battery_max_temperature + 50;
	bms_info->settings->bsm_data.battery_max_temperature_sn = bms_info->modbus_data->bsm_battery_max_temperature_sn;
	bms_info->settings->bsm_data.battery_min_temperature = bms_info->modbus_data->bsm_battery_min_temperature + 50;
	bms_info->settings->bsm_data.battery_min_temperature_sn = bms_info->modbus_data->bsm_battery_min_temperature_sn;
	bms_info->settings->bsm_data.u1.s.single_voltage_state = bms_info->modbus_data->bsm_single_voltage_state;
	bms_info->settings->bsm_data.u1.s.total_soc_state = bms_info->modbus_data->bsm_total_soc_state;
	bms_info->settings->bsm_data.u1.s.battery_current_state = bms_info->modbus_data->bsm_battery_current_state;
	bms_info->settings->bsm_data.u1.s.battery_temperature_state = bms_info->modbus_data->bsm_battery_temperature_state;
	bms_info->settings->bsm_data.u2.s.battery_insulation_state = bms_info->modbus_data->bsm_battery_insulation_state;
	bms_info->settings->bsm_data.u2.s.battery_connector_state = bms_info->modbus_data->bsm_battery_connector_state;
	bms_info->settings->bsm_data.u2.s.battery_charge_enable = bms_info->modbus_data->bsm_battery_charge_enable;

	bms_info->settings->bst_data.u1.s.stop_reason_soc = bms_info->modbus_data->bst_stop_reason_soc;
	bms_info->settings->bst_data.u1.s.stop_reason_voltage = bms_info->modbus_data->bst_stop_reason_voltage;
	bms_info->settings->bst_data.u1.s.stop_reason_single_voltage = bms_info->modbus_data->bst_stop_reason_single_voltage;
	bms_info->settings->bst_data.u1.s.stop_reason_charger_stop = bms_info->modbus_data->bst_stop_reason_charger_stop;
	bms_info->settings->bst_data.u2.s.stop_fault_reason_insulation = bms_info->modbus_data->bst_stop_fault_reason_insulation;
	bms_info->settings->bst_data.u2.s.stop_fault_reason_connector_temperature = bms_info->modbus_data->bst_stop_fault_reason_connector_temperature;
	bms_info->settings->bst_data.u2.s.stop_fault_reason_bms_connector_temperature = bms_info->modbus_data->bst_stop_fault_reason_bms_connector_temperature;
	bms_info->settings->bst_data.u2.s.stop_fault_reason_charger_connector = bms_info->modbus_data->bst_stop_fault_reason_charger_connector;
	bms_info->settings->bst_data.u2.s.stop_fault_reason_battery_temperature = bms_info->modbus_data->bst_stop_fault_reason_battery_temperature;
	bms_info->settings->bst_data.u2.s.stop_fault_reason_relay = bms_info->modbus_data->bst_stop_fault_reason_relay;
	bms_info->settings->bst_data.u2.s.stop_fault_reason_voltage_check = bms_info->modbus_data->bst_stop_fault_reason_voltage_check;
	bms_info->settings->bst_data.u2.s.stop_fault_reason_other = bms_info->modbus_data->bst_stop_fault_reason_other;
	bms_info->settings->bst_data.u3.s.stop_error_reason_current = bms_info->modbus_data->bst_stop_error_reason_current;
	bms_info->settings->bst_data.u3.s.stop_error_reason_voltage = bms_info->modbus_data->bst_stop_error_reason_voltage;

	//bms_info->settings->bsd_data.soc = bms_info->modbus_data->bsd_soc / 10;
	bms_info->settings->bsd_data.soc = bms_info->settings->bms_data_common.common_soc / 10;
	bms_info->settings->bsd_data.single_min_voltage = bms_info->modbus_data->bsd_single_min_voltage;
	bms_info->settings->bsd_data.single_max_voltage = bms_info->modbus_data->bsd_single_max_voltage;
	bms_info->settings->bsd_data.battery_min_temperature = bms_info->modbus_data->bsd_battery_min_temperature + 50;
	bms_info->settings->bsd_data.battery_max_temperature = bms_info->modbus_data->bsd_battery_max_temperature + 50;

	bms_info->settings->bem_data.u1.s.crm_00_timeout = bms_info->modbus_data->bem_crm_00_timeout;
	bms_info->settings->bem_data.u1.s.crm_aa_timeout = bms_info->modbus_data->bem_crm_aa_timeout;
	bms_info->settings->bem_data.u2.s.cts_cml_timeout = bms_info->modbus_data->bem_cts_cml_timeout;
	bms_info->settings->bem_data.u2.s.cro_timeout = bms_info->modbus_data->bem_cro_timeout;
	bms_info->settings->bem_data.u3.s.ccs_timeout = bms_info->modbus_data->bem_ccs_timeout;
	bms_info->settings->bem_data.u3.s.cst_timeout = bms_info->modbus_data->bem_cst_timeout;
	bms_info->settings->bem_data.u4.s.csd_timeout = bms_info->modbus_data->bem_csd_timeout;
	bms_info->settings->bem_data.u4.s.other = bms_info->modbus_data->bem_other;

	if(bms_info->bms_data_mutex) {
		os_status = osMutexRelease(bms_info->bms_data_mutex);

		if(os_status != osOK) {
		}
	}
}

int load_eeprom_modbus_data(bms_info_t *bms_info)
{
	int ret = -1;
	uint8_t id;
	uint32_t offset;
	uint32_t crc = 0;
	int i;
	eeprom_modbus_data_t *eeprom_modbus_data = NULL;
	uint8_t *data = NULL;

	if(bms_info == NULL) {
		return ret;
	}

	offset = sizeof(eeprom_modbus_data_t) * bms_info->eeprom_modbus_data_index;
	eeprom_modbus_data = bms_info->eeprom_modbus_data;

	for(i = 0; i < 10; i++) {
		id = eeprom_id(bms_info->eeprom_info);

		//udp_log_printf("eeprom id:0x%x\n", id);

		if(id == 0x29) {
			break;
		}

		osDelay(200);
	}

	if(id != 0x29) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	eeprom_read(bms_info->eeprom_info, offset, (uint8_t *)eeprom_modbus_data, sizeof(eeprom_modbus_data_t));

	if(eeprom_modbus_data->payload_size != sizeof(modbus_data_t)) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	crc += eeprom_modbus_data->payload_size;

	data = (uint8_t *)(&eeprom_modbus_data->modbus_data);

	for(i = 0; i < eeprom_modbus_data->payload_size; i++) {
		crc += data[i];
	}

	if(crc != eeprom_modbus_data->crc) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	ret = 0;
	return ret;
}

int save_eeprom_modbus_data(bms_info_t *bms_info)
{
	int ret = -1;
	uint8_t id;
	uint32_t offset;
	uint32_t crc = 0;
	int i;
	eeprom_modbus_data_t *eeprom_modbus_data = NULL;
	uint8_t *data = NULL;

	offset = sizeof(eeprom_modbus_data_t) * bms_info->eeprom_modbus_data_index;
	eeprom_modbus_data = bms_info->eeprom_modbus_data;

	if(bms_info == NULL) {
		return ret;
	}

	for(i = 0; i < 10; i++) {
		id = eeprom_id(bms_info->eeprom_info);

		//udp_log_printf("eeprom id:0x%x\n", id);

		if(id == 0x29) {
			break;
		}

		osDelay(200);
	}

	if(id != 0x29) {
		udp_log_printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
		return ret;
	}

	eeprom_modbus_data->payload_size = sizeof(modbus_data_t);

	crc += eeprom_modbus_data->payload_size;

	data = (uint8_t *)(&eeprom_modbus_data->modbus_data);

	for(i = 0; i < eeprom_modbus_data->payload_size; i++) {
		crc += data[i];
	}

	eeprom_modbus_data->crc = crc;

	eeprom_write(bms_info->eeprom_info, offset, (uint8_t *)eeprom_modbus_data, sizeof(eeprom_modbus_data_t));

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

	if(ret == 0) {
		modbus_data_to_bms_data(bms_info);
		bms_info->modbus_data->bms_state = bms_info->state;//初始化bms状态
		bms_info->modbus_data->bms_gun_connect = is_gun_connected(bms_info);
		bms_info->modbus_data->bms_poweron_enable = is_bms_poweron_enable(bms_info);
	} else {
		bms_data_to_modbus_data(bms_info, 1);
	}

	if(ret != 0) {
		save_eeprom_modbus_data(bms_info);
	}
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

	if(state == get_bms_state(bms_info)) {
		return;
	}

	if(get_bms_state(bms_info) == BMS_STATE_IDLE) {
		reset_bms_data_settings_charger_data(bms_info);
	}

	handler = bms_get_state_handler(state);

	if((handler != NULL) && (handler->prepare != NULL)) {
		handler->prepare(bms_info);
	}

	udp_log_printf("change to state:%s!\n", get_bms_state_des(state));

	bms_info->state = state;

	bms_data_to_modbus_data(bms_info, 0);
	bms_info->modbus_data->bms_state = bms_info->state;
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
		bms_data_to_modbus_data(bms_info, 0);
	}

	if(bms_info->handle_mutex) {
		os_status = osMutexRelease(bms_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

uint8_t is_gun_connected(bms_info_t *bms_info)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(bms_info->gun_connect_gpio, bms_info->gun_connect_pin);
	static GPIO_PinState state_pre = GPIO_PIN_RESET;

	if(state_pre != state) {
		udp_log_printf("%s %d -> %d\n", __func__, state_pre, state);
		state_pre = state;
	}

	if(state == GPIO_PIN_RESET) {
		//return 0;
		return 1;
	} else {
		return 1;
	}
}

uint8_t is_bms_poweron_enable(bms_info_t *bms_info)
{
	GPIO_PinState state = HAL_GPIO_ReadPin(bms_info->bms_poweron_enable_gpio, bms_info->bms_poweron_enable_pin);
	static GPIO_PinState state_pre = GPIO_PIN_RESET;

	if(state_pre != state) {
		udp_log_printf("%s %d -> %d\n", __func__, state_pre, state);
		state_pre = state;
	}

	if(state == GPIO_PIN_RESET) {
		//return 0;
		return 1;
	} else {
		return 1;
	}
}

static void try_to_reset_bms_configure(bms_info_t *bms_info)
{
	if(bms_info->modbus_data->reset_bms_configure != 0) {
		bms_info->modbus_data->reset_bms_configure = 0;
		bms_data_settings_default_init(bms_info->settings);
		bms_data_to_modbus_data(bms_info, 1);

		save_eeprom_modbus_data(bms_info);
	}
}

static void try_to_stop_bms(bms_info_t *bms_info)
{
	if(bms_info->modbus_data->stop_bms != 0) {
		bms_info->modbus_data->stop_bms = 0;

		save_eeprom_modbus_data(bms_info);

		if(bms_info->state != BMS_STATE_BCL_BCS_BSM_BMV_BMT_BSP) {
			return;
		}

		bms_info->modbus_data->bst_stop_reason_soc = 1;
		bms_info->settings->bst_data.u1.s.stop_reason_soc = 1;
		set_bms_state_locked(bms_info, BMS_STATE_BST);
	}
}

static void try_to_toggle_gun_on_off(bms_info_t *bms_info)
{
	if(bms_info->modbus_data->toggle_gun_on_off != 0) {
		uint8_t on_off = get_gun_on_off(bms_info);

		bms_info->modbus_data->toggle_gun_on_off = 0;

		save_eeprom_modbus_data(bms_info);

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

static void update_ui_data(bms_info_t *bms_info)
{
	bms_info->modbus_data->bms_gun_connect = is_gun_connected(bms_info);
	bms_info->modbus_data->bms_poweron_enable = is_bms_poweron_enable(bms_info);
	bms_info->modbus_data->gun_on_off_state = get_gun_on_off(bms_info);
}

static void process_ui_request(bms_info_t *bms_info)
{
	try_to_reset_bms_configure(bms_info);
	try_to_stop_bms(bms_info);
	try_to_toggle_gun_on_off(bms_info);
}

void bms_periodic(bms_info_t *bms_info)
{
	process_ui_request(bms_info);
	update_ui_data(bms_info);
}
