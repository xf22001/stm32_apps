

/*================================================================
 *
 *
 *   文件名称：charger.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分46秒
 *   修改日期：2020年05月06日 星期三 09时25分49秒
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

#include "channel_config.h"

#include "callback_chain.h"

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

	channel_info_config_t *channel_info_config;

	void *a_f_b_info;

	void *channel_com_info;

	void *channel_info;

	charger_op_ctx_t charger_op_ctx;

	multi_packets_info_t multi_packets_info;

	bms_data_settings_t *settings;

	callback_chain_t *report_status_chain;

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

	uint8_t auxiliary_power_state;
	uint8_t gun_lock_state;
	uint8_t power_output_state;
} charger_info_t;

typedef int (*charger_handle_state_t)(charger_info_t *charger_info);

typedef struct {
	charger_state_t state;
	charger_handle_state_t prepare;
	charger_handle_state_t handle_request;
	charger_handle_state_t handle_response;
} charger_state_handler_t;

typedef enum {
	CHARGER_ERROR_STATUS_NONE = 0,
	CHARGER_ERROR_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH,
	CHARGER_ERROR_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT,
	CHARGER_ERROR_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT,
	CHARGER_ERROR_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT,
	CHARGER_ERROR_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT,
	CHARGER_ERROR_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT,
	CHARGER_ERROR_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT,
	CHARGER_ERROR_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT,
	CHARGER_ERROR_STATUS_BRM_TIMEOUT,
	CHARGER_ERROR_STATUS_BCP_TIMEOUT,
	CHARGER_ERROR_STATUS_BRO_TIMEOUT,
	CHARGER_ERROR_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH,
	CHARGER_ERROR_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT,
	CHARGER_ERROR_STATUS_BCL_TIMEOUT,
	CHARGER_ERROR_STATUS_BCS_TIMEOUT,
	CHARGER_ERROR_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT,
} charger_info_error_status_t;

typedef struct {
	charger_state_t state;
	charger_info_error_status_t error_status;
} charger_report_status_t;

void free_charger_info(charger_info_t *charger_info);
charger_info_t *get_or_alloc_charger_info(channel_info_config_t *channel_info_config);

int add_charger_info_report_status_cb(charger_info_t *charger_info, callback_item_t *callback_item);
int remove_charger_info_report_status_cb(charger_info_t *charger_info, callback_item_t *callback_item);
void charger_info_report_status(charger_info_t *charger_info, charger_info_error_status_t error_status);
charger_state_t get_charger_state(charger_info_t *charger_info);
void set_charger_state(charger_info_t *charger_info, charger_state_t state);
void set_charger_state_locked(charger_info_t *charger_info, charger_state_t state);
void charger_handle_request(charger_info_t *charger_info);
void charger_handle_response(charger_info_t *charger_info);
void set_auxiliary_power_state(charger_info_t *charger_info, uint8_t state);
void set_gun_lock_state(charger_info_t *charger_info, uint8_t state);
void set_power_output_enable(charger_info_t *charger_info, uint8_t state);
int discharge(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int precharge(charger_info_t *charger_info, uint16_t voltage, charger_op_ctx_t *charger_op_ctx);
int relay_endpoint_overvoltage_status(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int insulation_check(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int battery_voltage_status(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int wait_no_current(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
#endif //_CHARGER_H
