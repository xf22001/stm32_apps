

/*================================================================
 *
 *
 *   文件名称：can_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 14时07分55秒
 *   修改日期：2021年06月16日 星期三 09时39分59秒
 *   描    述：
 *
 *================================================================*/
#include "can_txrx.h"

#include "os_utils.h"
#include "object_class.h"

extern can_ops_t can_ops_hal;
//extern can_ops_t can_ops_spi_can;

static object_class_t *can_class = NULL;
static uint8_t can_id = 0;

static can_ops_t *can_ops_sz[] = {
	&can_ops_hal,
	//&can_ops_spi_can,
};

static can_ops_t *get_can_ops(can_type_t type)
{
	uint8_t i;
	can_ops_t *can_ops = NULL;

	for(i = 0; i < ARRAY_SIZE(can_ops_sz); i++) {
		can_ops_t *can_ops_item = can_ops_sz[i];

		if(type == can_ops_item->type) {
			can_ops = can_ops_item;
			break;
		}
	}

	return can_ops;
}

static void free_can_info(can_info_t *can_info)
{
	app_panic();
}

static can_info_t *alloc_can_info(void *hcan)
{
	can_info_t *can_info = NULL;
	can_config_t *can_config = NULL;
	can_ops_t *can_ops = NULL;

	OS_ASSERT(hcan != NULL);

	can_config = get_can_config(hcan);
	OS_ASSERT(can_config != NULL);

	can_ops = get_can_ops(can_config->type);
	OS_ASSERT(can_ops != NULL);

	can_info = (can_info_t *)os_calloc(1, sizeof(can_info_t));
	OS_ASSERT(can_info != NULL);

	can_info->can_id = can_id++;
	can_info->hcan = hcan;
	can_info->can_config = can_config;
	can_info->can_ops = can_ops;

	can_info->hcan_mutex = mutex_create();
	can_info->tx_msg_q = signal_create(1);
	can_info->rx_msg_q = signal_create(1);

	can_info->rx_msg_r = 0;
	can_info->rx_msg_w = 0;
	can_info->rx_msg_pos = 0;
	can_info->tx_error = 0;

	OS_ASSERT(can_info->can_ops->can_init != NULL);

	can_info->can_ops->can_init(can_info);

	return can_info;
}

static int object_filter(void *o, void *hcan)
{
	int ret = -1;
	can_info_t *can_info = (can_info_t *)o;

	if(can_info->hcan == hcan) {
		ret = 0;
	}

	return ret;
}

can_info_t *get_or_alloc_can_info(void *hcan)
{
	can_info_t *can_info = NULL;

	os_enter_critical();

	if(can_class == NULL) {
		can_class = object_class_alloc();
	}

	os_leave_critical();

	can_info = (can_info_t *)object_class_get_or_alloc_object(can_class, object_filter, hcan, (object_alloc_t)alloc_can_info, (object_free_t)free_can_info);

	return can_info;
}

static int object_filter_by_id(void *o, void *id)
{
	int ret = -1;
	can_info_t *can_info = (can_info_t *)o;
	uint8_t *can_id = (uint8_t *)id;

	if(can_info->can_id == *can_id) {
		ret = 0;
	}

	return ret;
}

can_info_t *get_can_info_by_id(uint8_t id)
{
	return (can_info_t *)object_class_get_object(can_class, object_filter_by_id, (void *)&id);
}

void can_init(can_info_t *can_info)
{
	OS_ASSERT(can_info != NULL);
	OS_ASSERT(can_info->can_ops != NULL);
	OS_ASSERT(can_info->can_ops->can_init != NULL);
	can_info->can_ops->can_init(can_info);
}

int can_tx_data(can_info_t *can_info, can_tx_msg_t *msg, uint32_t timeout)
{
	int ret;

	OS_ASSERT(can_info != NULL);
	OS_ASSERT(can_info->can_ops != NULL);
	OS_ASSERT(can_info->can_ops->can_tx_data != NULL);

	ret = can_info->can_ops->can_tx_data(can_info, msg, timeout);

	return ret;
}

int can_rx_data(can_info_t *can_info, uint32_t timeout)
{
	int ret;

	OS_ASSERT(can_info != NULL);
	OS_ASSERT(can_info->can_ops != NULL);
	OS_ASSERT(can_info->can_ops->can_rx_data != NULL);

	ret = can_info->can_ops->can_rx_data(can_info, timeout);

	return ret;
}

can_rx_msg_t *can_get_msg(can_info_t *can_info)
{
	can_rx_msg_t *rx_msg = &can_info->rx_msg[can_info->rx_msg_pos];

	return rx_msg;
}
