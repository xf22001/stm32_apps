

/*================================================================
 *
 *
 *   文件名称：bms.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分57秒
 *   修改日期：2020年03月27日 星期五 12时21分42秒
 *   描    述：
 *
 *================================================================*/
#ifndef _BMS_H
#define _BMS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "can_txrx.h"
#include "modbus_txrx.h"
#include "bms_spec.h"
#include "eeprom.h"

#ifdef __cplusplus
}
#endif
typedef enum {
	BMS_STATE_IDLE = 0,
	BMS_STATE_BHM,
	BMS_STATE_BRM,
	BMS_STATE_BCP,
	BMS_STATE_BRO,
	BMS_STATE_BCL_BCS_BSM_BMV_BMT_BSP,
	BMS_STATE_BST,
	BMS_STATE_BSD_BEM,
} bms_state_t;

typedef struct {
	//uint16_t bhm_max_charge_voltage;//0.1v

	uint16_t brm_version_1;
	uint16_t brm_version_0;
	uint16_t brm_battery_type;//0x01 : '铅酸电池', 0x02 : '镍氢电池', 0x03 : '磷酸电池', 0x04 : '锰酸锂电池', 0x05 : '钴酸锂电池', 0x06 : '三元材料电池', 0x07 : '聚合物电池', 0x08 : '钛酸锂电池', 0xff : '其他电池'
	uint16_t brm_total_battery_rate_capicity;//0.1ah
	uint16_t brm_total_battery_rate_voltage;//0.1v
	uint16_t brm_battery_vendor[(4 + 1) / sizeof(uint16_t)];
	uint16_t brm_battery_vendor_sn[(sizeof(uint32_t) + 1) / sizeof(uint16_t)];
	uint16_t brm_battery_year;//1985+
	uint16_t brm_battery_month;//1-12
	uint16_t brm_battery_day;//1-31
	uint16_t brm_battery_charge_times[(3 + 1) / sizeof(uint16_t)];
	uint16_t brm_battery_property;//0-租赁，1-自有
	uint16_t brm_vin[(17 + 1) / sizeof(uint16_t)];
	uint16_t brm_version_serial;
	uint16_t brm_version_day;
	uint16_t brm_version_month;
	uint16_t brm_version_year;

	uint16_t bcp_max_charge_voltage_single_battery;//0.01v
	uint16_t bcp_max_charge_current;//0.1a -4000
	uint16_t bcp_rate_total_power;//0.1kwh
	//uint16_t bcp_max_charge_voltage;//0.1v
	uint16_t bcp_max_temperature;// -50
	//uint16_t bcp_total_soc;//0.1%
	uint16_t bcp_total_voltage;//0.1v
	uint16_t bcp_transfer_type;//0xff

	uint16_t bro_bro_result;//0x00-未准备好, 0xaa-准备好, 0xff-无效

	uint16_t bcl_require_voltage;//0.1v
	uint16_t bcl_require_current;//0.1a
	uint16_t bcl_charge_mode;//0x01-恒压, 0x02-恒流

	uint16_t bcs_charge_voltage;//0.1v
	uint16_t bcs_charge_current;//0.1a -400
	uint16_t bcs_single_battery_max_voltage;//0.01v 0-24v
	uint16_t bcs_single_battery_max_group;//0-15
	//uint16_t bcs_soc;//0-100%
	uint16_t bcs_remain_min;//0-600min

	uint16_t bsm_single_max_voltage_group;//1-256
	uint16_t bsm_battery_max_temperature;//1 -50
	uint16_t bsm_battery_max_temperature_sn;//1-128
	uint16_t bsm_battery_min_temperature;//1 -50
	uint16_t bsm_battery_min_temperature_sn;//1-128
	uint16_t bsm_single_voltage_state;//0x00-正常, 0x01-过高, 0x10-过低
	uint16_t bsm_total_soc_state;//0x00-正常, 0x01-过高, 0x10-过低
	uint16_t bsm_battery_current_state;//0x00-正常, 0x01-过流, 0x10-不可信状态
	uint16_t bsm_battery_temperature_state;//0x00-正常, 0x01-过高, 0x10-不可信状态
	uint16_t bsm_battery_insulation_state;//0x00-正常, 0x01-不正常, 0x10-不可信状态
	uint16_t bsm_battery_connector_state;//0x00-正常, 0x01-不正常, 0x10-不可信状态
	uint16_t bsm_battery_charge_enable;//0x00-禁止, 0x01-允许, 0x10-不可信状态

	uint16_t bst_stop_reason_soc;//0x00-未达soc目标值, 0x01-达到soc目标值, 0x10-不可信状态
	uint16_t bst_stop_reason_voltage;//0x00-未达总电压目标值, 0x01-达到总电压目标值, 0x10-不可信状态
	uint16_t bst_stop_reason_single_voltage;//0x00-未达单体电压目标值, 0x01-达到单体电压目标值, 0x10-不可信状态
	uint16_t bst_stop_reason_charger_stop;//0x00-正常, 0x01-充电机终止, 0x10-不可信状态
	uint16_t bst_stop_fault_reason_insulation;//0x00-正常, 0x01-故障, 0x10-不可信状态
	uint16_t bst_stop_fault_reason_connector_temperature;//0x00-正常, 0x01-故障, 0x10-不可信状态
	uint16_t bst_stop_fault_reason_bms_connector_temperature;//0x00-正常, 0x01-故障, 0x10-不可信状态
	uint16_t bst_stop_fault_reason_charger_connector;//0x00-正常, 0x01-故障, 0x10-不可信状态
	uint16_t bst_stop_fault_reason_battery_temperature;//0x00-正常, 0x01-温度过高, 0x10-不可信状态
	uint16_t bst_stop_fault_reason_relay;//0x00-正常, 0x01-故障, 0x10-不可信状态
	uint16_t bst_stop_fault_reason_voltage_check;//0x00-正常, 0x01-故障, 0x10-不可信状态
	uint16_t bst_stop_fault_reason_other;//0x00-正常, 0x01-故障, 0x10-不可信状态
	uint16_t bst_stop_error_reason_current;//0x00-正常, 0x01-超过需求值, 0x10-不可信状态
	uint16_t bst_stop_error_reason_voltage;//0x00-正常, 0x01-电压异常, 0x10-不可信状态

	//uint16_t bsd_soc;//0-100%
	uint16_t bsd_single_min_voltage;//0.01v 0-24v
	uint16_t bsd_single_max_voltage;//0.01v 0-24v
	uint16_t bsd_battery_min_temperature;// -50
	uint16_t bsd_battery_max_temperature;// -50

	uint16_t bem_crm_00_timeout;//0x00-正常, 0x01-超时, 0x10-不可信状态
	uint16_t bem_crm_aa_timeout;//0x00-正常, 0x01-超时, 0x10-不可信状态
	uint16_t bem_cts_cml_timeout;//0x00-正常, 0x01-超时, 0x10-不可信状态
	uint16_t bem_cro_timeout;//0x00-正常, 0x01-超时, 0x10-不可信状态
	uint16_t bem_ccs_timeout;//0x00-正常, 0x01-超时, 0x10-不可信状态
	uint16_t bem_cst_timeout;//0x00-正常, 0x01-超时, 0x10-不可信状态
	uint16_t bem_csd_timeout;//0x00-正常, 0x01-超时, 0x10-不可信状态
	uint16_t bem_other;

	uint16_t bms_common_max_charge_voltage;//0.1v
	uint16_t bms_common_soc;//0-100.0%

	uint16_t ccs_output_voltage;//0.1v
	uint16_t ccs_output_current;//0.1a
	uint16_t ccs_total_charge_time;//1min 0-600
	uint16_t ccs_charge_enable;//0x00-暂停, 0x01-允许

	uint16_t cml_max_output_voltage;//0.1v
	uint16_t cml_min_output_voltage;//0.1v
	uint16_t cml_max_output_current;//0.1a
	uint16_t cml_min_output_current;//0.1a

	uint16_t disable_bhm;
	uint16_t disable_brm;
	uint16_t disable_bcp;
	uint16_t disable_bro;
	uint16_t disable_bcl;
	uint16_t disable_bcs;
	uint16_t disable_bsm;
	uint16_t disable_bmv;
	uint16_t disable_bmt;
	uint16_t disable_bsp;
	uint16_t disable_bst;
	uint16_t disable_bsd;
	uint16_t disable_bem;
	uint16_t stop_bms;
	uint16_t reset_bms_configure;
	uint16_t toggle_gun_on_off;

	uint16_t bms_state;
	uint16_t bms_gun_connect;
	uint16_t bms_poweron_enable;
	uint16_t gun_on_off_state;

	uint16_t version_major;
	uint16_t version_minor;
	uint16_t version_rev;
} modbus_data_t;

