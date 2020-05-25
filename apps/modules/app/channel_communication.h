

/*================================================================
 *
 *
 *   文件名称：channel_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分48秒
 *   修改日期：2020年05月25日 星期一 15时32分41秒
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
#include "bms_status.h"

#ifdef __cplusplus
}
#endif

#define CHANNEL_COM_CONNECT_STATE_SIZE 10

typedef enum {
	CHANNEL_COM_STATE_IDLE = 0,
	CHANNEL_COM_STATE_REQUEST,
	CHANNEL_COM_STATE_RESPONSE,
	CHANNEL_COM_STATE_ERROR,
} channel_com_state_t;

typedef struct {
	channel_com_state_t state;
	uint32_t stamp;
	uint32_t send_stamp;
	uint8_t available;
} channel_com_cmd_ctx_t;

typedef struct {
	struct list_head list;
	can_info_t *can_info;
	osMutexId handle_mutex;
	channel_info_config_t *channel_info_config;

	channel_com_cmd_ctx_t *cmd_ctx;

	void *channel_info;
	void *charger_info;
	void *a_f_b_info;

	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;

	bms_status_t bms_status;
	callback_item_t charger_info_report_status_cb;

	uint8_t connect_state[CHANNEL_COM_CONNECT_STATE_SIZE];
	uint8_t connect_state_index;

} channel_com_info_t;

void free_channel_com_info(channel_com_info_t *channel_com_info);
channel_com_info_t *get_or_alloc_channel_com_info(channel_info_config_t *channel_info_config);
void request_precharge(channel_com_info_t *channel_com_info);
void task_channel_com_request(void const *argument);
void task_channel_com_response(void const *argument);

#endif //_CHANNEL_COMMUNICATION_H
