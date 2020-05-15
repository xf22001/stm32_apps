

/*================================================================
 *
 *
 *   文件名称：power_modules_handlers.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 16时20分29秒
 *   修改日期：2020年05月15日 星期五 17时24分47秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handlers.h"
#include "power_modules_handler_huawei.h"
#include "power_modules_handler_increase.h"

static power_modules_handler_t *power_modules_handler_sz[] = {
	&power_modules_handler_huawei,
	&power_modules_handler_increase,
};


power_modules_handler_t *get_power_modules_handler(power_module_type_t power_module_type)
{
	int i;
	power_modules_handler_t *power_modules_handler = NULL;
	power_modules_handler_t *power_modules_handler_item = NULL;

	for(i = 0; i < sizeof(power_modules_handler_sz) / sizeof(power_modules_handler_t *); i++) {
		power_modules_handler_item = power_modules_handler_sz[i];

		if(power_modules_handler_item->power_module_type == power_module_type) {
			power_modules_handler = power_modules_handler_item;
			break;
		}
	}

	return power_modules_handler;
}