typedef struct {
	uint32_t crc;
	uint16_t payload_size;
	modbus_data_t modbus_data;
} eeprom_modbus_data_t;

typedef struct {
	struct list_head list;
	
	can_info_t *can_info;
	bms_state_t state;
	osMutexId handle_mutex;
	osMutexId bms_data_mutex;

	GPIO_TypeDef *gun_connect_gpio;
	uint16_t gun_connect_pin;
	GPIO_TypeDef *bms_poweron_enable_gpio;
	uint16_t bms_poweron_enable_pin;
	GPIO_TypeDef *gun_on_off_gpio;
	uint16_t gun_on_off_pin;

	uint16_t gun_on_off_state;

	uint8_t bms_data_multi_sz[1024];
	uint16_t bms_data_multi_fn;
	uint16_t bms_data_multi_bytes;
	uint16_t bms_data_multi_packets;
	uint16_t bms_data_multi_next_index;//1-total

	bms_data_settings_t *settings;

	modbus_info_t *modbus_info;
	modbus_data_t *modbus_data;
	uint8_t eeprom_modbus_data_index;
	eeprom_modbus_data_t *eeprom_modbus_data;
	eeprom_info_t *eeprom_info;

	uint32_t stamp;
	uint32_t stamp_1;
	uint32_t stamp_2;

	uint32_t send_stamp;
	uint32_t send_stamp_1;
	uint32_t send_stamp_2;

	uint8_t received_ccs;
	uint8_t received_csd;

	uint8_t received_cem;
	uint8_t sent_bem;
} bms_info_t;

