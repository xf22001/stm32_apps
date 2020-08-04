

/*================================================================
 *   
 *   
 *   文件名称：can_command.h
 *   创 建 者：肖飞
 *   创建日期：2020年07月07日 星期二 08时26分08秒
 *   修改日期：2020年08月03日 星期一 13时30分05秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CAN_COMMAND_H
#define _CAN_COMMAND_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#ifdef __cplusplus
}
#endif

#define CAN_COM_CONNECT_STATE_SIZE 10

typedef enum {
	CAN_COM_STATE_IDLE = 0,
	CAN_COM_STATE_REQUEST,
	CAN_COM_STATE_RESPONSE,
	CAN_COM_STATE_ERROR,
} can_com_com_state_t;

typedef enum {
	CAN_COM_RESPONSE_STATUS_WAIT = 0,
	CAN_COM_RESPONSE_STATUS_DONE,
} can_com_response_status_t;

#pragma pack(push, 1)

typedef struct {
	uint8_t cmd;
	uint8_t index;
	uint8_t data[6];
} can_com_cmd_common_t;

typedef struct {
	uint8_t cmd;
	uint8_t index;
	can_com_response_status_t response_status;
} can_com_cmd_response_t;

#pragma pack(pop)

typedef struct {
	can_com_com_state_t state;
	uint32_t stamp;
	uint32_t send_stamp;
	uint8_t available;
	uint8_t index;
} can_com_cmd_ctx_t;

typedef struct {
	uint8_t state[CAN_COM_CONNECT_STATE_SIZE];
	uint8_t index;
	uint32_t update_stamp;
} can_com_connect_state_t;

char *get_can_com_state_des(can_com_com_state_t state);
char *get_can_com_response_status_des(can_com_response_status_t status);
int can_com_prepare_request(can_com_cmd_ctx_t *can_com_cmd_ctx, can_com_cmd_common_t *can_com_cmd_common, uint8_t cmd, uint8_t *data, uint8_t data_size);
int can_com_process_response(can_com_cmd_ctx_t *can_com_cmd_ctx, can_com_cmd_response_t *can_com_cmd_response, uint8_t cmd, uint8_t data_size);
int can_com_prepare_response(can_com_cmd_ctx_t *can_com_cmd_ctx, can_com_cmd_response_t *can_com_cmd_response, uint8_t cmd, uint8_t data_size);
int can_com_process_request(can_com_cmd_ctx_t *can_com_cmd_ctx, can_com_cmd_common_t *can_com_cmd_common, uint8_t cmd, uint8_t *data, uint8_t data_size);
void can_com_set_connect_state(can_com_connect_state_t *can_com_connect_state, uint8_t state);
uint8_t can_com_get_connect_state(can_com_connect_state_t *can_com_connect_state);
uint32_t can_com_get_connect_stamp(can_com_connect_state_t *can_com_connect_state);

#endif //_CAN_COMMAND_H
