

/*================================================================
 *   
 *   
 *   文件名称：channels_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月25日 星期一 14时24分10秒
 *   修改日期：2020年05月25日 星期一 16时56分10秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_COMMUNICATION_H
#define _CHANNELS_COMMUNICATION_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#include "os_utils.h"
#include "channels_config.h"
#include "bms_status.h"

#ifdef __cplusplus
}
#endif

#define CHANNELS_COM_CONNECT_STATE_SIZE 10

typedef enum {
	CHANNELS_COM_STATE_IDLE = 0,
	CHANNELS_COM_STATE_REQUEST,
	CHANNELS_COM_STATE_RESPONSE,
	CHANNELS_COM_STATE_ERROR,
} channels_com_state_t;

typedef struct {
	uint8_t channel_id;
	channels_com_state_t state;
	uint32_t stamp;
	uint32_t send_stamp;
	uint8_t available;
} channels_com_cmd_ctx_t;

typedef struct {
	struct list_head list;
	can_info_t *can_info;
	osMutexId handle_mutex;
	channels_info_config_t *channels_info_config;

	channels_com_cmd_ctx_t *cmd_ctx;

	void *channels_info;
	void *charger_info;
	void *a_f_b_info;

	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;

	bms_status_t bms_status;

	uint8_t connect_state[CHANNELS_COM_CONNECT_STATE_SIZE];
	uint8_t connect_state_index;

} channels_com_info_t;

void free_channels_com_info(channels_com_info_t *channels_com_info);
channels_com_info_t *get_or_alloc_channels_com_info(channels_info_config_t *channels_info_config);

#endif //_CHANNELS_COMMUNICATION_H
