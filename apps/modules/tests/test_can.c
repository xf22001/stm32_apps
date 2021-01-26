

/*================================================================
 *
 *
 *   文件名称：test_can.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 16时45分27秒
 *   修改日期：2021年01月26日 星期二 11时51分55秒
 *   描    述：
 *
 *================================================================*/
#include "test_can.h"

#include "os_utils.h"

#include "cmsis_os.h"
#include "app_platform.h"

#include "can_data_task.h"
#include "can.h"
#include "log.h"

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

typedef struct {
	uint32_t d0;
	uint32_t d1;
} can_data_t;

static int can_send_data(can_info_t *info, can_data_t *data)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_data_t *can_data = (can_data_t *)tx_msg.Data;

	tx_msg.ExtId = 0x12345678;//扩展ID 初始化
	tx_msg.IDE = CAN_ID_EXT;//ID类型 为扩展ID
	tx_msg.RTR = CAN_RTR_DATA;//
	tx_msg.DLC = sizeof(tx_msg.Data);//数据长度为3个字节

	*can_data = *data;

	ret = can_tx_data(info, &tx_msg, 10);

	return ret;
}

typedef struct {
	can_data_t send;
	can_data_t recv;
	uint32_t stamp;
	uint32_t recv_stamp;
	char *des;
} can_test_ctx_t;

static void can_data_request(void *fn_ctx, void *chain_ctx)
{
	can_test_ctx_t *ctx = (can_test_ctx_t *)fn_ctx;
	can_data_task_info_t *can_data_task_info = (can_data_task_info_t *)chain_ctx;
	uint32_t ticks = osKernelSysTick();
	int ret;

	if(ticks - ctx->recv_stamp >= 100) {
		ctx->recv_stamp = ticks;
		debug("%s rx timeout!\n", ctx->des);
	}

	if(ticks - ctx->stamp < 50) {
		return;
	}

	ctx->stamp = ticks;

	ret = can_send_data(can_data_task_info->can_info, &ctx->send);

	if(ret == 0) {
		ctx->send.d0++;

		if(ctx->send.d0 == 0) {
			ctx->send.d1++;
		}

		//debug("%s tx done!\n", ctx->des);
	} else {
		debug("%s tx error!\n", ctx->des);
	}
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	can_test_ctx_t *ctx = (can_test_ctx_t *)fn_ctx;
	can_data_task_info_t *can_data_task_info = (can_data_task_info_t *)chain_ctx;
	can_rx_msg_t *can_rx_msg = can_get_msg(can_data_task_info->can_info);
	can_data_t *can_data = (can_data_t *)can_rx_msg->Data;
	uint32_t ticks = osKernelSysTick();

	if(can_rx_msg->ExtId == 0x12345678) {
		if(ctx->recv.d0 + 1 == can_data->d0) {
			if(can_data->d0 == 0) {
				if(ctx->recv.d1 + 1 == can_data->d1) {
					//debug("%s rx done!\n", ctx->des);
				} else {
					debug("%s ctx->recv.d1:%d, can_data->d1:%d!\n", ctx->des, ctx->recv.d1, can_data->d1);
					debug("%s rx error!\n", ctx->des);
				}
			} else {
				if(ctx->recv.d1 == can_data->d1) {
					//debug("%s rx done!\n", ctx->des);
				} else {
					debug("%s ctx->recv.d1:%d, can_data->d1:%d!\n", ctx->des, ctx->recv.d1, can_data->d1);
					debug("%s rx error!\n", ctx->des);
				}
			}
		} else {
			debug("%s ctx->recv.d0:%d, can_data->d0:%d!\n", ctx->des, ctx->recv.d0, can_data->d0);
			debug("%s rx error!\n", ctx->des);
		}
	} else {
		debug("%s can_rx_msg->ExtId:%08x!\n", ctx->des, can_rx_msg->ExtId);
		debug("%s rx error!\n", ctx->des);
	}

	ctx->recv = *can_data;

	ctx->recv_stamp = ticks;
}

static can_test_ctx_t ctx1 = {0};
static callback_item_t can1_data_request_cb;
static callback_item_t can1_data_response_cb;

static can_test_ctx_t ctx2 = {0};
static callback_item_t can2_data_request_cb;
static callback_item_t can2_data_response_cb;

void test_can(void)
{

	can_data_task_info_t *can_data_task_info;

	can_data_task_info = get_or_alloc_can_data_task_info(&hcan1);

	ctx1.des = "can1";
	ctx1.send.d0 = 1;
	ctx1.send.d1 = 0;
	can1_data_request_cb.fn = can_data_request;
	can1_data_request_cb.fn_ctx = &ctx1;
	add_can_data_task_info_request_cb(can_data_task_info, &can1_data_request_cb);

	can1_data_response_cb.fn = can_data_response;
	can1_data_response_cb.fn_ctx = &ctx1;
	add_can_data_task_info_response_cb(can_data_task_info, &can1_data_response_cb);



	can_data_task_info = get_or_alloc_can_data_task_info(&hcan2);

	ctx2.des = "can2";
	ctx2.send.d0 = 1;
	ctx2.send.d1 = 0;
	can2_data_request_cb.fn = can_data_request;
	can2_data_request_cb.fn_ctx = &ctx2;
	add_can_data_task_info_request_cb(can_data_task_info, &can2_data_request_cb);

	can2_data_response_cb.fn = can_data_response;
	can2_data_response_cb.fn_ctx = &ctx2;
	add_can_data_task_info_response_cb(can_data_task_info, &can2_data_response_cb);
}
