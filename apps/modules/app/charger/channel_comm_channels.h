

/*================================================================
 *
 *
 *   文件名称：channel_comm_channels.h
 *   创 建 者：肖飞
 *   创建日期：2021年06月06日 星期日 15时02分58秒
 *   修改日期：2021年06月07日 星期一 10时44分12秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_COMM_CHANNELS_H
#define _CHANNEL_COMM_CHANNELS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#include "os_utils.h"
#include "can_txrx.h"
#include "channel_comm_command.h"
#include "can_command.h"
#include "callback_chain.h"
#include "connect_state.h"
#include "channels.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	os_mutex_t handle_mutex;
	can_info_t *can_info;
	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;
	void *channels_info;

	command_status_t *cmd_ctx;

	void *data_ctx;

	connect_state_t *connect_state;

	uint32_t periodic_stamp;

	uint8_t channel_comm_number;

	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;
} channel_comm_channels_info_t;

uint8_t channel_comms_channels_get_connect_state(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t channel_comm_id);
uint32_t channel_comms_channels_get_connect_stamp(channel_comm_channels_info_t *channel_comm_channels_info, uint8_t channel_comm_id);
int start_channel_comm_channels(channels_info_t *channels_info);

#endif //_CHANNEL_COMM_CHANNELS_H
