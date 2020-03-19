

/*================================================================
 *
 *
 *   文件名称：charger.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 12时57分41秒
 *   修改日期：2020年01月19日 星期日 16时35分45秒
 *   描    述：
 *
 *================================================================*/
#include "charger.h"
#include "charger_handler.h"

#include "os_utils.h"
#include <string.h>

static LIST_HEAD(charger_info_list);

static void bms_data_settings_init(bms_data_settings_t *settings)
{
	if(settings == NULL) {
		return;
	}

	memset(settings, 0, sizeof(bms_data_settings_t));
}

static bms_data_settings_t *bms_data_alloc_settings(void)
{
	bms_data_settings_t *settings = (bms_data_settings_t *)os_alloc(sizeof(bms_data_settings_t));

	if(settings == NULL) {
		return settings;
	}

	bms_data_settings_init(settings);

	return settings;
}

charger_info_t *get_charger_info(can_info_t *can_info)
{
	charger_info_t *charger_info = NULL;
	charger_info_t *charger_info_item = NULL;

	list_for_each_entry(charger_info_item, &charger_info_list, charger_info_t, list) {
		if(charger_info_item->can_info == can_info) {
			charger_info = charger_info_item;
			break;
		}
	}

	return charger_info;
}

void free_charger_info(charger_info_t *charger_info)
{
	osStatus os_status;

	if(charger_info == NULL) {
		return;
	}

	if(charger_info->settings) {
		os_free(charger_info->settings);
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexDelete(charger_info->handle_mutex);

		if(osOK != os_status) {
		}
	}

	list_del(&charger_info->list);

	os_free(charger_info);
}

charger_info_t *alloc_charger_info(can_info_t *can_info)
{
	charger_info_t *charger_info = NULL;
	osMutexDef(handle_mutex);

	charger_info = get_charger_info(can_info);
	if(charger_info != NULL) {
		return charger_info;
	}

	charger_info = (charger_info_t *)os_alloc(sizeof(charger_info_t));
	if(charger_info == NULL) {
		return charger_info;
	}

	charger_info->can_info = can_info;
	charger_info->state = CHARGER_STATE_IDLE;
	charger_info->handle_mutex = osMutexCreate(osMutex(handle_mutex));
	charger_info->settings = bms_data_alloc_settings();

	list_add_tail(&charger_info->list, &charger_info_list);

	return charger_info;
}

charger_state_t get_charger_state(charger_info_t *charger_info)
{
	return charger_info->state;
}

void set_charger_state(charger_info_t *charger_info, charger_state_t state)
{
	charger_state_handler_t *handler = charger_get_state_handler(state);

	if((handler != NULL) && (handler->prepare != NULL)) {
		handler->prepare(charger_info);
	}

	charger_info->state = state;
}

void set_charger_state_locked(charger_info_t *charger_info, charger_state_t state)
{
	osStatus os_status;

	if(charger_info->handle_mutex) {
		os_status = osMutexWait(charger_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	set_charger_state(charger_info, state);

	if(charger_info->handle_mutex) {
		os_status = osMutexRelease(charger_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

void charger_handle_request(charger_info_t *charger_info)
{
	charger_state_handler_t *handler = charger_get_state_handler(charger_info->state);
	osStatus os_status;
	int ret = 0;

	if(handler == NULL) {
		return;
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexWait(charger_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	ret = handler->handle_request(charger_info);

	if(ret != 0) {
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexRelease(charger_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}

void charger_handle_response(charger_info_t *charger_info)
{
	charger_state_handler_t *handler = charger_get_state_handler(charger_info->state);
	osStatus os_status;
	int ret = 0;

	if(handler == NULL) {
		return;
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexWait(charger_info->handle_mutex, osWaitForever);

		if(os_status != osOK) {
		}
	}

	ret = handler->handle_response(charger_info);

	if(ret != 0) {
	}

	if(charger_info->handle_mutex) {
		os_status = osMutexRelease(charger_info->handle_mutex);

		if(os_status != osOK) {
		}
	}
}
