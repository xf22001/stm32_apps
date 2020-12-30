

/*================================================================
 *
 *
 *   文件名称：can_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时29分22秒
 *   修改日期：2020年12月30日 星期三 15时02分05秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CAN_TXRX_H
#define _CAN_TXRX_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "list_utils.h"
#include "can_config.h"

typedef CanTxMsgTypeDef can_tx_msg_t;
typedef CanRxMsgTypeDef can_rx_msg_t;

typedef void (*can_hal_init_t)(void);
typedef void (*receive_init_t)(CAN_HandleTypeDef *hcan);

typedef struct {
	CAN_HandleTypeDef *hcan;
	CAN_HandleTypeDef *config_can;
	can_rx_msg_t rx_msg;
	can_rx_msg_t rx_msg1;
	osMutexId hcan_mutex;
	osMessageQId tx_msg_q;
	osMessageQId rx_msg_q;

	uint32_t receive_fifo;
	can_config_t *can_config;

	can_hal_init_t can_hal_init;
	receive_init_t receive_init;
} can_info_t;

void free_can_info(can_info_t *can_info);
can_info_t *get_or_alloc_can_info(CAN_HandleTypeDef *hcan);
void set_can_info_hal_init(can_info_t *can_info, can_hal_init_t can_hal_init);
int can_tx_data(can_info_t *can_info, can_tx_msg_t *msg, uint32_t timeout);
int can_rx_data(can_info_t *can_info, uint32_t timeout);
can_rx_msg_t *can_get_msg(can_info_t *can_info);
#endif //_CAN_TXRX_H
