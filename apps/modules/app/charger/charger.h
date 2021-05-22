

/*================================================================
 *
 *
 *   文件名称：charger.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月19日 星期二 12时32分24秒
 *   修改日期：2021年04月13日 星期二 14时01分43秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHARGER_H
#define _CHARGER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "can_txrx.h"
#include "callback_chain.h"
#include "channels_config.h"
#include "channels.h"

#ifdef __cplusplus
}
#endif

typedef int (*charger_bms_state_handle_t)(void *_charger_info);

typedef struct {
	uint8_t bms_state;
	charger_bms_state_handle_t prepare;
	charger_bms_state_handle_t handle_request;
	charger_bms_state_handle_t handle_response;
} charger_bms_state_handler_t;

typedef charger_bms_state_handler_t *(*get_charger_bms_state_handler_t)(uint8_t bms_state);

typedef struct {
	channel_charger_bms_type_t charger_bms_type;
	get_charger_bms_state_handler_t get_charger_bms_state_handler;
} charger_bms_handler_t;

typedef int (*charger_init_t)(void *_charger_info);

typedef struct {
	channel_charger_type_t charger_type;
	charger_init_t init;
} charger_handler_t;

typedef enum {
	CHARGER_BMS_REQUEST_NONE = 0,
	CHARGER_BMS_REQUEST_START,
	CHARGER_BMS_REQUEST_STOP,
} charger_bms_request_t;

typedef struct {
	channel_info_t *channel_info;

	can_info_t *bms_can_info;
	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;

	charger_handler_t *charger_handler;

	charger_bms_handler_t *charger_bms_handler;
	uint8_t bms_state;
	uint8_t bms_state_request;
	charger_bms_state_handler_t *charger_bms_state_handler_cache;
	callback_chain_t *charger_bms_status_changed;
	charger_bms_request_t bms_request;

	uint32_t periodic_stamps;
} charger_info_t;

charger_info_t *alloc_charger_info(channel_info_t *channel_info);

#endif //_CHARGER_H
