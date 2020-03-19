

/*================================================================
 *   
 *   
 *   文件名称：flash.h
 *   创 建 者：肖飞
 *   创建日期：2019年12月03日 星期二 10时09分34秒
 *   修改日期：2019年12月03日 星期二 13时32分03秒
 *   描    述：
 *
 *================================================================*/
#ifndef _FLASH_H
#define _FLASH_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif
int8_t flash_erase_sector(uint32_t start, uint32_t sectors);
int8_t flash_write(uint32_t start_addr, uint8_t *data, uint32_t size);
#endif //_FLASH_H
