

/*================================================================
 *
 *
 *   文件名称：card_reader.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月22日 星期五 14时00分40秒
 *   修改日期：2020年05月22日 星期五 14时24分41秒
 *   描    述：
 *
 *================================================================*/
#include "card_reader.h"
#include <string.h>

static LIST_HEAD(card_reader_info_list);
static osMutexId card_reader_info_list_mutex = NULL;

static card_reader_info_t *get_card_reader_info(channels_info_config_t *channels_info_config)
{
	card_reader_info_t *card_reader_info = NULL;
	card_reader_info_t *card_reader_info_item = NULL;
	osStatus os_status;

	if(card_reader_info_list_mutex == NULL) {
		return card_reader_info;
	}

	os_status = osMutexWait(card_reader_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(card_reader_info_item, &card_reader_info_list, card_reader_info_t, list) {
		if(card_reader_info_item->channels_info_config == channels_info_config) {
			card_reader_info = card_reader_info_item;
			break;
		}
	}

	os_status = osMutexRelease(card_reader_info_list_mutex);

	if(os_status != osOK) {
	}

	return card_reader_info;
}

void free_card_reader_info(card_reader_info_t *card_reader_info)
{
	osStatus os_status;

	if(card_reader_info == NULL) {
		return;
	}

	if(card_reader_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(card_reader_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&card_reader_info->list);

	os_status = osMutexRelease(card_reader_info_list_mutex);

	if(os_status != osOK) {
	}

	os_free(card_reader_info);
}

int card_reader_set_channel_info_config(card_reader_info_t *card_reader_info, channels_info_config_t *channels_info_config)
{
	int ret = -1;
	uart_info_t *uart_info;

	uart_info = get_or_alloc_uart_info(channel_info_config->huart_card_reader);

	if(uart_info == NULL) {
		return ret;
	}

	card_reader_info->uart_info = uart_info;

	ret = 0;
	return ret;
}

card_reader_info_t *get_or_alloc_card_reader_info(channels_info_config_t *channels_info_config)
{
	card_reader_info_t *card_reader_info = NULL;
	osStatus os_status;

	card_reader_info = get_card_reader_info(channels_info_config);

	if(card_reader_info != NULL) {
		return card_reader_info;
	}

	if(card_reader_info_list_mutex == NULL) {
		osMutexDef(card_reader_info_list_mutex);
		card_reader_info_list_mutex = osMutexCreate(osMutex(card_reader_info_list_mutex));

		if(card_reader_info_list_mutex == NULL) {
			return card_reader_info;
		}
	}

	card_reader_info = (card_reader_info_t *)os_alloc(sizeof(card_reader_info_t));

	if(card_reader_info == NULL) {
		return card_reader_info;
	}

	memset(card_reader_info, 0, sizeof(card_reader_info_t));

	card_reader_info->channels_info_config = channels_info_config;

	os_status = osMutexWait(card_reader_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&card_reader_info->list, &card_reader_info_list);

	os_status = osMutexRelease(card_reader_info_list_mutex);

	if(os_status != osOK) {
	}

	if(card_reader_set_channel_info_config(channels_info_config) != 0) {
		goto failed;
	}

	return card_reader_info;
failed:

	if(card_reader_info != NULL) {
		free_card_reader_info(card_reader_info);
		card_reader_info = NULL;
	}

	return card_reader_info;
}

