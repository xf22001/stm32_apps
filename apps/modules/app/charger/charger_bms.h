

/*================================================================
 *   
 *   
 *   文件名称：charger_bms.h
 *   创 建 者：肖飞
 *   创建日期：2021年06月04日 星期五 17时10分17秒
 *   修改日期：2021年06月04日 星期五 23时46分09秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHARGER_BMS_H
#define _CHARGER_BMS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "charger.h"

#ifdef __cplusplus
}
#endif

void set_charger_bms_request_action(charger_info_t *charger_info, charger_bms_request_action_t charger_bms_request_action);
void set_charger_bms_request_state(charger_info_t *charger_info, uint8_t request_state);
int charger_bms_init(void *_charger_info);

#endif //_CHARGER_BMS_H
