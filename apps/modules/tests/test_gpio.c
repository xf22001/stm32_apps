

/*================================================================
 *   
 *   
 *   文件名称：test_gpio.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 11时19分47秒
 *   修改日期：2019年11月13日 星期三 15时02分18秒
 *   描    述：
 *
 *================================================================*/
#include "test_gpio.h"
#include "main.h"
#include "cmsis_os.h"

#define test_relay(relay) do { \
	HAL_GPIO_WritePin(relay##_GPIO_Port, relay##_Pin, GPIO_PIN_SET); \
	osDelay(1000); \
	HAL_GPIO_WritePin(relay##_GPIO_Port, relay##_Pin, GPIO_PIN_RESET); \
	osDelay(1000); \
} while(0)

#define test_in(in) do { \
	GPIO_PinState state = HAL_GPIO_ReadPin(in##_GPIO_Port, in##_Pin); \
	if(state == GPIO_PIN_RESET) { \
	} else { \
	} \
	osDelay(1000); \
} while(0)

#define test_out(out) do { \
	HAL_GPIO_WritePin(out##_GPIO_Port, out##_Pin, GPIO_PIN_SET); \
	osDelay(1000); \
	HAL_GPIO_WritePin(out##_GPIO_Port, out##_Pin, GPIO_PIN_RESET); \
	osDelay(1000); \
} while(0)

#define test_led(led) do { \
	HAL_GPIO_WritePin(led##_GPIO_Port, led##_Pin, GPIO_PIN_SET); \
	osDelay(1000); \
	HAL_GPIO_WritePin(led##_GPIO_Port, led##_Pin, GPIO_PIN_RESET); \
	osDelay(1000); \
} while(0)


void task_test_gpio(void const *argument)
{
	while(1) {
		//test_relay(relay_2);
		//test_relay(relay_3);
		//test_relay(relay_4);
		//test_relay(relay_5);
		//test_relay(relay_6);
		//test_relay(relay_7);
		//test_relay(relay_8);
		test_in(in_1);
		test_in(in_2);
		test_in(in_3);
		test_in(in_4);
		test_in(in_5);
		test_in(in_6);
		test_in(in_7);
		test_in(in_8);
		test_out(out_1);
		test_out(out_2);
		test_out(out_3);
		test_out(out_4);
		test_out(out_5);
		test_out(out_6);
		test_out(out_7);
		test_out(out_8);
		test_led(led_485_1);
		test_led(led_232_2);
		test_led(led_485_3);
		test_led(led_232_6);
		test_led(led_bms);
		test_led(led_ccs);
		test_led(led_lan);
	}
}
