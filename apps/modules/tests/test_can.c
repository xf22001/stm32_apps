

/*================================================================
 *
 *
 *   文件名称：test_can.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 16时45分27秒
 *   修改日期：2021年01月20日 星期三 15时58分39秒
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

static int can_transmit_test(can_info_t *info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;


	tx_msg.ExtId = 0x1826f456;   //扩展ID 初始化
	tx_msg.IDE = CAN_ID_EXT;           //ID类型 为扩展ID
	tx_msg.RTR = CAN_RTR_DATA;         //
	tx_msg.DLC = 3;                    //数据长度为3个字节

	tx_msg.Data[0] = 1;   //通讯协议版本号
	tx_msg.Data[1] = 1;
	tx_msg.Data[2] = 0;

	ret = can_tx_data(info, &tx_msg, 10);

	return ret;
}

typedef struct {
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

	if(ticks - ctx->recv_stamp >= 1000) {
		ctx->recv_stamp = ticks;
		debug("%s rx error!\n", ctx->des);
	}

	if(ticks - ctx->stamp < 500) {
		return;
	}

	ctx->stamp = ticks;

	ret = can_transmit_test(can_data_task_info->can_info);

	if(ret == 0) {
		//debug("%s tx done!\n", ctx->des);
	} else {
		debug("%s tx error!\n", ctx->des);
	}
}

static void can_data_response(void *fn_ctx, void *chain_ctx)
{
	can_test_ctx_t *ctx = (can_test_ctx_t *)fn_ctx;
	//can_data_task_info_t *can_data_task_info = (can_data_task_info_t *)chain_ctx;
	uint32_t ticks = osKernelSysTick();

	ctx->recv_stamp = ticks;
	//debug("%s rx done!\n", ctx->des);
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
	can1_data_request_cb.fn = can_data_request;
	can1_data_request_cb.fn_ctx = &ctx1;
	add_can_data_task_info_request_cb(can_data_task_info, &can1_data_request_cb);

	can1_data_response_cb.fn = can_data_response;
	can1_data_response_cb.fn_ctx = &ctx1;
	add_can_data_task_info_response_cb(can_data_task_info, &can1_data_response_cb);



	can_data_task_info = get_or_alloc_can_data_task_info(&hcan2);

	ctx2.des = "can2";
	can2_data_request_cb.fn = can_data_request;
	can2_data_request_cb.fn_ctx = &ctx2;
	add_can_data_task_info_request_cb(can_data_task_info, &can2_data_request_cb);

	can2_data_response_cb.fn = can_data_response;
	can2_data_response_cb.fn_ctx = &ctx2;
	add_can_data_task_info_response_cb(can_data_task_info, &can2_data_response_cb);
}
