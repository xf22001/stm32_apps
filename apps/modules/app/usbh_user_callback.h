

/*================================================================
 *   
 *   
 *   文件名称：usbh_user_callback.h
 *   创 建 者：肖飞
 *   创建日期：2021年03月25日 星期四 14时19分58秒
 *   修改日期：2021年03月25日 星期四 14时24分56秒
 *   描    述：
 *
 *================================================================*/
#ifndef _USBH_USER_CALLBACK_H
#define _USBH_USER_CALLBACK_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "usbh_def.h"

#ifdef __cplusplus
}
#endif

void usbh_user_callback(USBH_HandleTypeDef *phost, uint8_t id);
#endif //_USBH_USER_CALLBACK_H
