

/*================================================================
 *
 *
 *   文件名称：charger.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分46秒
 *   修改日期：2020年04月17日 星期五 17时35分23秒
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

typedef enum {
	RETURN_ERROR = 0,
	BRM_TIMEOUT,
	BCP_TIMEOUT,
	BRO_TIMEOUT,
	BCS_TIMEOUT,
	BCL_TIMEOUT,
	BST_TIMEOUT,
	BSD_TIMEOUT,
	BRO_ERROR,
	BMS_SUCCESS,
	BMS_UNCONNECT,
	RETURN_INC_TIMEOUT,       //绝缘检测超时
	RETURN_INC_ERROR,         //绝缘故障
	RETURN_DISCHARGE_TIMEOUT, //泄放电路超时
	RETURN_DISCHARGE_ERROR,   //泄放失败
	RETURN_COCON_TIMEOUT,     //粘连检测超时
	RETURN_COCON_ERROR,       //粘连检测失败
	BCS_SUCCESS,              //接收BCS成功
	RETURN_INC_BMS_ERROR,     //检测绝缘时前段带电
	BMS_STARTING,             //BMS正在启动
	CRO_TIMEOUT,              //检测车上电压超时
	PRECHARGE_ERROR,          //预充错误
	BMS_U_UNMATCH,            //车辆电压不匹配
	SHORT_CIRCUIT_ERROR,      //短路故障
	BFC_ERR,                  //CFC数据错误
	BRO_ABNORMAL,             //BRO异常

	RETURN_SUCCESS = 0xff,
} charger_bms_error_t;

typedef enum {
	CHM_OP_STATE_NONE = 0,
	CHM_OP_STATE_DISCHARGE,
	CHM_OP_STATE_DISCHARGE_CHECK,
	CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK,
	CHM_OP_STATE_INSULATION_CHECK_PRECHARGE,
	CHM_OP_STATE_INSULATION_CHECK_CHARGE,
	CHM_OP_STATE_INSULATION_CHECK_DELAY,
	CHM_OP_STATE_INSULATION_CHECK_START,
	CHM_OP_STATE_INSULATION_CHECK_TEST,
	CHM_OP_STATE_INSULATION_CHECK_FINISH_DISCHARGE,
	CHM_OP_STATE_INSULATION_CHECK_FINISH,
	CHM_OP_STATE_NEXT_STATE,
} chm_op_state_t;

typedef enum {
	CRO_OP_STATE_NONE = 0,
	CRO_OP_STATE_PREPARE_PRECHARGE,
	CRO_OP_STATE_PRECHARGE,
	CRO_OP_STATE_DELAY,
} cro_op_state_t;

typedef struct {
	struct list_head list;

	can_info_t *can_info;
	charger_state_t state;
	osMutexId handle_mutex;

	multi_packets_info_t multi_packets_info;

	bms_data_settings_t *settings;
	charger_bms_error_t charger_bms_error;

	uint32_t stamp;
	uint32_t stamp_1;
	uint32_t send_stamp;
	uint32_t send_stamp_1;

	uint8_t bhm_received;
	chm_op_state_t chm_op_state;
	chm_op_state_t chm_op_state_for_discharge;
	uint16_t precharge_voltage;

	cro_op_state_t cro_op_state;

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
#endif //_CHARGER_H
