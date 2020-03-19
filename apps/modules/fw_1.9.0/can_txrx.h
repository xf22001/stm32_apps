

/*================================================================
 *
 *
 *   文件名称：can_txrx.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时29分22秒
 *   修改日期：2020年03月19日 星期四 13时49分21秒
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

#include "stm32f2xx_hal.h"
#include "stm32f2xx_hal_can.h"
#include "cmsis_os.h"

#include "list_utils.h"

typedef struct {
	uint32_t StdId;    /*!< Specifies the standard identifier.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x7FF */

	uint32_t ExtId;    /*!< Specifies the extended identifier.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x1FFFFFFF */

	uint32_t IDE;      /*!< Specifies the type of identifier for the message that will be transmitted.
                          This parameter can be a value of @ref CAN_Identifier_Type */

	uint32_t RTR;      /*!< Specifies the type of frame for the message that will be transmitted.
                          This parameter can be a value of @ref CAN_remote_transmission_request */

	uint32_t DLC;      /*!< Specifies the length of the frame that will be transmitted.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 8 */

	uint8_t Data[8];   /*!< Contains the data to be transmitted.
                          This parameter must be a number between Min_Data = 0 and Max_Data = 0xFF */
	uint32_t tx_mailbox;
} can_tx_msg_t;

typedef struct {
	uint32_t StdId;       /*!< Specifies the standard identifier.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 0x7FF */

	uint32_t ExtId;       /*!< Specifies the extended identifier.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 0x1FFFFFFF */

	uint32_t IDE;         /*!< Specifies the type of identifier for the message that will be received.
                             This parameter can be a value of @ref CAN_Identifier_Type */

	uint32_t RTR;         /*!< Specifies the type of frame for the received message.
                             This parameter can be a value of @ref CAN_remote_transmission_request */

	uint32_t DLC;         /*!< Specifies the length of the frame that will be received.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 8 */

	uint8_t Data[8];      /*!< Contains the data to be received.
                             This parameter must be a number between Min_Data = 0 and Max_Data = 0xFF */
} can_rx_msg_t;

typedef void (*can_hal_init_t)(void);
typedef void (*receive_init_t)(CAN_HandleTypeDef *hcan);

typedef struct {
	struct list_head list;

	CAN_HandleTypeDef *hcan;
	CAN_HandleTypeDef *config_can;
	can_rx_msg_t rx_msg;
	osMutexId hcan_mutex;
	osMessageQId tx_msg_q;
	osMessageQId rx_msg_q;

	uint32_t filter_number;
	uint32_t filter_fifo;
	uint32_t receive_fifo;

	can_hal_init_t can_hal_init;
	receive_init_t receive_init;
} can_info_t;

can_info_t *get_can_info(CAN_HandleTypeDef *hcan);
void free_can_info(can_info_t *can_info);
can_info_t *alloc_can_info(CAN_HandleTypeDef *hcan);
void set_can_info_hal_init(can_info_t *can_info, can_hal_init_t can_hal_init);
int can_tx_data(can_info_t *can_info, can_tx_msg_t *msg, uint32_t timeout);
int can_rx_data(can_info_t *can_info, uint32_t timeout);
can_rx_msg_t *can_get_msg(can_info_t *can_info);
#endif //_CAN_TXRX_H
