

/*================================================================
 *   
 *   
 *   文件名称：relay_board_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年07月06日 星期一 17时08分58秒
 *   修改日期：2020年07月07日 星期二 13时08分01秒
 *   描    述：
 *
 *================================================================*/
#ifndef _RELAY_BOARD_COMMUNICATION_H
#define _RELAY_BOARD_COMMUNICATION_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#include "os_utils.h"
#include "channel_config.h"
#include "callback_chain.h"
#include "can_command.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	struct list_head list;
	os_mutex_t handle_mutex;
	can_info_t *can_info;
	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;

	channel_info_config_t *channel_info_config;

	can_com_cmd_ctx_t *cmd_ctx;

	void *relay_board_com_data_ctx;

	can_com_connect_state_t connect_state;

	uint32_t periodic_stamp;
} relay_board_com_info_t;

void free_relay_board_com_info(relay_board_com_info_t *relay_board_com_info);
relay_board_com_info_t *get_or_alloc_relay_board_com_info(channel_info_config_t *channel_info_config);
void task_relay_board_com_request(void const *argument);
void task_relay_board_com_response(void const *argument);

#endif //_RELAY_BOARD_COMMUNICATION_H
