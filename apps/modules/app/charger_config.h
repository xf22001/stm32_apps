

/*================================================================
 *   
 *   
 *   文件名称：charger_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月18日 星期六 12时33分39秒
 *   修改日期：2020年04月28日 星期二 08时35分15秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHARGER_CONFIG_H
#define _CHARGER_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "can_txrx.h"

#ifdef __cplusplus
}
#endif

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

typedef struct {
	uint32_t stamp;
	uint8_t state;
} charger_op_ctx_t;

typedef enum {
	CHARGER_STATUS_NONE = 0,
	CHARGER_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH,
	CHARGER_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT,
	CHARGER_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT,
	CHARGER_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT,
	CHARGER_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT,
	CHARGER_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT,
	CHARGER_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT,
	CHARGER_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT,
	CHARGER_STATUS_BRM_TIMEOUT,
	CHARGER_STATUS_BCP_TIMEOUT,
	CHARGER_STATUS_BRO_TIMEOUT,
	CHARGER_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH,
	CHARGER_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT,
	CHARGER_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT,
} charger_status_t;

typedef void (*report_charger_status_t)(charger_status_t charger_status);
typedef void (*set_auxiliary_power_state_t)(uint8_t state);
typedef void (*set_gun_lock_state_t)(uint8_t state);
typedef void (*set_output_power_enable_t)(uint8_t enable);
typedef int (*discharge_t)(charger_op_ctx_t *charger_op_ctx);
typedef int (*precharge_t)(uint16_t voltage, charger_op_ctx_t *charger_op_ctx);
typedef int (*relay_endpoint_overvoltage_status_t)(charger_op_ctx_t *charger_op_ctx);
typedef int (*insulation_check_t)(charger_op_ctx_t *charger_op_ctx);
typedef int (*battery_voltage_status_t)(charger_op_ctx_t *charger_op_ctx);
typedef int (*wait_no_current_t)(charger_op_ctx_t *charger_op_ctx);

typedef struct {
	CAN_HandleTypeDef *hcan;
	report_charger_status_t report_charger_status;
	set_auxiliary_power_state_t set_auxiliary_power_state;
	set_gun_lock_state_t set_gun_lock_state;
	set_output_power_enable_t set_power_output_enable;
	discharge_t discharge;
	precharge_t precharge;
	relay_endpoint_overvoltage_status_t relay_endpoint_overvoltage_status;
	insulation_check_t insulation_check;
	battery_voltage_status_t battery_voltage_status;
	wait_no_current_t wait_no_current;
} charger_info_config_t;

extern charger_info_config_t charger_info_config_can1;
extern charger_info_config_t charger_info_config_can2;
#endif //_CHARGER_CONFIG_H
