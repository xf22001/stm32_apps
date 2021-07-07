

/*================================================================
 *
 *
 *   文件名称：card_reader.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月24日 星期一 16时08分40秒
 *   修改日期：2021年07月07日 星期三 16时58分19秒
 *   描    述：
 *
 *================================================================*/
#include "card_reader.h"
#include "channels.h"
#include "uart_data_task.h"
#include "card_reader_handler_zlg.h"

#include "log.h"

static card_reader_handler_t *card_reader_handler_sz[] = {
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

int start_card_reader_cb(card_reader_info_t *card_reader_info, card_reader_start_t *card_reader_start)
{
	int ret = -1;
	card_reader_ctrl_cmd_info_t card_reader_ctrl_cmd_info = {0};

	card_reader_ctrl_cmd_info.cmd = CARD_READER_CTRL_CMD_START;
	card_reader_ctrl_cmd_info.args = card_reader_start;

	do_callback_chain(card_reader_info->card_reader_ctrl_cmd_callback_chain, &card_reader_ctrl_cmd_info);

	return card_reader_ctrl_cmd_info.code;
}

static char *get_card_reader_state_des(card_reader_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CARD_READER_STATE_INIT);
			add_des_case(CARD_READER_STATE_IDLE);
			add_des_case(CARD_READER_STATE_RUNNING);

		default: {
		}
		break;
	}

	return des;
}

void set_card_reader_state(card_reader_info_t *card_reader_info, card_reader_state_t state)
{
	debug("set card reader state %s -> %s", get_card_reader_state_des(card_reader_info->state), get_card_reader_state_des(state));
	card_reader_info->state = state;
}

card_reader_state_t get_card_reader_state(card_reader_info_t *card_reader_info)
{
	return card_reader_info->state;
}

card_reader_info_t *alloc_card_reader_info(channels_info_t *channels_info)
{
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	card_reader_settings_t *card_reader_settings = &channels_settings->card_reader_settings;
	card_reader_info_t *card_reader_info = (card_reader_info_t *)os_calloc(1, sizeof(card_reader_info_t));

	OS_ASSERT(card_reader_info != NULL);
	card_reader_info->channels_info = channels_info;

	card_reader_info->card_reader_callback_chain = alloc_callback_chain();
	OS_ASSERT(card_reader_info->card_reader_callback_chain != NULL);

	card_reader_info->card_reader_ctrl_cmd_callback_chain = alloc_callback_chain();
	OS_ASSERT(card_reader_info->card_reader_ctrl_cmd_callback_chain != NULL);

	card_reader_info->card_reader_handler = get_card_reader_handler(card_reader_settings->type);

	if((card_reader_info->card_reader_handler != NULL) && (card_reader_info->card_reader_handler->init != NULL)) {
		card_reader_info->card_reader_handler->init(card_reader_info);
	}

	return card_reader_info;
}
