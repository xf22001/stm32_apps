

/*================================================================
 *   
 *   
 *   文件名称：can_command.h
 *   创 建 者：肖飞
 *   创建日期：2020年07月07日 星期二 08时26分08秒
 *   修改日期：2021年04月13日 星期二 17时28分53秒
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

#include "command_status.h"

#ifdef __cplusplus
}
#endif

#define CAN_COM_CONNECT_STATE_SIZE 10

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

char *get_can_com_response_status_des(can_com_response_status_t status);
int can_com_prepare_tx_request(command_status_t *command_status, can_com_cmd_common_t *can_com_cmd_common, uint8_t cmd, uint8_t *data, uint8_t data_size);
int can_com_process_rx_response(command_status_t *command_status, can_com_cmd_response_t *can_com_cmd_response, uint8_t cmd, uint8_t data_size);
int can_com_prepare_tx_response(command_status_t *command_status, can_com_cmd_response_t *can_com_cmd_response, uint8_t cmd, uint8_t data_size);
int can_com_process_rx_request(command_status_t *command_status, can_com_cmd_common_t *can_com_cmd_common, uint8_t cmd, uint8_t *data, uint8_t data_size);
int can_com_prepare_tx_request_broadcast(command_status_t *command_status, can_com_cmd_common_t *can_com_cmd_common, uint8_t cmd, uint8_t *data, uint8_t data_size);

#endif //_CAN_COMMAND_H
