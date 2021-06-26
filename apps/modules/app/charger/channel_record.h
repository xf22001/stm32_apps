

/*================================================================
 *   
 *   
 *   文件名称：channel_record.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分28秒
 *   修改日期：2021年06月26日 星期六 12时33分54秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_RECORD_H
#define _CHANNEL_RECORD_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "callback_chain.h"
#include "eeprom.h"
#include "channels_config.h"

#ifdef __cplusplus
}
#endif

#define CHANNEL_RECORD_NUMBER 500

#pragma pack(push, 1)

typedef struct {
	uint16_t start;
	uint16_t end;
} channel_record_info_t;

typedef enum {
	CHANNEL_RECORD_ITEM_STATE_INVALID = 0,
	CHANNEL_RECORD_ITEM_STATE_INIT,
	CHANNEL_RECORD_ITEM_STATE_UPDATE,
	CHANNEL_RECORD_ITEM_STATE_FINISH,
	CHANNEL_RECORD_ITEM_STATE_UPLOAD,
} channel_record_item_state_t;

typedef enum {
	CHANNEL_RECORD_ITEM_START_REASON_MANUAL = 0,
	CHANNEL_RECORD_ITEM_START_REASON_CARD,
	CHANNEL_RECORD_ITEM_START_REASON_REMOTE,
	CHANNEL_RECORD_ITEM_START_REASON_VIN,
	CHANNEL_RECORD_ITEM_START_REASON_BMS,
	CHANNEL_RECORD_ITEM_START_REASON_CHARGER,
} channel_record_item_start_reason_t;

typedef enum {
	CHANNEL_RECORD_ITEM_STOP_REASON_NONE = 0,
	CHANNEL_RECORD_ITEM_STOP_REASON_MANUAL,
	CHANNEL_RECORD_ITEM_STOP_REASON_TEMPERATURE_LIMIT,
	CHANNEL_RECORD_ITEM_STOP_REASON_VOLTAGE_LIMIT,
	CHANNEL_RECORD_ITEM_STOP_REASON_AMOUNT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SOC_ARCHIEVED,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SOC_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_VOLTAGE_ARCHIEVED,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_VOLTAGE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SINGLE_VOLTAGE_ARCHIEVED,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SINGLE_VOLTAGE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_INSULATION_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_INSULATION_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_OVER_TEMPERATURE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_CONNECTOR_OVER_TEMPERATURE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_OVER_TEMPERATURE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_TEMPERATURE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OTHER_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OTHER_FAULT_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OVER_CURRENT,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CURRENT_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_ABNORMAL_VOLTAGE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_ABNORMAL_VOLTAGE_NOT_CREDIBLE,
	CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_OTHER_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BRM_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCP_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BRO_READY_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCS_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BCL_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BST_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_BSD_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_CRO_NOT_READY_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_CHECK_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_DISCHARGE_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_DISCHARGE_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ADHESION_CHECK_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ADHESION_FAULT,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_INSULATION_CHECK_WITH_ELECTRIFIED,
	CHANNEL_RECORD_ITEM_STOP_REASON_DC_BMS_GB_ABNORMAL_VOLTAGE,
	CHANNEL_RECORD_ITEM_STOP_REASON_CHARGER_NOT_CONNECTED,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_CHECK_L_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_L,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_CHECK_N_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_N,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_CC1_READY_0_TIMEOUT,
	CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_CC1_READY_1_TIMEOUT,
} channel_record_item_stop_reason_t;

typedef struct {
	uint16_t id;
	uint8_t channel_id;
	uint8_t state;//channel_record_item_state_t
	uint8_t start_reason;//channel_record_item_start_reason_t
	uint8_t stop_reason;//channel_record_item_stop_reason_t
	uint64_t card_id;

	time_t start_time;
	time_t stop_time;

	uint8_t vin[17];
	uint8_t chm_version_1;//0x01
	uint16_t chm_version_0;//0x01
	uint8_t brm_battery_type;//0x01 : '铅酸电池', 0x02 : '镍氢电池', 0x03 : '磷酸电池', 0x04 : '锰酸锂电池', 0x05 : '钴酸锂电池', 0x06 : '三元材料电池', 0x07 : '聚合物电池', 0x08 : '钛酸锂电池', 0xff : '其他电池'
	uint16_t bcp_rate_total_power;//0.1kwh
	uint16_t bcp_total_voltage;//0.1v
	uint16_t bcp_max_charge_voltage_single_battery;//0.01v
	uint8_t bcp_max_temperature;// -50
	uint16_t bcp_max_charge_voltage;//0.1v 最高允许充电总电压
	uint8_t start_soc;
	uint8_t stop_soc;

	uint32_t withholding;//告诉后台此卡的预扣款是多少 0.01 元
	uint32_t account_balance;
	uint32_t amount;
	uint32_t start_total_energy;//0.0001kwh
	uint32_t stop_total_energy;//0.0001kwh
	uint32_t energy_seg[PRICE_SEGMENT_SIZE];

	uint32_t energy;

	uint8_t magic;//0x73
} channel_record_item_t;

#pragma pack(pop)

typedef struct {
	uint8_t id;
	channel_record_info_t channel_record_info;
	callback_chain_t *channel_record_sync_chain;
	os_signal_t sync_signal;
	os_mutex_t mutex;
	eeprom_info_t *eeprom_info;
} channel_record_task_info_t; 

typedef int (*channel_record_state_filter_t)(channel_record_item_state_t state);

int alloc_channel_record_item_id(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item);
int get_channel_record_info(channel_record_task_info_t *channel_record_task_info, channel_record_info_t *channel_record_info);
int get_channel_record_item_by_id(channel_record_task_info_t *channel_record_task_info, uint16_t id, channel_record_item_t *channel_record_item);
int get_channel_record_item_by_state(channel_record_task_info_t *channel_record_task_info, channel_record_state_filter_t filter, uint16_t start, uint16_t end, uint16_t *id);
int channel_record_update(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item);
int channel_record_sync(channel_record_task_info_t *channel_record_task_info);
channel_record_task_info_t *get_or_alloc_channel_record_task_info(uint8_t id);

#endif //_CHANNEL_RECORD_H
