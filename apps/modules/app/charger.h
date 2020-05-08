

/*================================================================
 *
 *
 *   文件名称：charger.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分46秒
 *   修改日期：2020年05月08日 星期五 14时25分23秒
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

typedef enum {
	PRECHARGE_ACTION_STOP = 0,
	PRECHARGE_ACTION_START = 1,
	PRECHARGE_ACTION_START_SINGLE_MODULE = 2,
} precharge_action_t;

typedef struct {
	struct list_head list;

	can_info_t *can_info;
	charger_state_t state;
	osMutexId handle_mutex;

	channel_info_config_t *channel_info_config;
	void *a_f_b_info;
	void *channel_com_info;
	void *channel_info;

	multi_packets_info_t multi_packets_info;

	bms_data_settings_t *settings;

	callback_chain_t *report_status_chain;

	uint32_t stamp;
	uint32_t stamp_1;
	uint32_t stamp_2;
	uint32_t send_stamp;
	uint32_t send_stamp_1;
	uint32_t start_send_cst_stamp;

	charger_op_ctx_t charger_op_ctx;

	chm_op_state_t chm_op_state;
	cro_op_state_t cro_op_state;
	csd_cem_op_state_t csd_cem_op_state;

	uint8_t bhm_received;
	uint8_t brm_received;
	uint8_t bcp_received;
	uint8_t bcl_received;
	uint8_t bsm_received;
	uint8_t bcs_received;
	uint8_t bst_received;
	uint8_t bsd_received;

	uint16_t precharge_voltage;
	uint8_t precharge_action;//0-停止预充, 1-开始预充, 2-单模块预充

	uint8_t auxiliary_power_state;//辅助电源打开状态
	uint8_t gun_lock_state;//锁枪状态
	uint8_t power_output_state;//当前输出继电器打开状态
	uint8_t gun_connect_state;//插枪状态
	uint8_t gun_connect_state_debounce_count;//插枪状态防抖动值
	uint8_t gun_connect_state_update_stamp;
	uint8_t door_state;
	uint8_t error_stop_state;
	uint8_t gb;//标准
	uint8_t test_mode;//测试模式
	uint8_t precharge_enable;//允许预充
	uint8_t fault;//充电机故障
	uint8_t charger_power_on;//充电机主板开机状态
	uint8_t manual;//手动模式
	uint8_t adhesion_test;//粘连检测
	uint8_t double_gun_one_car;//双枪充一车
	uint8_t cp_ad;//cp-ad采样
	uint8_t auxiliary_power_type;//12-24v选择
	uint16_t module_output_voltage;//模块充电电压
	uint16_t charnnel_max_output_power;//通道最大输出功率
	uint16_t module_output_current;//模块充电电流
	uint8_t bms_connect_retry;

} charger_info_t;

typedef int (*charger_handle_state_t)(charger_info_t *charger_info);

typedef struct {
	charger_state_t state;
	charger_handle_state_t prepare;
	charger_handle_state_t handle_request;
	charger_handle_state_t handle_response;
} charger_state_handler_t;

typedef enum {
	CHARGER_INFO_STATUS_NONE = 0,
	CHARGER_INFO_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH,
	CHARGER_INFO_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT,
	CHARGER_INFO_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT,
	CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT,
	CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT,
	CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT,
	CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT,
	CHARGER_INFO_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT,
	CHARGER_INFO_STATUS_BRM_TIMEOUT,
	CHARGER_INFO_STATUS_BCP_TIMEOUT,
	CHARGER_INFO_STATUS_BRO_TIMEOUT,
	CHARGER_INFO_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH,
	CHARGER_INFO_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT,
	CHARGER_INFO_STATUS_BCL_TIMEOUT,
	CHARGER_INFO_STATUS_BCS_TIMEOUT,
	CHARGER_INFO_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT,
	CHARGER_INFO_STATUS_BRM_RECEIVED,
	CHARGER_INFO_STATUS_BCP_RECEIVED,
	CHARGER_INFO_STATUS_BCL_RECEIVED,
	CHARGER_INFO_STATUS_BSM_RECEIVED,
	CHARGER_INFO_STATUS_BCS_RECEIVED,
	CHARGER_INFO_STATUS_BST_RECEIVED,
	CHARGER_INFO_STATUS_BSD_RECEIVED,
} charger_info_status_t;

typedef struct {
	charger_state_t state;
	charger_info_status_t status;
} charger_report_status_t;

void free_charger_info(charger_info_t *charger_info);
charger_info_t *get_or_alloc_charger_info(channel_info_config_t *channel_info_config);
char *get_charger_state_des(charger_state_t state);

int add_charger_info_report_status_cb(charger_info_t *charger_info, callback_item_t *callback_item);
int remove_charger_info_report_status_cb(charger_info_t *charger_info, callback_item_t *callback_item);
void charger_info_report_status(charger_info_t *charger_info, charger_state_t state, charger_info_status_t status);
charger_state_t get_charger_state(charger_info_t *charger_info);
void set_charger_state(charger_info_t *charger_info, charger_state_t state);
void set_charger_state_locked(charger_info_t *charger_info, charger_state_t state);
void charger_handle_request(charger_info_t *charger_info);
void charger_handle_response(charger_info_t *charger_info);
void set_auxiliary_power_state(charger_info_t *charger_info, uint8_t state);
void set_gun_lock_state(charger_info_t *charger_info, uint8_t state);
void set_power_output_enable(charger_info_t *charger_info, uint8_t state);
int discharge(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int precharge(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int relay_endpoint_overvoltage_status(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int insulation_check(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int battery_voltage_status(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
int wait_no_current(charger_info_t *charger_info, charger_op_ctx_t *charger_op_ctx);
void charger_periodic(charger_info_t *charger_info);
#endif //_CHARGER_H
