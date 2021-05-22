

/*================================================================
 *   
 *   
 *   文件名称：charger_handler.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 14时18分46秒
 *   修改日期：2019年10月31日 星期四 14时23分32秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHARGER_HANDLER_H
#define _CHARGER_HANDLER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "charger.h"

#ifdef __cplusplus
}
#endif
charger_state_handler_t *charger_get_state_handler(charger_state_t state);
#endif //_CHARGER_HANDLER_H
