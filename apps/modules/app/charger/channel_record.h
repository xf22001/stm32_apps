

/*================================================================
 *   
 *   
 *   文件名称：channel_record.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月23日 星期日 13时40分28秒
 *   修改日期：2021年05月23日 星期日 14时59分23秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_RECORD_H
#define _CHANNEL_RECORD_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "callback_chain.h"

#ifdef __cplusplus
}
#endif

#define CHANNEL_RECORD_NUMBER 500

#pragma pack(push, 1)

typedef struct {
	uint16_t total;
	uint16_t start;
	uint16_t end;
} channel_record_info_t;

typedef struct {
	uint16_t index;
	uint8_t channel_id;
	uint8_t upload;
} channel_record_item_t;

#pragma pack(pop)

typedef struct {
	uint32_t id;
	channel_record_info_t channel_record_info;
	callback_chain_t *channel_record_sync_chain;
	callback_chain_t *channel_record_update_chain;
	os_signal_t sync_signal;
	os_mutex_t mutex;
} channel_record_task_info_t; 
#endif //_CHANNEL_RECORD_H
