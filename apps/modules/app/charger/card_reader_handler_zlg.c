

/*================================================================
 *
 *
 *   文件名称：card_reader_handler_zlg.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月24日 星期一 16时49分13秒
 *   修改日期：2021年05月24日 星期一 17时38分38秒
 *   描    述：
 *
 *================================================================*/
#include "card_reader_handler_zlg.h"
#include "uart_data_task.h"

typedef struct {
	uint8_t frame_len;
	uint8_t type;
	uint8_t cmd;
	uint8_t data_len;
	uint8_t data[0];
} zlg_frame_header_t;

typedef struct {
	uint8_t bcc;
	uint8_t etx;
} zlg_frame_tail_t;

typedef enum {
	CARD_READ_HANDLER_STATE_RESET = 0,
	CARD_READ_HANDLER_STATE_IDLE,
} card_read_handler_state_t;

typedef struct {
	card_read_handler_state_t state;
} card_read_handler_ctx_t;

static card_read_handler_ctx_t *card_read_handler_ctx = NULL;

static int card_reader_reset(card_reader_info_t *card_reader_info)
{
	int ret = 0;

	zlg_frame_header_t *header = (zlg_frame_header_t *)card_reader_info->tx_buffer;
	uint8_t *data = (uint8_t *)(header + 1);
	zlg_frame_tail_t *tail = (zlg_frame_tail_t *)(data + 1);

	header->frame_len = 0x07;
	header->type = 0x02;
	header->cmd = 'L';
	header->data_len = 1;
	data[0] = 0x01;
	tail->bcc = 0x00;
	tail->etx = 0x03;
	

	return ret;
}

static void uart_data_request(void *fn_ctx, void *chain_ctx)
{
	card_reader_info_t *card_reader_info = (card_reader_info_t *)fn_ctx;
	int ret = -1;

	switch(card_read_handler_ctx->state) {
		case CARD_READ_HANDLER_STATE_RESET: {
			ret = card_reader_reset(card_reader_info);

			if(ret == 0) {
				card_read_handler_ctx->state = CARD_READ_HANDLER_STATE_IDLE;
			}
		}
		break;

		case CARD_READ_HANDLER_STATE_IDLE: {
		}
		break;

		default: {
		}
		break;
	}
}

static int init(void *_card_reader_info)
{
	int ret = 0;
	card_reader_info_t *card_reader_info = (card_reader_info_t *)_card_reader_info;
	channels_info_t *channels_info = card_reader_info->channels_info;
	channels_config_t *channels_config = channels_info->channels_config;
	uart_data_task_info_t *uart_data_task_info;

	if(card_read_handler_ctx == NULL) {
		card_read_handler_ctx = (card_read_handler_ctx_t *)os_calloc(1, sizeof(card_read_handler_ctx_t));
		OS_ASSERT(card_read_handler_ctx != NULL);
	}

	card_read_handler_ctx->state = CARD_READ_HANDLER_STATE_RESET;

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
