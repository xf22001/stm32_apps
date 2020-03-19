

/*================================================================
 *   
 *   
 *   文件名称：bms_handler.h
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 14时18分55秒
 *   修改日期：2019年10月31日 星期四 14时22分54秒
 *   描    述：
 *
 *================================================================*/
#ifndef _BMS_HANDLER_H
#define _BMS_HANDLER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "bms.h"

#ifdef __cplusplus
}
#endif
bms_state_handler_t *bms_get_state_handler(bms_state_t state);
#endif //_BMS_HANDLER_H
