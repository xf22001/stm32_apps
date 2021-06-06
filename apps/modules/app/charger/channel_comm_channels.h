

/*================================================================
 *
 *
 *   文件名称：channel_comm_channels.h
 *   创 建 者：肖飞
 *   创建日期：2021年06月06日 星期日 15时02分58秒
 *   修改日期：2021年06月06日 星期日 17时17分54秒
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

#endif //_CHANNEL_COMM_CHANNELS_H
