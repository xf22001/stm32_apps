

/*================================================================
 *
 *
 *   文件名称：card_reader_handler_zlg.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月24日 星期一 16时49分13秒
 *   修改日期：2021年05月25日 星期二 10时49分27秒
 *   描    述：
 *
 *================================================================*/
#include "card_reader_handler_zlg.h"
#include "uart_data_task.h"

#pragma pack(push, 1)

typedef struct {
	uint8_t frame_len;
	uint8_t type;
	uint8_t cmd;
	uint8_t data_len;
	uint8_t data[0];
} zlg_frame_tx_header_t;

typedef struct {
	uint8_t frame_len;
	uint8_t type;
	uint8_t status;
	uint8_t data_len;
	uint8_t data[0];
} zlg_frame_rx_header_t;

typedef struct {
	uint8_t bcc;
	uint8_t etx;
} zlg_frame_tail_t;

#pragma pack(pop)

typedef enum {
	CARD_READ_HANDLER_STATE_IDLE = 0,
	CARD_READ_HANDLER_STATE_RESET,
} card_read_handler_state_t;

typedef struct {
	card_read_handler_state_t state;
} card_read_handler_ctx_t;

static card_read_handler_ctx_t *card_read_handler_ctx = NULL;

static uint8_t zlg_bcc(uint8_t *data, uint8_t len)
{
	uint8_t bcc = 0;
	int i;

	for(i = 0; i < len; i++) {
		bcc ^= data[i];
	}

	bcc = ~bcc;

	return bcc;
}

static int encode_zlg_cmd(uint8_t *tx_buffer, uint8_t tx_buffer_size, uint8_t type, char cmd, uint8_t *data, uint8_t len)
{
	int ret = -1;
	zlg_frame_tx_header_t *zlg_frame_tx_header = (zlg_frame_tx_header_t *)tx_buffer;
	uint8_t *zlg_frame_data = (uint8_t *)(zlg_frame_tx_header + 1);
	zlg_frame_tail_t *tail = (zlg_frame_tail_t *)(data + len);

	zlg_frame_tx_header->frame_len = sizeof(zlg_frame_tx_header_t) + len + sizeof(zlg_frame_tail_t);

	if(zlg_frame_tx_header->frame_len > tx_buffer_size) {
		return ret;
	}

	zlg_frame_tx_header->type = type;
	zlg_frame_tx_header->cmd = cmd;
	zlg_frame_tx_header->data_len = len;
	memcpy(zlg_frame_data, data, len);
	tail->bcc = zlg_bcc((uint8_t *)zlg_frame_tx_header, sizeof(zlg_frame_tx_header_t) + len);
	tail->etx = 0x03;
	ret = 0;
	return ret;
}

static int decode_zlg_cmd(uint8_t *rx_buffer, uint8_t rx_buffer_size, uint8_t *type, uint8_t *status, uint8_t **data, uint8_t *len)
{
	int ret = -1;
	zlg_frame_rx_header_t *zlg_frame_rx_header = (zlg_frame_rx_header_t *)rx_buffer;
	uint8_t *zlg_frame_data = (uint8_t *)(zlg_frame_rx_header + 1);
	zlg_frame_tail_t *tail;

	if(zlg_frame_rx_header->frame_len != sizeof(zlg_frame_rx_header_t) + zlg_frame_rx_header->data_len + sizeof(zlg_frame_tail_t)) {
		return ret;
	}

	if(zlg_frame_rx_header->frame_len > rx_buffer_size) {
		return ret;
	}

	tail = (zlg_frame_tail_t *)(zlg_frame_data + zlg_frame_rx_header->data_len);

	if(tail->bcc != zlg_bcc((uint8_t *)zlg_frame_rx_header, sizeof(zlg_frame_rx_header_t) + zlg_frame_rx_header->data_len)) {
		return ret;
	}

	if(tail->etx != 0x03) {
		return ret;
	}

	*type = zlg_frame_rx_header->type;
	*status = zlg_frame_rx_header->status;
	*data = zlg_frame_data;
	*len = zlg_frame_rx_header->data_len;

	ret = 0;
	return ret;
}

static int card_reader_reset(card_reader_info_t *card_reader_info)
{
	int ret = 0;

	zlg_frame_tx_header_t *zlg_frame_tx_header = (zlg_frame_tx_header_t *)card_reader_info->tx_buffer;
	zlg_frame_rx_header_t *zlg_frame_rx_header = (zlg_frame_rx_header_t *)card_reader_info->rx_buffer;
	uint8_t type;
	uint8_t status;
	uint8_t *data;
	uint8_t len;
	uint8_t ms = 1;

	ret = encode_zlg_cmd(card_reader_info->tx_buffer, CARD_READ_BUFFER_LENGTH, 0x02, 'L', &ms, sizeof(ms));

	if(ret != 0) {
		return ret;
	}

	ret = uart_tx_rx_data(card_reader_info->uart_info,
	                      card_reader_info->tx_buffer,
	                      zlg_frame_tx_header->frame_len,
	                      card_reader_info->rx_buffer,
	                      CARD_READ_BUFFER_LENGTH,
	                      100);

	if(ret != 0) {
		return ret;
	}

	ret = decode_zlg_cmd(card_reader_info->rx_buffer,
	                     CARD_READ_BUFFER_LENGTH,
	                     &type,
	                     &status,
	                     &data,
	                     &len);

	if(ret != 0) {
		return ret;
	}

	if(zlg_frame_rx_header->status != 0x00) {
		return ret;
	}

	ret = 0;

	return ret;
}

static void uart_data_request(void *fn_ctx, void *chain_ctx)
{
	card_reader_info_t *card_reader_info = (card_reader_info_t *)fn_ctx;
	int ret = -1;

	switch(card_read_handler_ctx->state) {
		case CARD_READ_HANDLER_STATE_IDLE: {
			card_read_handler_ctx->state = CARD_READ_HANDLER_STATE_RESET;
		}
		break;

		case CARD_READ_HANDLER_STATE_RESET: {
			ret = card_reader_reset(card_reader_info);

			if(ret != 0) {
				card_read_handler_ctx->state = CARD_READ_HANDLER_STATE_IDLE;
			}
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
