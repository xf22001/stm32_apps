

/*================================================================
 *   
 *   
 *   文件名称：relay_board_command.h
 *   创 建 者：肖飞
 *   创建日期：2020年07月06日 星期一 11时11分49秒
 *   修改日期：2020年07月07日 星期二 12时26分19秒
 *   描    述：
 *
 *================================================================*/
#ifndef _RELAY_BOARD_COMMAND_H
#define _RELAY_BOARD_COMMAND_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	RELAY_BOARD_CMD_RELAY_BOARD_HEARTBEAT = 0,
	RELAY_BOARD_CMD_RELAY_BOARDS_HEARTBEAT,
	RELAY_BOARD_CMD_TOTAL,
} relay_board_cmd_t;

#pragma pack(push, 1)

typedef struct {
	char buffer[64];
} relay_board_heartbeat_t;

typedef struct {
	char buffer[64];
} relay_boards_heartbeat_t;

#pragma pack(pop)

#endif //_RELAY_BOARD_COMMAND_H
