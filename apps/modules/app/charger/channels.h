

/*================================================================
 *
 *
 *   文件名称：channels.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 10时08分44秒
 *   修改日期：2021年06月11日 星期五 11时08分37秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_H
#define _CHANNELS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "event_helper.h"
#include "channels_config.h"

#include "callback_chain.h"
#include "bitmap_ops.h"
#include "channel_record.h"

#ifdef __cplusplus
}
#endif

#define CHANNEL_TASK_PERIODIC (50)

//for channel event
typedef enum {
	CHANNEL_EVENT_TYPE_UNKNOW = 0,
	CHANNEL_EVENT_TYPE_START_CHANNEL,
	CHANNEL_EVENT_TYPE_STOP_CHANNEL,
} channel_event_type_t;

typedef struct {
	uint8_t channel_id;
	channel_event_type_t type;
} channel_event_t;


//all channels event type
typedef enum {
	CHANNELS_EVENT_CHANNEL_UNKNOW = 0,
	CHANNELS_EVENT_CHANNEL_INSULATION,
	CHANNELS_EVENT_CHANNEL_TELEMETER,
	CHANNELS_EVENT_CHANNEL_CARD_READER,
	CHANNELS_EVENT_CHANNEL_DISPLAY,
	CHANNELS_EVENT_CHANNEL_EVENT,
} channels_event_type_t;

//all channels event typedef
typedef struct {
	channels_event_type_t type;
	union {
		channel_event_t channel_event;
	} event;
} channels_event_t;

typedef int (*channel_init_t)(void *_channel_info);
typedef int (*channel_start_t)(void *_channel_info);
typedef int (*channel_stop_t)(void *_channel_info);

typedef struct {
	channel_type_t channel_type;
	channel_init_t init;
	channel_start_t channel_start;
	channel_stop_t channel_stop;
} channel_handler_t;

typedef enum {
	CHANNEL_STATE_NONE = 0,
	CHANNEL_STATE_IDLE,
	CHANNEL_STATE_START,
	CHANNEL_STATE_STARTING,
	CHANNEL_STATE_CHARGING,
	CHANNEL_STATE_STOPPING,
	CHANNEL_STATE_STOP,
} channel_state_t;

typedef enum {
	AC_CURRENT_LIMIT_16A = 0,
	AC_CURRENT_LIMIT_32A,
	AC_CURRENT_LIMIT_63A,
} ac_current_limit_t;

typedef enum {
	AUXILIARY_POWER_TYPE_12 = 0,
	AUXILIARY_POWER_TYPE_24
} auxiliary_power_type_t;

typedef enum {
	CHANNEL_FAULT_UNKNOW = 0,

	CHANNEL_FAULT_TELEMETER,
	CHANNEL_FAULT_CONNECT,
	CHANNEL_FAULT_FUNCTION_BOARD_CONNECT,
	CHANNEL_FAULT_OVER_TEMPERATURE,
	CHANNEL_FAULT_DC_BMS_GB_BRM_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_BCP_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_BRO_READY_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_BCS_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_BCL_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_BST_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_BSD_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_CRO_NOT_READY_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_INSULATION_CHECK_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_INSULATION_FAULT,
	CHANNEL_FAULT_DC_BMS_GB_DISCHARGE_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_DISCHARGE_FAULT,
	CHANNEL_FAULT_DC_BMS_GB_ADHESION_CHECK_TIMEOUT,
	CHANNEL_FAULT_DC_BMS_GB_ADHESION_FAULT,
	CHANNEL_FAULT_DC_BMS_GB_INSULATION_CHECK_WITH_ELECTRIFIED,
	CHANNEL_FAULT_DC_BMS_GB_ABNORMAL_VOLTAGE,
	CHANNEL_FAULT_AC_CHARGER_CONNECT_STATE_OFF,
	CHANNEL_FAULT_AC_CHARGER_CC1_READY_0_TIMEOUT,
	CHANNEL_FAULT_AC_CHARGER_CC1_READY_1_TIMEOUT,
	CHANNEL_FAULT_SIZE,
} channel_fault_t;

#pragma pack(push, 1)

typedef struct {
	uint8_t ac_current_limit;//ac_current_limit_t
	uint8_t auxiliary_power_type;//auxiliary_power_type_t
} channel_settings_t;

#pragma pack(pop)


typedef struct {
	channel_config_t *channel_config;
	uint8_t channel_id;
	void *channels_info;
	channel_handler_t *channel_handler;
	void *channel_handler_ctx;

	channel_settings_t channel_settings;

	bitmap_t *faults;//channel_fault_t

	channel_state_t request_state;
	channel_state_t state;

	uint8_t charger_connect_state;
	uint8_t charger_lock_state;
	uint8_t vehicle_relay_state;

	uint8_t bms_state;

	callback_item_t periodic_callback_item;
	callback_item_t event_callback_item;

	callback_chain_t *idle_chain;
	callback_chain_t *start_chain;
	callback_chain_t *starting_chain;
	callback_chain_t *charging_chain;
	callback_chain_t *stopping_chain;
	callback_chain_t *stop_chain;
	callback_chain_t *state_changed_chain;

	callback_chain_t *charger_connect_changed_chain;

	void *charger_info;
	void *energy_meter_info;

	uint16_t cp_ad;

	uint32_t total_energy;

	uint16_t va;
	uint16_t vb;
	uint16_t vc;
	uint16_t ca;
	uint16_t cb;
	uint16_t cc;

	channel_record_item_t channel_record_item;
} channel_info_t;

#pragma pack(push, 1)

typedef struct {
	uint32_t price[PRICE_ARRAY_SIZE];
	uint8_t seg[PRICE_SEGMENT_SIZE];
} price_info_t;

typedef struct {
	char device_id[32];
	uint8_t device_type;
	uint16_t power_module_type;
	uint16_t power_threshold;//单位 0.1kW
	uint8_t magnification;//电表放大倍率.0:2位小数, 1:3位小数
	price_info_t price_info;
} channels_settings_t;

#pragma pack(pop)

typedef enum {
	CHANNELS_FAULT_UNKNOW = 0,
	CHANNELS_FAULT_INSULATION,
	CHANNELS_FAULT_CARD_READER,
	CHANNELS_FAULT_DISPLAY,
	CHANNELS_FAULT_POWER_MODULES,
	CHANNELS_FAULT_DOOR,
	CHANNELS_FAULT_RELAY_ADHESION,
	CHANNELS_FAULT_FORCE_STOP,
	CHANNELS_FAULT_INPUT_OVER_VOLTAGE,
	CHANNELS_FAULT_INPUT_LOW_VOLTAGE,
	CHANNELS_FAULT_SIZE,
} channels_fault_t;

typedef struct {
	channels_config_t *channels_config;

	event_pool_t *event_pool;
	callback_chain_t *common_periodic_chain;
	callback_chain_t *common_event_chain;

	callback_item_t periodic_callback_item;
	callback_item_t event_callback_item;

	uint8_t configed;

	uint32_t periodic_stamp;

	uint8_t channel_number;
	channel_info_t *channel_info;
	void *card_reader_info;
	void *channels_power_module;
	channels_settings_t channels_settings;
	bitmap_t *faults;
	void *channel_comm_channel_info;
	void *channel_comm_channels_info;
	int8_t temperature;
} channels_info_t;

char *get_channel_event_type_des(channel_event_type_t type);
int set_fault(bitmap_t *faults, int fault, uint8_t v);
int get_fault(bitmap_t *faults, int fault);
int get_first_fault(bitmap_t *faults);
int send_channels_event(channels_info_t *channels_info, channels_event_t *channels_event, uint32_t timeout);
channels_info_t *start_channels(void);

#endif //_CHANNELS_H
