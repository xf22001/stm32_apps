

/*================================================================
 *
 *
 *   文件名称：charger.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月19日 星期二 12时32分24秒
 *   修改日期：2021年06月04日 星期五 17时31分26秒
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

typedef int (*charger_bms_handle_init_t)(void *_charger_info);
typedef int (*charger_bms_handle_request_t)(void *_charger_info);
typedef int (*charger_bms_handle_response_t)(void *_charger_info);

typedef struct {
	channel_charger_type_t channel_charger_type;
	charger_bms_handle_init_t handle_init;
	charger_bms_handle_request_t handle_request;
	charger_bms_handle_response_t handle_response;
} charger_bms_handler_t;

typedef int (*charger_init_t)(void *_charger_info);

typedef struct {
	channel_charger_type_t charger_type;
	charger_init_t init;
} charger_handler_t;

typedef enum {
	CHARGER_BMS_REQUEST_ACTION_NONE = 0,
	CHARGER_BMS_REQUEST_ACTION_START,
	CHARGER_BMS_REQUEST_ACTION_STOP,
} charger_bms_request_action_t;

typedef enum {
	CHARGER_BMS_WORK_STATE_IDLE,
	CHARGER_BMS_WORK_STATE_STARTING,
	CHARGER_BMS_WORK_STATE_RUNNING,
	CHARGER_BMS_WORK_STATE_STOPPING,
} charger_bms_work_state_t;

typedef struct {
	channel_info_t *channel_info;

	os_mutex_t handle_mutex;

	can_info_t *bms_can_info;
	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;

	charger_handler_t *charger_handler;

	charger_bms_handler_t *charger_bms_handler;
	charger_bms_state_handler_t *charger_bms_state_handler;
	uint8_t state;
	uint8_t request_state;
	callback_chain_t *charger_bms_status_changed;

	charger_bms_work_state_t charger_bms_work_state;
	charger_bms_request_action_t charger_bms_request_action;

	uint8_t connect_state;
	uint32_t periodic_stamps;
} charger_info_t;

charger_info_t *alloc_charger_info(channel_info_t *channel_info);

#endif //_CHARGER_H
