

/*================================================================
 *   
 *   
 *   文件名称：can_data_task.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月19日 星期二 16时16分38秒
 *   修改日期：2021年01月19日 星期二 17时31分16秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CAN_DATA_TASK_H
#define _CAN_DATA_TASK_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "can_txrx.h"
#include "callback_chain.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	can_info_t *can_info;
	callback_chain_t *can_data_request_chain;
	callback_chain_t *can_data_response_chain;
} can_data_task_info_t;

int add_can_data_task_info_request_cb(can_data_task_info_t *can_data_task_info, callback_item_t *callback_item);
int remove_can_data_task_info_request_cb(can_data_task_info_t *can_data_task_info, callback_item_t *callback_item);
int add_can_data_task_info_response_cb(can_data_task_info_t *can_data_task_info, callback_item_t *callback_item);
int remove_can_data_task_info_response_cb(can_data_task_info_t *can_data_task_info, callback_item_t *callback_item);
can_data_task_info_t *get_or_alloc_can_data_task_info(CAN_HandleTypeDef *hcan);
#endif //_CAN_DATA_TASK_H
