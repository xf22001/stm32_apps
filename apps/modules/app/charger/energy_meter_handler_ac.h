

/*================================================================
 *   
 *   
 *   文件名称：energy_meter_handler_ac.h
 *   创 建 者：肖飞
 *   创建日期：2021年06月05日 星期六 12时57分38秒
 *   修改日期：2021年06月05日 星期六 13时06分10秒
 *   描    述：
 *
 *================================================================*/
#ifndef _ENERGY_METER_HANDLER_AC_H
#define _ENERGY_METER_HANDLER_AC_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "energy_meter.h"

#ifdef __cplusplus
}
#endif

int energy_meter_ac_init(energy_meter_info_t *energy_meter_info);

#endif //_ENERGY_METER_HANDLER_AC_H
