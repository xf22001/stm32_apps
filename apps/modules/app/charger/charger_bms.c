

/*================================================================
 *   
 *   
 *   文件名称：charger_bms.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月11日 星期日 08时59分50秒
 *   修改日期：2021年04月11日 星期日 09时01分22秒
 *   描    述：
 *
 *================================================================*/
#include "charger_bms.h"

#include "charger_bms_gb.h"

static charger_bms_handler_t *charger_bms_handler_sz[] = {
	&charger_bms_handler_gb,
};

charger_bms_handler_t *get_charger_bms_handler(channel_charger_bms_type_t charger_bms_type)
{
	int i;
	charger_bms_handler_t *charger_bms_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(charger_bms_handler_sz); i++) {
		charger_bms_handler_t *charger_bms_handler_item = charger_bms_handler_sz[i];

		if(charger_bms_handler_item->charger_bms_type == charger_bms_type) {
			charger_bms_handler = charger_bms_handler_item;
		}
	}

	return charger_bms_handler;
}

