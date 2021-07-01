

/*================================================================
 *
 *
 *   文件名称：channels.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 10时08分44秒
 *   修改日期：2021年07月01日 星期四 13时45分59秒
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
	uint8_t reason;
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
} channel_handler_t;

typedef enum {
	CHANNEL_STATE_NONE = 0,
	CHANNEL_STATE_IDLE,
	CHANNEL_STATE_START,
	CHANNEL_STATE_STARTING,
	CHANNEL_STATE_CHARGING,
	CHANNEL_STATE_STOP,
	CHANNEL_STATE_STOPPING,
	CHANNEL_STATE_END,
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
	CHANNEL_FAULT_ADHESION_L,
	CHANNEL_FAULT_ADHESION_N,
	CHANNEL_FAULT_SIZE,
} channel_fault_t;

#pragma pack(push, 1)

typedef struct {
	uint8_t type;//channel_charger_type_t
} channel_charger_settings_t;

typedef struct {
	uint8_t type;//channel_energy_meter_type_t
} channel_energy_meter_settings_t;

typedef struct {
	uint8_t channel_type;//channel_type_t
	channel_charger_settings_t charger_settings;
	channel_energy_meter_settings_t energy_meter_settings;

	uint8_t ac_current_limit;//ac_current_limit_t
} channel_settings_t;

#pragma pack(pop)

typedef struct {
	channel_config_t *channel_config;
	uint8_t channel_id;
	void *channels_info;
	channel_handler_t *channel_handler;
	void *channel_handler_ctx;
	void *channel_record_handler_ctx;

	channel_settings_t channel_settings;

	bitmap_t *faults;//channel_fault_t

	channel_state_t request_state;
	channel_state_t state;

	uint8_t charger_lock_state;

	uint8_t bms_state;

	callback_item_t periodic_callback_item;
	callback_item_t event_callback_item;

	callback_chain_t *idle_chain;
	callback_chain_t *start_chain;
	callback_chain_t *starting_chain;
	callback_chain_t *charging_chain;
	callback_chain_t *stop_chain;
	callback_chain_t *stopping_chain;
	callback_chain_t *end_chain;
	callback_chain_t *state_changed_chain;

	callback_chain_t *charger_connect_changed_chain;

	void *charger_info;
	void *energy_meter_info;

	uint16_t temperature_ad;
	uint16_t cp_ad;
	uint16_t adhe_ad;

	uint32_t total_energy;//0.0001kwh
	uint32_t voltage;
	uint32_t current;

	uint16_t va;
	uint16_t vb;
	uint16_t vc;
	uint16_t ca;
	uint16_t cb;
	uint16_t cc;

	channel_record_item_t channel_record_item;

	uint8_t auxiliary_power_type;//auxiliary_power_type_t
} channel_info_t;

#pragma pack(push, 1)

typedef struct {
	uint32_t service_price;
	uint32_t price[PRICE_SEGMENT_SIZE];
} price_info_t;

typedef struct {
	uint8_t channels_power_module_number;
	uint8_t channels_power_module_type;//channels_power_module_type_t
} channels_power_module_settings_t;

typedef struct {
	uint8_t type;//card_reader_type_t
} card_reader_settings_t;

typedef struct {
	uint8_t channel_number;
	channels_power_module_settings_t channels_power_module_settings;
	card_reader_settings_t card_reader_settings;

	char device_id[32];
	uint8_t device_type;
	uint16_t power_module_type;
	uint16_t power_threshold;//单位 0.1kW
	uint8_t magnification;//电表放大倍率.0:2位小数, 1:3位小数
	price_info_t price_info;
	uint32_t withholding;//告诉后台此卡的预扣款是多少 0.01 元
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
	CHANNELS_FAULT_ELECTRIC_LEAK_CALIBRATION,
	CHANNELS_FAULT_ELECTRIC_LEAK_PROTECT,
	CHANNELS_FAULT_PE_PROTECT,
	CHANNELS_FAULT_SIZE,
} channels_fault_t;

typedef enum {
	ELECTRIC_LEAKAGE_DETECT_TYPE_UNKNOW = 0,
	ELECTRIC_LEAKAGE_DETECT_TYPE_A,
	ELECTRIC_LEAKAGE_DETECT_TYPE_B,
	ELECTRIC_LEAKAGE_DETECT_TYPE_C,
} electric_leakage_detect_type_t;

typedef enum {
	ELECTRIC_LEAKAGE_DETECT_B_STATE_CAL_PREPARE = 0,//cal to high, test to low, trip to low
	ELECTRIC_LEAKAGE_DETECT_B_STATE_CAL_START,//cal keep high >= 100ms, then cal to low
	ELECTRIC_LEAKAGE_DETECT_B_STATE_TEST_PREPARE,//cal 100ms >= keep >= 50ms, then cal to high
	ELECTRIC_LEAKAGE_DETECT_B_STATE_TEST_START,//test keep low >=500ms, then to high
	ELECTRIC_LEAKAGE_DETECT_B_STATE_WAIT_TRIP,//test keep high 400ms, then to low, wait trip;
	ELECTRIC_LEAKAGE_DETECT_B_STATE_PREPARE_DETECT,//wait trip to low = 100ms
	ELECTRIC_LEAKAGE_DETECT_B_STATE_DETECT,//wait trip
	ELECTRIC_LEAKAGE_DETECT_B_STATE_ERROR,
} electric_leakage_detect_b_state_t;

typedef struct {
	channels_config_t *channels_config;

	eeprom_info_t *eeprom_info;

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
	bitmap_t *faults;//channels_fault_t
	void *channel_comm_channel_info;
	void *channel_comm_channels_info;
	int8_t temperature;

	uint8_t electric_leakage_detect_type;//electric_leakage_detect_type_t;
	uint8_t electric_leakage_detect_b_state;//electric_leakage_detect_b_state_t
	uint32_t electric_leakage_detect_b_stamps;
} channels_info_t;

char *get_channel_event_type_des(channel_event_type_t type);
int set_fault(bitmap_t *faults, int fault, uint8_t v);
int get_fault(bitmap_t *faults, int fault);
int get_first_fault(bitmap_t *faults);
int send_channels_event(channels_info_t *channels_info, channels_event_t *channels_event, uint32_t timeout);
channels_info_t *start_channels(void);

#endif //_CHANNELS_H
