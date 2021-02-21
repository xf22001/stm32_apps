

/*================================================================
 *   
 *   
 *   文件名称：command_status.h
 *   创 建 者：肖飞
 *   创建日期：2020年06月17日 星期三 15时40分38秒
 *   修改日期：2021年02月21日 星期日 19时37分25秒
 *   描    述：
 *
 *================================================================*/
#ifndef _COMMAND_STATUS_H
#define _COMMAND_STATUS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include "app.h"
#include "app_platform.h"
#include "cmsis_os.h"
#include "os_utils.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	COMMAND_STATE_IDLE = 0,
	COMMAND_STATE_REQUEST,
	COMMAND_STATE_RESPONSE,
	COMMAND_STATE_ERROR,
} command_state_t;

typedef struct {
	command_state_t state;
	uint32_t update_stamp;
	uint8_t available;
} command_status_t;

static inline int validate_command_status(command_status_t *status, uint32_t ticks, uint32_t timeout)
{
	int ret = -1;

	if(status->available == 0) {
		return ret;
	}

	if(ticks_duration(ticks, status->update_stamp) >= timeout) {
		status->available = 0;
		return ret;
	}

	ret = 0;

	return ret;
}
#endif //_COMMAND_STATUS_H
