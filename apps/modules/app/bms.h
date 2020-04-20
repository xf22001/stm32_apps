

/*================================================================
 *
 *
 *   文件名称：bms.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分57秒
 *   修改日期：2020年04月20日 星期一 15时12分00秒
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
#include "modbus_slave_txrx.h"
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

typedef enum {
	MODBUS_ADDR_BRM_VERSION_1 = 0,
	MODBUS_ADDR_BRM_VERSION_0 = 1,
	MODBUS_ADDR_BRM_BATTERY_TYPE = 2,
	MODBUS_ADDR_BRM_TOTAL_BATTERY_RATE_CAPICITY = 3,
	MODBUS_ADDR_BRM_TOTAL_BATTERY_RATE_VOLTAGE = 4,
	MODBUS_ADDR_BRM_BATTERY_VENDOR_0 = 5,
	MODBUS_ADDR_BRM_BATTERY_VENDOR_1 = 6,
	MODBUS_ADDR_BRM_BATTERY_VENDOR_SN_0 = 7,
	MODBUS_ADDR_BRM_BATTERY_VENDOR_SN_1 = 8,
	MODBUS_ADDR_BRM_BATTERY_YEAR = 9,
	MODBUS_ADDR_BRM_BATTERY_MONTH = 10,
	MODBUS_ADDR_BRM_BATTERY_DAY = 11,
	MODBUS_ADDR_BRM_BATTERY_CHARGE_TIMES_0 = 12,
	MODBUS_ADDR_BRM_BATTERY_CHARGE_TIMES_1 = 13,
	MODBUS_ADDR_BRM_BATTERY_PROPERTY = 14,
	MODBUS_ADDR_BRM_VIN_0 = 15,
	MODBUS_ADDR_BRM_VIN_1 = 16,
	MODBUS_ADDR_BRM_VIN_2 = 17,
	MODBUS_ADDR_BRM_VIN_3 = 18,
	MODBUS_ADDR_BRM_VIN_4 = 19,
	MODBUS_ADDR_BRM_VIN_5 = 20,
	MODBUS_ADDR_BRM_VIN_6 = 21,
	MODBUS_ADDR_BRM_VIN_7 = 22,
	MODBUS_ADDR_BRM_VIN_8 = 23,
	MODBUS_ADDR_BRM_VERSION_SERIAL = 24,
	MODBUS_ADDR_BRM_VERSION_DAY = 25,
	MODBUS_ADDR_BRM_VERSION_MONTH = 26,
	MODBUS_ADDR_BRM_VERSION_YEAR = 27,
	MODBUS_ADDR_BCP_MAX_CHARGE_VOLTAGE_SINGLE_BATTERY = 28,
	MODBUS_ADDR_BCP_MAX_CHARGE_CURRENT = 29,
	MODBUS_ADDR_BCP_RATE_TOTAL_POWER = 30,
	MODBUS_ADDR_BCP_MAX_TEMPERATURE = 31,
	MODBUS_ADDR_BCP_TOTAL_VOLTAGE = 32,
	MODBUS_ADDR_BCP_TRANSFER_TYPE = 33,
	MODBUS_ADDR_BRO_BRO_RESULT = 34,
	MODBUS_ADDR_BCL_REQUIRE_VOLTAGE = 35,
	MODBUS_ADDR_BCL_REQUIRE_CURRENT = 36,
	MODBUS_ADDR_BCL_CHARGE_MODE = 37,
	MODBUS_ADDR_BCS_CHARGE_VOLTAGE = 38,
	MODBUS_ADDR_BCS_CHARGE_CURRENT = 39,
	MODBUS_ADDR_BCS_SINGLE_BATTERY_MAX_VOLTAGE = 40,
	MODBUS_ADDR_BCS_SINGLE_BATTERY_MAX_GROUP = 41,
	MODBUS_ADDR_BCS_REMAIN_MIN = 42,
	MODBUS_ADDR_BSM_SINGLE_MAX_VOLTAGE_GROUP = 43,
	MODBUS_ADDR_BSM_BATTERY_MAX_TEMPERATURE = 44,
	MODBUS_ADDR_BSM_BATTERY_MAX_TEMPERATURE_SN = 45,
	MODBUS_ADDR_BSM_BATTERY_MIN_TEMPERATURE = 46,
	MODBUS_ADDR_BSM_BATTERY_MIN_TEMPERATURE_SN = 47,
	MODBUS_ADDR_BSM_SINGLE_VOLTAGE_STATE = 48,
	MODBUS_ADDR_BSM_TOTAL_SOC_STATE = 49,
	MODBUS_ADDR_BSM_BATTERY_CURRENT_STATE = 50,
	MODBUS_ADDR_BSM_BATTERY_TEMPERATURE_STATE = 51,
	MODBUS_ADDR_BSM_BATTERY_INSULATION_STATE = 52,
	MODBUS_ADDR_BSM_BATTERY_CONNECTOR_STATE = 53,
	MODBUS_ADDR_BSM_BATTERY_CHARGE_ENABLE = 54,
	MODBUS_ADDR_BST_STOP_REASON_SOC = 55,
	MODBUS_ADDR_BST_STOP_REASON_VOLTAGE = 56,
	MODBUS_ADDR_BST_STOP_REASON_SINGLE_VOLTAGE = 57,
	MODBUS_ADDR_BST_STOP_REASON_CHARGER_STOP = 58,
	MODBUS_ADDR_BST_STOP_FAULT_REASON_INSULATION = 59,
	MODBUS_ADDR_BST_STOP_FAULT_REASON_CONNECTOR_TEMPERATURE = 60,
	MODBUS_ADDR_BST_STOP_FAULT_REASON_BMS_CONNECTOR_TEMPERATURE = 61,
	MODBUS_ADDR_BST_STOP_FAULT_REASON_CHARGER_CONNECTOR = 62,
	MODBUS_ADDR_BST_STOP_FAULT_REASON_BATTERY_TEMPERATURE = 63,
	MODBUS_ADDR_BST_STOP_FAULT_REASON_RELAY = 64,
	MODBUS_ADDR_BST_STOP_FAULT_REASON_VOLTAGE_CHECK = 65,
	MODBUS_ADDR_BST_STOP_FAULT_REASON_OTHER = 66,
	MODBUS_ADDR_BST_STOP_ERROR_REASON_CURRENT = 67,
	MODBUS_ADDR_BST_STOP_ERROR_REASON_VOLTAGE = 68,
	MODBUS_ADDR_BSD_SINGLE_MIN_VOLTAGE = 69,
	MODBUS_ADDR_BSD_SINGLE_MAX_VOLTAGE = 70,
	MODBUS_ADDR_BSD_BATTERY_MIN_TEMPERATURE = 71,
	MODBUS_ADDR_BSD_BATTERY_MAX_TEMPERATURE = 72,
	MODBUS_ADDR_BEM_CRM_00_TIMEOUT = 73,
	MODBUS_ADDR_BEM_CRM_AA_TIMEOUT = 74,
	MODBUS_ADDR_BEM_CTS_CML_TIMEOUT = 75,
	MODBUS_ADDR_BEM_CRO_TIMEOUT = 76,
	MODBUS_ADDR_BEM_CCS_TIMEOUT = 77,
	MODBUS_ADDR_BEM_CST_TIMEOUT = 78,
	MODBUS_ADDR_BEM_CSD_TIMEOUT = 79,
	MODBUS_ADDR_BEM_OTHER = 80,
	MODBUS_ADDR_BMS_COMMON_MAX_CHARGE_VOLTAGE = 81,
	MODBUS_ADDR_BMS_COMMON_SOC = 82,
	MODBUS_ADDR_CCS_OUTPUT_VOLTAGE = 83,
	MODBUS_ADDR_CCS_OUTPUT_CURRENT = 84,
	MODBUS_ADDR_CCS_TOTAL_CHARGE_TIME = 85,
	MODBUS_ADDR_CCS_CHARGE_ENABLE = 86,
	MODBUS_ADDR_CML_MAX_OUTPUT_VOLTAGE = 87,
	MODBUS_ADDR_CML_MIN_OUTPUT_VOLTAGE = 88,
	MODBUS_ADDR_CML_MAX_OUTPUT_CURRENT = 89,
	MODBUS_ADDR_CML_MIN_OUTPUT_CURRENT = 90,
	MODBUS_ADDR_DISABLE_BHM = 91,
	MODBUS_ADDR_DISABLE_BRM = 92,
	MODBUS_ADDR_DISABLE_BCP = 93,
	MODBUS_ADDR_DISABLE_BRO = 94,
	MODBUS_ADDR_DISABLE_BCL = 95,
	MODBUS_ADDR_DISABLE_BCS = 96,
	MODBUS_ADDR_DISABLE_BSM = 97,
	MODBUS_ADDR_DISABLE_BMV = 98,
	MODBUS_ADDR_DISABLE_BMT = 99,
	MODBUS_ADDR_DISABLE_BSP = 100,
	MODBUS_ADDR_DISABLE_BST = 101,
	MODBUS_ADDR_DISABLE_BSD = 102,
	MODBUS_ADDR_DISABLE_BEM = 103,
	MODBUS_ADDR_STOP_BMS = 104,
	MODBUS_ADDR_RESET_BMS_CONFIGURE = 105,
	MODBUS_ADDR_TOGGLE_GUN_ON_OFF = 106,
	MODBUS_ADDR_BMS_STATE = 107,
	MODBUS_ADDR_BMS_GUN_CONNECT = 108,
	MODBUS_ADDR_BMS_POWERON_ENABLE = 109,
	MODBUS_ADDR_GUN_ON_OFF_STATE = 110,
	MODBUS_ADDR_VERSION_MAJOR = 111,
	MODBUS_ADDR_VERSION_MINOR = 112,
	MODBUS_ADDR_VERSION_REV = 113,
	MODBUS_ADDR_INVALID,
} modbus_addr_t;
typedef enum {
	MODBUS_DATA_GET = 0,
	MODBUS_DATA_SET,
} modbus_data_op_t;

#pragma pack(push, 1)

typedef struct {
	uint32_t crc;
	uint16_t payload_size;
} eeprom_modbus_head_t;

typedef struct {
	uint8_t disable_bhm;
	uint8_t disable_brm;
	uint8_t disable_bcp;
	uint8_t disable_bro;
	uint8_t disable_bcl;
	uint8_t disable_bcs;
	uint8_t disable_bsm;
	uint8_t disable_bmv;
	uint8_t disable_bmt;
	uint8_t disable_bsp;
	uint8_t disable_bst;
	uint8_t disable_bsd;
	uint8_t disable_bem;

	uint8_t stop_bms;
	uint8_t reset_bms_configure;
	uint8_t toggle_gun_on_off;
} bms_data_configs_t;

typedef struct {
	eeprom_modbus_head_t head;
	bms_data_settings_t bms_data_settings;
	bms_data_configs_t bms_data_config;
} eeprom_modbus_data_t;

#pragma pack(pop)

typedef struct {
	struct list_head list;

	can_info_t *can_info;
	bms_state_t state;
	osMutexId handle_mutex;

	GPIO_TypeDef *gun_connect_gpio;
	uint16_t gun_connect_pin;
	GPIO_TypeDef *bms_poweron_enable_gpio;
	uint16_t bms_poweron_enable_pin;
	GPIO_TypeDef *gun_on_off_gpio;
	uint16_t gun_on_off_pin;

	multi_packets_info_t multi_packets_info;

	bms_data_settings_t *settings;
	bms_data_configs_t configs;

	uint8_t gun_on_off_state;

	uint8_t bms_gun_connect;
	uint8_t bms_poweron_enable;

	modbus_slave_info_t *modbus_slave_info;
	modbus_slave_data_info_t modbus_slave_data_info;
	callback_item_t modbus_slave_data_changed_cb;

	uint8_t eeprom_modbus_data_index;
	eeprom_info_t *eeprom_info;

	uint32_t stamp;
	uint32_t stamp_1;
	uint32_t stamp_2;

	uint32_t send_stamp;
	uint32_t send_stamp_1;
	uint32_t send_stamp_2;

	uint8_t received_ccs;
	uint8_t sent_bst;
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

void free_bms_info(bms_info_t *bms_info);
bms_info_t *get_or_alloc_bms_info(can_info_t *can_info);
void bms_set_modbus_slave_info(bms_info_t *bms_info, modbus_slave_info_t *modbus_slave_info);
void bms_set_eeprom_info(bms_info_t *bms_info, eeprom_info_t *eeprom_info);
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
