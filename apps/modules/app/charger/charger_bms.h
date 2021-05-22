

/*================================================================
 *   
 *   
 *   文件名称：charger_bms.h
 *   创 建 者：肖飞
 *   创建日期：2021年04月11日 星期日 08时59分55秒
 *   修改日期：2021年04月11日 星期日 09时02分20秒
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

charger_bms_handler_t *get_charger_bms_handler(channel_charger_bms_type_t charger_bms_type);
#endif //_CHARGER_BMS_H
