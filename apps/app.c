

/*================================================================
 *
 *
 *   文件名称：app.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月11日 星期五 16时54分03秒
 *   修改日期：2020年04月14日 星期二 10时24分29秒
 *   描    述：
 *
 *================================================================*/
#include "app.h"
#include "app_platform.h"
#include "cmsis_os.h"

extern IWDG_HandleTypeDef hiwdg;

void app(void const *argument)
{
	//osDelay(1000);

	while(1) {
		osDelay(100);
	}
}

void idle(void const *argument)
{
	while(1) {
		HAL_IWDG_Refresh(&hiwdg);
		osDelay(1000);
	}
}
