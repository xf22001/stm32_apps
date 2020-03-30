

/*================================================================
 *   
 *   
 *   文件名称：charger.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分46秒
 *   修改日期：2020年03月30日 星期一 11时38分35秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHARGER_H
#define _CHARGER_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "can_txrx.h"

#include "bms_spec.h"

#include "list_utils.h"

typedef enum {
	CHARGER_STATE_IDLE = 0,
	CHARGER_STATE_CHM,
	CHARGER_STATE_CRM,
	CHARGER_STATE_CTS_CML,
	CHARGER_STATE_CRO,
	CHARGER_STATE_CCS,
	CHARGER_STATE_CST,
	CHARGER_STATE_CSD_CEM,
} charger_state_t;

typedef struct {
	struct list_head list;
	
	can_info_t *can_info;
	charger_state_t state;
	osMutexId handle_mutex;

	multi_packets_info_t multi_packets_info;

	bms_data_settings_t *settings;

	uint32_t stamp;
	uint32_t stamp_1;
	uint32_t send_stamp;
	uint32_t send_stamp_1;
	uint32_t insulation_check_stamp;

	uint8_t bcl_received;
	uint8_t bcs_received;

	uint8_t brm_received;
} charger_info_t;

typedef int (*charger_handle_state_t)(charger_info_t *charger_info);

typedef struct {
	charger_state_t state;
	charger_handle_state_t prepare;
	charger_handle_state_t handle_request;
	charger_handle_state_t handle_response;
} charger_state_handler_t;

charger_info_t *get_charger_info(can_info_t *can_info);
void free_charger_info(charger_info_t *charger_info);
charger_info_t *alloc_charger_info(can_info_t *can_info);

charger_state_t get_charger_state(charger_info_t *charger_info);
void set_charger_state(charger_info_t *charger_info, charger_state_t state);
void set_charger_state_locked(charger_info_t *charger_info, charger_state_t state);
void charger_handle_request(charger_info_t *charger_info);
void charger_handle_response(charger_info_t *charger_info);
#endif //_CHARGER_H
