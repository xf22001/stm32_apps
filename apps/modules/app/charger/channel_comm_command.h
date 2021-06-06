

/*================================================================
 *   
 *   
 *   文件名称：channel_comm_command.h
 *   创 建 者：肖飞
 *   创建日期：2021年06月06日 星期日 15时37分40秒
 *   修改日期：2021年06月06日 星期日 17时10分32秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_COMM_COMMAND_H
#define _CHANNEL_COMM_COMMAND_H
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
	CHANNEL_COMM_CMD_CHANNEL_HEARTBEAT = 0,
	CHANNEL_COMM_CMD_CHANNELS_HEARTBEAT,
	CHANNEL_COMM_CMD_TOTAL,
} channel_comm_cmd_t;

#pragma pack(push, 1)

typedef struct {
	uint8_t magic;
} channels_heartbeat_t;

typedef struct {
	uint8_t magic;
} channel_heartbeat_t;

#pragma pack(pop)

#endif //_CHANNEL_COMM_COMMAND_H
