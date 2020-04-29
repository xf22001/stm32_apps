

/*================================================================
 *
 *
 *   文件名称：can_config.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月17日 星期五 09时16分53秒
 *   修改日期：2020年04月29日 星期三 11时02分29秒
 *   描    述：
 *
 *================================================================*/
#include "can_config.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

can_config_t can_config_can1 = {
	.hcan = &hcan1,
	.filter_number = 0,
	.filter_fifo = CAN_FILTER_FIFO0,
	.config_can = &hcan1,
	.filter_id = 0,
	.filter_mask_id = 0,
	.filter_rtr = 0,
	.filter_mask_rtr = 0,
	.filter_ext = 0,
	.filter_mask_ext = 0,
};

can_config_t can_config_can2 = {
	.hcan = &hcan2,
	.filter_number = 14,
	.filter_fifo = CAN_FILTER_FIFO1,
	.config_can = &hcan1,
	.filter_id = 0,
	.filter_mask_id = 0,
	.filter_rtr = 0,
	.filter_mask_rtr = 0,
	.filter_ext = 0,
	.filter_mask_ext = 0,
};
