

/*================================================================
 *   
 *   
 *   文件名称：test_serial.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月28日 星期一 10时54分05秒
 *   修改日期：2021年02月07日 星期日 13时31分38秒
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
void test_serial(UART_HandleTypeDef *huart);
#endif //_TEST_SERIAL_H
