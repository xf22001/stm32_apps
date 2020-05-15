

/*================================================================
 *   
 *   
 *   文件名称：power_modules_handlers.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 16时38分57秒
 *   修改日期：2020年05月15日 星期五 16时46分04秒
 *   描    述：
 *
 *================================================================*/
#ifndef _POWER_MODULES_HANDLERS_H
#define _POWER_MODULES_HANDLERS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "power_modules.h"

#ifdef __cplusplus
}
#endif

power_modules_handler_t *get_power_modules_handler(power_module_type_t power_module_type);

#endif //_POWER_MODULES_HANDLERS_H
