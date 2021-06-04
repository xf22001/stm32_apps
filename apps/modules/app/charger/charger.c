

/*================================================================
 *
 *
 *   文件名称：charger.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月19日 星期二 12时32分21秒
 *   修改日期：2021年06月04日 星期五 17时39分14秒
 *   描    述：
 *
 *================================================================*/
#include "charger.h"

#include <string.h>

#include "charger_bms.h"
#include "channels.h"
#include "os_utils.h"
#include "can_data_task.h"

#include "log.h"

charger_info_t *alloc_charger_info(channel_info_t *channel_info)
{
	charger_info_t *charger_info = NULL;

	charger_info = (charger_info_t *)os_calloc(1, sizeof(charger_info_t));

	OS_ASSERT(charger_info != NULL);

	charger_info->channel_info = channel_info;

	charger_info->charger_bms_status_changed = alloc_callback_chain();

	OS_ASSERT(charger_info->charger_bms_status_changed != NULL);

	OS_ASSERT(charger_bms_init(charger_info) == 0);

	return charger_info;
}

