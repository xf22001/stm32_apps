

/*================================================================
 *   
 *   
 *   文件名称：iap.h
 *   创 建 者：肖飞
 *   创建日期：2019年12月04日 星期三 10时07分05秒
 *   修改日期：2020年01月15日 星期三 10时37分23秒
 *   描    述：
 *
 *================================================================*/
#ifndef _IAP_H
#define _IAP_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif
#define USER_FLASH_FIRST_PAGE_ADDRESS 0x08040000
#define APP_CONFIG_ADDRESS 0x0807FC00
#define APP_SIZE_ADDRESS (APP_CONFIG_ADDRESS + 1)

void jump_to_app(void);
#endif //_IAP_H
