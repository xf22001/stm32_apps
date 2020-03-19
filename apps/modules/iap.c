

/*================================================================
 *   
 *   
 *   文件名称：iap.c
 *   创 建 者：肖飞
 *   创建日期：2019年12月04日 星期三 10时07分01秒
 *   修改日期：2019年12月04日 星期三 10时10分30秒
 *   描    述：
 *
 *================================================================*/
#include "iap.h"

#include "stm32f2xx_hal.h"

typedef  void (*app_entry_t)(void);
void jump_to_app(void)
{
	uint8_t *iap_en = (uint8_t *)APP_CONFIG_ADDRESS;
	app_entry_t entry;

	if(*iap_en == 1) {
		if (((*(__IO uint32_t *)USER_FLASH_FIRST_PAGE_ADDRESS) & 0x2FFE0000 ) == 0x20020000) {
			/* Jump to user application */
			entry = (app_entry_t)(*(__IO uint32_t *) (USER_FLASH_FIRST_PAGE_ADDRESS + 4));

			/* Initialize user application's Stack Pointer */
			__set_MSP(*(__IO uint32_t *) USER_FLASH_FIRST_PAGE_ADDRESS);
			entry();
		}

	}
}
