

/*================================================================
 *
 *
 *   文件名称：charger.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分46秒
 *   修改日期：2020年04月27日 星期一 16时05分25秒
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

#include "charger_config.h"

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

typedef enum {
	CHM_OP_STATE_NONE = 0,
	CHM_OP_STATE_DISCHARGE,
	CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK,
	CHM_OP_STATE_INSULATION_CHECK_PRECHARGE,
	CHM_OP_STATE_INSULATION_CHECK_DELAY_1,
	CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE,
	CHM_OP_STATE_INSULATION_CHECK_DELAY_2,
	CHM_OP_STATE_INSULATION_CHECK_DISCHARGE,
	CHM_OP_STATE_INSULATION_CHECK,
	CHM_OP_STATE_ABORT_DISCHARGE,
} chm_op_state_t;

typedef enum {
	CRO_OP_STATE_NONE = 0,
	CRO_OP_STATE_START_PRECHARGE,
	CRO_OP_STATE_GET_BATTERY_STATUS,
	CRO_OP_STATE_PRECHARGE,
	CRO_OP_STATE_PRECHARGE_DELAY_1,
	CRO_OP_STATE_PRECHARGE_DELAY_2,
} cro_op_state_t;

typedef enum {
	CSD_CEM_OP_STATE_NONE = 0,
	CSD_CEM_OP_STATE_WAIT_NO_CURRENT,
	CSD_CEM_OP_STATE_DISABLE_OUTPUT_DELAY,
	CSD_CEM_OP_STATE_DISCHARGE,
	CSD_CEM_OP_STATE_DISCHARGE_CHECK,
	CSD_CEM_OP_STATE_FINISH,
} csd_cem_op_state_t;

typedef struct {
	struct list_head list;

	can_info_t *can_info;
	charger_state_t state;
	osMutexId handle_mutex;

	charger_info_config_t *charger_info_config;

	charger_op_ctx_t charger_op_ctx;

	multi_packets_info_t multi_packets_info;

	bms_data_settings_t *settings;

	uint32_t stamp;
	uint32_t stamp_1;
	uint32_t stamp_2;
	uint32_t send_stamp;
	uint32_t send_stamp_1;

	uint8_t bhm_received;
	chm_op_state_t chm_op_state;
	uint16_t precharge_voltage;

	cro_op_state_t cro_op_state;

	uint32_t start_send_cst_stamp;

	csd_cem_op_state_t csd_cem_op_state;

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

void free_charger_info(charger_info_t *charger_info);
charger_info_t *get_or_alloc_charger_info(can_info_t *can_info);

charger_state_t get_charger_state(charger_info_t *charger_info);
void set_charger_state(charger_info_t *charger_info, charger_state_t state);
void set_charger_state_locked(charger_info_t *charger_info, charger_state_t state);
void charger_handle_request(charger_info_t *charger_info);
void charger_handle_response(charger_info_t *charger_info);

void charger_set_auxiliary_power_state(charger_info_t *charger_info, uint8_t on_off);
void charger_set_gun_lock_state(charger_info_t *charger_info, uint8_t on_off);
#endif //_CHARGER_H
