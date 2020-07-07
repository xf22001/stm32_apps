

/*================================================================
 *   
 *   
 *   文件名称：relay_boards_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年07月06日 星期一 14时28分08秒
 *   修改日期：2020年07月07日 星期二 10时28分11秒
 *   描    述：
 *
 *================================================================*/
#ifndef _RELAY_BOARDS_COMMUNICATION_H
#define _RELAY_BOARDS_COMMUNICATION_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#include "os_utils.h"
#include "channels_config.h"
#include "can_command.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	struct list_head list;
	osMutexId handle_mutex;
	can_info_t *can_info;
	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;
	channels_info_config_t *channels_info_config;
	void *channels_info;

	uint8_t relay_board_number;

	can_com_cmd_ctx_t *cmd_ctx;

	void *relay_boards_com_data_ctx;

	can_com_connect_state_t *connect_state;

	uint32_t periodic_stamp;

} relay_boards_com_info_t;

void free_relay_boards_com_info(relay_boards_com_info_t *relay_boards_com_info);
relay_boards_com_info_t *get_or_alloc_relay_boards_com_info(channels_info_config_t *channels_info_config);
void relay_boards_com_request(relay_boards_com_info_t *relay_boards_com_info);
int relay_boards_com_response(relay_boards_com_info_t *relay_boards_com_info, can_rx_msg_t *can_rx_msg);

#endif //_RELAY_BOARDS_COMMUNICATION_H
