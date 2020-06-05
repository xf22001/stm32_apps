

/*================================================================
 *
 *
 *   文件名称：channels_communication.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月25日 星期一 14时24分10秒
 *   修改日期：2020年06月05日 星期五 09时39分56秒
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
#include "channel_command.h"

#ifdef __cplusplus
}
#endif

#define CHANNELS_COM_CONNECT_STATE_SIZE 10

typedef struct {
	uint32_t main_board_id : 8;//src 0xff
	uint32_t channel_id : 8;//dest
	uint32_t unused : 8;
	uint32_t flag : 5;//0x10
	uint32_t unused1 : 3;
} channels_com_can_tx_id_t;

typedef union {
	channels_com_can_tx_id_t s;
	uint32_t v;
} u_channels_com_can_tx_id_t;

typedef struct {
	uint32_t channel_id : 8;//src
	uint32_t main_board_id : 8;//dest 0xff
	uint32_t unused : 8;
	uint32_t flag : 5;//0x10
	uint32_t unused1 : 3;
} channels_com_can_rx_id_t;

typedef union {
	channels_com_can_rx_id_t s;
	uint32_t v;
} u_channels_com_can_rx_id_t;

typedef struct {
	uint8_t channel_id;
	channel_com_state_t state;
	uint32_t stamp;
	uint32_t send_stamp;
	uint8_t available;
	uint8_t index;
} channels_com_cmd_ctx_t;

typedef struct {
	uint8_t state[CHANNELS_COM_CONNECT_STATE_SIZE];
	uint8_t index;
} connect_state_t;

typedef struct {
	struct list_head list;
	can_info_t *can_info;
	osMutexId handle_mutex;
	channels_info_config_t *channels_info_config;

	channels_com_cmd_ctx_t *cmd_ctx;

	void *channels_info;

	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;

	void *channels_com_data_ctx;

	connect_state_t *connect_state;

	uint32_t periodic_stamp;

} channels_com_info_t;

void free_channels_com_info(channels_com_info_t *channels_com_info);
channels_com_info_t *get_or_alloc_channels_com_info(channels_info_config_t *channels_info_config);
void task_channels_com_request(void const *argument);
void task_channels_com_response(void const *argument);

#endif //_CHANNELS_COMMUNICATION_H
