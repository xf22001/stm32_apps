

/*================================================================
 *   
 *   
 *   文件名称：card_reader_handler_zlg.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月24日 星期一 16时49分13秒
 *   修改日期：2021年05月24日 星期一 16时53分17秒
 *   描    述：
 *
 *================================================================*/
#include "card_reader_handler_zlg.h"
#include "uart_data_task.h"

static void uart_data_request(void *fn_ctx, void *chain_ctx)
{
	//card_reader_info_t *card_reader_info = (card_reader_info_t *)fn_ctx;
}

static int init(void *_card_reader_info)
{
	int ret = 0;
	card_reader_info_t *card_reader_info = (card_reader_info_t *)_card_reader_info;
	channels_info_t *channels_info = card_reader_info->channels_info;
	channels_config_t *channels_config = channels_info->channels_config;
	uart_data_task_info_t *uart_data_task_info;

	card_reader_info->uart_info = get_or_alloc_uart_info(channels_config->card_reader_config.huart_card_reader);
	OS_ASSERT(card_reader_info->uart_info != NULL);

	uart_data_task_info = get_or_alloc_uart_data_task_info(channels_config->card_reader_config.huart_card_reader);
	OS_ASSERT(uart_data_task_info != NULL);

	card_reader_info->uart_data_request_cb.fn = uart_data_request;
	card_reader_info->uart_data_request_cb.fn_ctx = card_reader_info;
	add_uart_data_task_info_cb(uart_data_task_info, &card_reader_info->uart_data_request_cb);

	return ret;
}

card_reader_handler_t card_reader_handler_zlg = {
	.card_reader_type = CARD_READER_TYPE_ZLG,
	.init = init,
};
