

/*================================================================
 *
 *
 *   文件名称：channels.h
 *   创 建 者：肖飞
 *   创建日期：2020年01月02日 星期四 08时53分41秒
 *   修改日期：2020年04月04日 星期六 18时10分22秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_H
#define _CHANNELS_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "event_helper.h"
#include "list_utils.h"

#define CHANNEL_INSTANCES_NUMBER 4
#define CHANNEL_TASK_PERIODIC (50)

typedef enum {
	CHANNEL_STATE_IDLE = 0,
	CHANNEL_STATE_DO_START,
	CHANNEL_STATE_RUNNING,
	CHANNEL_STATE_DO_STOP,
} channel_state_t;

typedef enum {
	CHANNEL_EVENT_TYPE_UNKNOW = 0,
} channel_event_type_t;

typedef struct {
	uint8_t channel_id;
	channel_event_type_t type;
} channel_event_t;

typedef int (*handle_channel_event_t)(channel_event_t *channel_event);

typedef struct {
	uint8_t channel_id;
	channel_state_t state;
	handle_channel_event_t handle_channel_event;
} channel_info_t;

typedef struct {
	struct list_head list;
	event_pool_t *event_pool;
	channel_info_t channel_info[CHANNEL_INSTANCES_NUMBER];
} channels_info_t;

void free_channels_info(channels_info_t *channels_info);
channels_info_t *get_or_alloc_channels_info(event_pool_t *event_pool);
void channels_process_event(channels_info_t *channels_info);
void task_channels(void const *argument);
void task_channel_event(void const *argument);

#endif //_CHANNELS_H