typedef int (*bms_handle_state_t)(bms_info_t *bms_info);

typedef struct {
	bms_state_t state;
	bms_handle_state_t prepare;
	bms_handle_state_t handle_request;
	bms_handle_state_t handle_response;
} bms_state_handler_t;

bms_info_t *get_bms_info(can_info_t *can_info);
void free_bms_info(bms_info_t *bms_info);
bms_info_t *alloc_bms_info(can_info_t *can_info);
void bms_set_modbus_info(bms_info_t *bms_info, modbus_info_t *modbus_info);
void bms_set_eeprom_info(bms_info_t *bms_info, eeprom_info_t *eeprom_info);
void show_modbus_data_offset(void);
void bms_data_to_modbus_data(bms_info_t *bms_info, uint8_t do_init);
void modbus_data_to_bms_data(bms_info_t *bms_info);
int load_eeprom_modbus_data(bms_info_t *bms_info);
int save_eeprom_modbus_data(bms_info_t *bms_info);
void bms_restore_data(bms_info_t *bms_info);
bms_state_t get_bms_state(bms_info_t *bms_info);
void set_bms_state(bms_info_t *bms_info, bms_state_t state);
void set_bms_state_locked(bms_info_t *bms_info, bms_state_t state);
void bms_handle_request(bms_info_t *bms_info);
void bms_handle_response(bms_info_t *bms_info);
uint8_t is_gun_connected(bms_info_t *bms_info);
uint8_t is_bms_poweron_enable(bms_info_t *bms_info);
void set_gun_on_off(bms_info_t *bms_info, uint8_t on_off);
void bms_periodic(bms_info_t *bms_info);
#endif //_BMS_H
