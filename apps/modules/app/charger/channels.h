

/*================================================================
 *
 *
 *   文件名称：channels.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 10时08分44秒
 *   修改日期：2021年05月28日 星期五 16时25分15秒
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

typedef struct {
	channel_type_t channel_type;
	channel_init_t init;
} channel_handler_t;

typedef struct {
	channel_config_t *channel_config;
	uint8_t channel_id;
	void *channels_info;
	channel_handler_t *channel_handler;

	callback_item_t periodic_callback_item;
	callback_item_t event_callback_item;

	void *charger_info;
	void *energy_meter_info;
} channel_info_t;

#pragma pack(push, 1)

typedef struct {
	char device_id[32];
	uint8_t device_type;
	uint16_t power_module_type;
} channels_settings_t;

#pragma pack(pop)

typedef enum {
	CHANNELS_FAULT_UNKNOW = 0,
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
} channels_info_t;

char *get_channel_event_type_des(channel_event_type_t type);
int set_channels_info_fault(channels_info_t *channels_info, channels_fault_t fault);
int reset_channels_info_fault(channels_info_t *channels_info, channels_fault_t fault);
int get_channels_info_fault(channels_info_t *channels_info, channels_fault_t fault);
int send_channels_event(channels_info_t *channels_info, channels_event_t *channels_event, uint32_t timeout);
channels_info_t *start_channels(void);

#endif //_CHANNELS_H
