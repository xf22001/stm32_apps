

/*================================================================
 *   
 *   
 *   文件名称：channel_record.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分28秒
 *   修改日期：2021年06月10日 星期四 17时29分16秒
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
	CHANNEL_RECORD_ITEM_STOP_REASON_NONE = 0,
	CHANNEL_RECORD_ITEM_STOP_REASON_MANUAL,
	CHANNEL_RECORD_ITEM_STOP_REASON_TEMPERATURE_LIMIT,
	CHANNEL_RECORD_ITEM_STOP_REASON_VOLTAGE_LIMIT,
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
} channel_record_item_stop_reason_t;

typedef struct {
	uint16_t id;
	uint8_t channel_id;
	uint8_t state;//channel_record_item_state_t
	uint8_t stop_reason;//channel_record_item_stop_reason_t

	uint32_t account_balance;
	uint32_t amount;
	uint32_t total_energy;
	uint32_t energy_seg[PRICE_SEGMENT_SIZE];

	uint32_t energy;
	uint32_t voltage;
	uint32_t current;
} channel_record_item_t;

#pragma pack(pop)

typedef struct {
	uint32_t id;
	channel_record_info_t channel_record_info;
	callback_chain_t *channel_record_sync_chain;
	os_signal_t sync_signal;
	os_mutex_t mutex;
	eeprom_info_t *eeprom_info;
} channel_record_task_info_t; 

int alloc_channel_record_item_id(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item);
int get_channel_record_item_by_id(channel_record_task_info_t *channel_record_task_info, uint16_t id, channel_record_item_t *channel_record_item);
int channel_record_update(channel_record_task_info_t *channel_record_task_info, channel_record_item_t *channel_record_item);
int channel_record_sync(channel_record_task_info_t *channel_record_task_info);
channel_record_task_info_t *get_or_alloc_channel_record_task_info(uint32_t id);

#endif //_CHANNEL_RECORD_H
