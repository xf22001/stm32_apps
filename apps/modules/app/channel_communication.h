

/*================================================================
 *
 *
 *   文件名称：channel_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分48秒
 *   修改日期：2020年05月18日 星期一 11时11分22秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_COMMUNICATION_H
#define _CHANNEL_COMMUNICATION_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#include "os_utils.h"
#include "channel_config.h"
#include "callback_chain.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	RETURN_ERROR = 0,         //无
	BRM_TIMEOUT,              //BRM报文超时
	BCP_TIMEOUT,              //BCP超时
	BRO_TIMEOUT,              //BRO超时
	BCS_TIMEOUT,              //BCS超时
	BCL_TIMEOUT,              //BCL超时
	BST_TIMEOUT,              //BST超时
	BSD_TIMEOUT,              //BSD超时
	BRO_ERROR,                //BRO未准备就绪
	BMS_SUCCESS,              //BMS通讯正常

	BMS_UNCONNECT,            //BMS未连接
	RETURN_INC_TIMEOUT,       //绝缘检测超时
	RETURN_INC_ERROR,         //绝缘故障
	RETURN_DISCHARGE_TIMEOUT, //泄放电路超时
	RETURN_DISCHARGE_ERROR,   //泄放失败
	RETURN_COCON_TIMEOUT,     //粘连检测超时
	RETURN_COCON_ERROR,       //粘连检测失败
	BCS_SUCCESS,              //接收BCS成功
	RETURN_INC_BMS_ERROR,     //检测绝缘时前段带电
	BMS_STARTING,             //正在启动

	CRO_TIMEOUT,              //20检测车上电压超时
	PRECHARGE_ERROR,          //预充错误
	BMS_U_UNMATCH,            //车辆电压不匹配
	SHORT_CIRCUIT_ERROR,      //短路故障
	BFC_ERR,                  //CFC数据错误
	BRO_ABNORMAL,             //BRO异常

	RETURN_SUCCESS = 0xff,
} bms_status_t;

typedef enum {
	CHANNEL_COM_CMD_1_101 = 0,
	CHANNEL_COM_CMD_2_102,
	CHANNEL_COM_CMD_13_113,
	CHANNEL_COM_CMD_3_103,
	CHANNEL_COM_CMD_4_104,
	CHANNEL_COM_CMD_5_105,
	CHANNEL_COM_CMD_6_106,
	CHANNEL_COM_CMD_7_107,
	CHANNEL_COM_CMD_8_108,
	CHANNEL_COM_CMD_9_109,
	CHANNEL_COM_CMD_10_110,
	CHANNEL_COM_CMD_11_111,
	CHANNEL_COM_CMD_20_120,
	CHANNEL_COM_CMD_21_121,
	CHANNEL_COM_CMD_22_122,
	CHANNEL_COM_CMD_25_125,
	CHANNEL_COM_CMD_30_130,
	CHANNEL_COM_CMD_50_150,
	CHANNEL_COM_CMD_51_151,
	CHANNEL_COM_CMD_60_160,
	CHANNEL_COM_CMD_61_161,
	CHANNEL_COM_CMD_62_162,
	CHANNEL_COM_CMD_63_163,
	CHANNEL_COM_CMD_64_164,
	CHANNEL_COM_CMD_65_165,
	CHANNEL_COM_CMD_66_166,
	CHANNEL_COM_CMD_67_167,
	CHANNEL_COM_CMD_68_168,
	CHANNEL_COM_CMD_69_169,
	CHANNEL_COM_CMD_70_170,
	CHANNEL_COM_CMD_71_171,
	CHANNEL_COM_CMD_72_172,
	CHANNEL_COM_CMD_73_173,
	CHANNEL_COM_CMD_TOTAL,
} channel_com_cmd_t;

typedef enum {
	CHANNEL_COM_STATE_IDLE = 0,
	CHANNEL_COM_STATE_REQUEST,
	CHANNEL_COM_STATE_RESPONSE,
	CHANNEL_COM_STATE_ERROR,
} channel_com_state_t;

typedef struct {
	channel_com_state_t state;
	uint32_t stamp;
	uint32_t retry;
	uint8_t available;
} channel_com_cmd_ctx_t;

typedef struct {
	struct list_head list;
	can_info_t *can_info;
	osMutexId handle_mutex;
	channel_info_config_t *channel_info_config;

	channel_com_cmd_ctx_t cmd_ctx[CHANNEL_COM_CMD_TOTAL];

	void *channel_info;
	void *charger_info;
	void *a_f_b_info;

	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;

	bms_status_t bms_status;
	callback_item_t charger_info_report_status_cb;

} channel_com_info_t;

void free_channel_com_info(channel_com_info_t *channel_com_info);
channel_com_info_t *get_or_alloc_channel_com_info(channel_info_config_t *channel_info_config);
void request_precharge(channel_com_info_t *channel_com_info);
void task_channel_com_request(void const *argument);
void task_channel_com_response(void const *argument);

#endif //_CHANNEL_COMMUNICATION_H
