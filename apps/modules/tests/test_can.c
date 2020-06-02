

/*================================================================
 *
 *
 *   文件名称：test_can.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 16时45分27秒
 *   修改日期：2020年06月02日 星期二 17时02分18秒
 *   描    述：
 *
 *================================================================*/
#include "test_can.h"

#include "os_utils.h"

#include "cmsis_os.h"
#include "app_platform.h"

#include "can_txrx.h"
#include "can.h"

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

static void task_can_send(void const *argument)
{
	can_info_t *can_info = (can_info_t *)argument;

	if(can_info == NULL) {
		app_panic();
	}

	for(;;) {
		int ret = can_transmit_test(can_info);

		if(ret == 0) {
		}

		osDelay(1000);
	}
}

static void task_can_receive(void const *argument)
{
	can_info_t *can_info = (can_info_t *)argument;

	if(can_info == NULL) {
		app_panic();
	}

	for(;;) {
		int ret = can_rx_data(can_info, 10);

		if(ret == 0) {
			can_transmit_test(can_info);
		}
	}
}


void test_can(void)
{

	{
		can_info_t *can_info = get_or_alloc_can_info(&hcan1);
		osThreadDef(test_can, task_can_send, osPriorityNormal, 0, 128 * 4);

		if(can_info == NULL) {
			app_panic();
		}

		osThreadCreate(osThread(test_can), can_info);
	}

	{
		can_info_t *can_info = get_or_alloc_can_info(&hcan2);
		osThreadDef(test_can, task_can_receive, osPriorityNormal, 0, 128 * 4);

		if(can_info == NULL) {
			app_panic();
		}

		osThreadCreate(osThread(test_can), can_info);
	}
}
