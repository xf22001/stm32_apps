

/*================================================================
 *   
 *   
 *   文件名称：usb_upgrade.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月08日 星期六 20时15分44秒
 *   修改日期：2021年05月08日 星期六 21时03分47秒
 *   描    述：
 *
 *================================================================*/
#ifndef _USB_UPGRADE_H
#define _USB_UPGRADE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif
void start_usb_upgrade(void);
void handle_usb_upgrade(void);
#endif //_USB_UPGRADE_H
