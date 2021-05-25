

/*================================================================
 *   
 *   
 *   文件名称：card_reader.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月24日 星期一 16时08分40秒
 *   修改日期：2021年05月25日 星期二 11时48分37秒
 *   描    述：
 *
 *================================================================*/
#include "card_reader.h"
#include "channels.h"
#include "uart_data_task.h"
#include "card_reader_handler_zlg.h"

card_reader_handler_t *card_reader_handler_sz[] = {
	&card_reader_handler_zlg,
};

static card_reader_handler_t *get_card_reader_handler(card_reader_type_t card_reader_type)
{
	int i;
	card_reader_handler_t *card_reader_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(card_reader_handler_sz); i++) {
		card_reader_handler_t *card_reader_handler_item = card_reader_handler_sz[i];

		if(card_reader_handler_item->card_reader_type == card_reader_type) {
			card_reader_handler = card_reader_handler_item;
		}
	}

	return card_reader_handler;
}

card_reader_info_t *alloc_card_reader_info(channels_info_t *channels_info)
{
	channels_config_t *channels_config = channels_info->channels_config;
	card_reader_config_t *card_reader_config = &channels_config->card_reader_config;
	card_reader_info_t *card_reader_info = (card_reader_info_t *)os_calloc(1, sizeof(card_reader_info_t));

	OS_ASSERT(card_reader_info != NULL);
	card_reader_info->channels_info = channels_info;

	card_reader_info->card_reader_handler = get_card_reader_handler(card_reader_config->card_reader_type);

	if(card_reader_config->huart_card_reader == NULL) {
		return card_reader_info;
	}

	if((card_reader_info->card_reader_handler != NULL) && (card_reader_info->card_reader_handler->init != NULL)) {
		card_reader_info->card_reader_handler->init(card_reader_info);
	}

	return card_reader_info;
}
