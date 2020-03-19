

/*================================================================
 *
 *
 *   文件名称：app.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月11日 星期五 16时54分03秒
 *   修改日期：2020年03月19日 星期四 14时02分25秒
 *   描    述：
 *
 *================================================================*/
#include "app.h"
#include "stm32f2xx_hal.h"
#include "cmsis_os.h"

#include "stm32f2xx_hal_tim.h"

#include "modules/os_utils.h"
#include "modules/usart_txrx.h"
#include "modules/can_txrx.h"
#include "modules/spi_txrx.h"
#include "modules/modbus_txrx.h"
#include "modules/test_serial.h"
#include "modules/test_can.h"
#include "modules/test_gpio.h"
#include "modules/test_charger_bms.h"
#include "modules/bms.h"
#include "modules/charger.h"
#include "modules/eeprom.h"
#include "modules/task_probe_tool.h"
#include "modules/net_client.h"
#include "modules/channels.h"

extern IWDG_HandleTypeDef hiwdg;

extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart3;

void app(void const *argument)
{
	//osDelay(1000);

	osThreadDef(probe_tool, task_probe_tool, osPriorityNormal, 0, 128 * 2 * 2);
	osThreadCreate(osThread(probe_tool), NULL);

	//while(is_log_client_address_valid() == 0) {
	//	osDelay(1);
	//}

	//{
	//	uart_info_t *uart_info = alloc_uart_info(&huart3);

	//	if(uart_info == NULL) {
	//		app_panic();
	//	}

	//	osThreadDef(test_serial_rx, task_test_serial, osPriorityNormal, 0, 128);
	//	osThreadCreate(osThread(test_serial_rx), uart_info);
	//}

	//{
	//	osThreadDef(test_gpio, task_test_gpio, osPriorityNormal, 0, 128);
	//	osThreadCreate(osThread(test_gpio), NULL);
	//}

	osThreadDef(net_client, task_net_client, osPriorityHigh, 0, 128 * 2 * 16);
	osThreadCreate(osThread(net_client), NULL);
	//test_can();
	test_charger_bms();

	//{
	//	event_pool_t *event_pool = alloc_event_pool();
	//	osThreadDef(test_channels, task_channels, osPriorityNormal, 0, 128 * 2);
	//	osThreadDef(test_channel_event, task_channel_event, osPriorityNormal, 0, 128 * 2);

	//	if(event_pool == NULL) {
	//		app_panic();
	//	}

	//	osThreadCreate(osThread(test_channels), event_pool);
	//	osThreadCreate(osThread(test_channel_event), event_pool);
	//}

	while(1) {
		osDelay(100);
	}
}

typedef enum {
	PWM_COMPARE_COUNT_UP = 0,
	PWM_COMPARE_COUNT_DOWN,
	PWM_COMPARE_COUNT_KEEP,
} compare_count_type_t;

static void update_work_led(void)
{
	static compare_count_type_t type = PWM_COMPARE_COUNT_UP;
	static uint16_t duty_cycle = 0;
	static uint16_t keep_count = 0;
	//计数值小于duty_cycle,输出1;大于duty_cycle输出0

	switch(type) {
		case PWM_COMPARE_COUNT_UP: {

			if(duty_cycle < 1000) {
				duty_cycle += 5;
			} else {
				type = PWM_COMPARE_COUNT_KEEP;
			}
		}
		break;

		case PWM_COMPARE_COUNT_DOWN: {
			if(duty_cycle > 0) {
				duty_cycle -= 5;
			} else {
				type = PWM_COMPARE_COUNT_UP;
			}

		}
		break;

		case PWM_COMPARE_COUNT_KEEP: {
			if(keep_count < duty_cycle) {
				keep_count += 10;
			} else {
				keep_count = 0;
				type = PWM_COMPARE_COUNT_DOWN;
			}

		}
		break;

		default:
			break;
	}

	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, duty_cycle);
}

void idle(void const *argument)
{
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);

	while(1) {
		HAL_IWDG_Refresh(&hiwdg);
		update_work_led();
		osDelay(1);
	}
}
