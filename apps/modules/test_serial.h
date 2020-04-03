

/*================================================================
 *   
 *   
 *   文件名称：test_serial.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 10时54分05秒
 *   修改日期：2020年04月03日 星期五 12时40分39秒
 *   描    述：
 *
 *================================================================*/
#ifndef _TEST_SERIAL_H
#define _TEST_SERIAL_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"

#ifdef __cplusplus
}
#endif
void serial_self_test(UART_HandleTypeDef *huart);
void task_test_serial(void const *argument);
#endif //_TEST_SERIAL_H
