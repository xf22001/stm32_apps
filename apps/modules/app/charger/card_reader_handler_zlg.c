

/*================================================================
 *
 *
 *   文件名称：card_reader_handler_zlg.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月24日 星期一 16时49分13秒
 *   修改日期：2021年07月08日 星期四 10时23分20秒
 *   描    述：
 *
 *================================================================*/
#include "card_reader_handler_zlg.h"
#include "uart_data_task.h"
#include "log.h"

#pragma pack(push, 1)

typedef enum {
	ZLG_STX = 0x20,
	ZLG_ETX = 0x03,
	ZLG_ACK = 0x06,
	ZLG_NAK = 0x15,
} zlg_const_t;

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

typedef struct {
	//ad mode
	uint8_t uart_auto_send : 1;//1为有卡主动发送
	uint8_t uart_int : 1;//1为有卡产生中断
	uint8_t loop : 1;//1为数据输出后,继续检测
	uint8_t halt : 1;//1为最后执行halt
	uint8_t rfu1 : 4;

	//tx mode
	uint8_t alt : 2;//00:TX1、TX2 交替驱动 000000 01:仅 TX1 驱动 10:仅 TX2 驱动 11:TX1、TX2 同时驱动
	uint8_t rfu2 : 6;

	uint8_t req_code;//0x26——IDLE 0x52——ALL
	uint8_t auth_mode;//‘E’——用 E2 密码验证 ‘F’——用直接密码验证 0——不验证
	uint8_t ab_keyab;//0x60——密钥 A 0x61——密钥 B
	uint8_t key[6];//若验证命令=‘E’,则为密钥区号(1 字节) 若验证命令=‘F’,则为密钥(6 字节)
	uint8_t block;//S50:0——63 S70:0——255
} audo_detect_config_t;

typedef struct {
	uint8_t tx_dre;
	uint16_t req;
	uint8_t select;
	uint8_t sn_length;
	uint32_t sn;
	uint32_t data[4];
} card_data_t;

typedef struct {
	uint8_t block;
	uint32_t data[4];
} block_card_data_t;

#pragma pack(pop)

typedef enum {
	CARD_READER_HANDLER_STATE_NONE = 0,
	CARD_READER_HANDLER_STATE_SET_BAUDRATE,
	CARD_READER_HANDLER_STATE_RESET,
	CARD_READER_HANDLER_STATE_IDLE,
	CARD_READER_HANDLER_STATE_AUTO_DETECT,
	CARD_READER_HANDLER_STATE_CARD_DATA_READ,
	CARD_READER_HANDLER_STATE_CARD_DATA_WRITE,
} card_reader_handler_state_t;

typedef struct {
	card_reader_handler_state_t state;
	card_reader_handler_state_t request_state;
	card_data_t card_data;
	uint8_t init_retry;
} card_reader_handler_ctx_t;

static char *get_card_reader_handler_state_des(card_reader_handler_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CARD_READER_HANDLER_STATE_NONE);
			add_des_case(CARD_READER_HANDLER_STATE_SET_BAUDRATE);
			add_des_case(CARD_READER_HANDLER_STATE_RESET);
			add_des_case(CARD_READER_HANDLER_STATE_IDLE);
			add_des_case(CARD_READER_HANDLER_STATE_AUTO_DETECT);
			add_des_case(CARD_READER_HANDLER_STATE_CARD_DATA_READ);
			add_des_case(CARD_READER_HANDLER_STATE_CARD_DATA_WRITE);

		default: {
		}
		break;
	}

	return des;
}

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
	zlg_frame_tail_t *tail = (zlg_frame_tail_t *)(zlg_frame_data + len);

	zlg_frame_tx_header->frame_len = sizeof(zlg_frame_tx_header_t) + len + sizeof(zlg_frame_tail_t);

	if(zlg_frame_tx_header->frame_len > tx_buffer_size) {
		return ret;
	}

	zlg_frame_tx_header->type = type;
	zlg_frame_tx_header->cmd = cmd;
	zlg_frame_tx_header->data_len = len;
	memcpy(zlg_frame_data, data, len);
	tail->bcc = zlg_bcc((uint8_t *)zlg_frame_tx_header, sizeof(zlg_frame_tx_header_t) + len);
	tail->etx = ZLG_ETX;
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

	if(tail->etx != ZLG_ETX) {
		return ret;
	}

	*type = zlg_frame_rx_header->type;
	*status = zlg_frame_rx_header->status;
	*data = zlg_frame_data;
	*len = zlg_frame_rx_header->data_len;

	ret = 0;
	return ret;
}

static int card_reader_set_baudrate(card_reader_info_t *card_reader_info)
{
	int ret = -1;
	int received;
	card_reader_info->tx_buffer[0] = ZLG_STX;
	received = uart_tx_rx_data(card_reader_info->uart_info,
	                           card_reader_info->tx_buffer,
	                           1,
	                           card_reader_info->rx_buffer,
	                           1,
	                           30);

	if(received == 1) {
		if(card_reader_info->rx_buffer[0] == ZLG_ACK) {
			ret = 0;
		}
	}

	return ret;
}

static int card_reader_reset(card_reader_info_t *card_reader_info)
{
	int ret = -1;
	int received;

	zlg_frame_tx_header_t *zlg_frame_tx_header = (zlg_frame_tx_header_t *)card_reader_info->tx_buffer;
	zlg_frame_rx_header_t *zlg_frame_rx_header = (zlg_frame_rx_header_t *)card_reader_info->rx_buffer;
	uint8_t type;
	uint8_t status;
	uint8_t *data;
	uint8_t len;
	uint8_t ms = 1;

	if(encode_zlg_cmd(card_reader_info->tx_buffer, CARD_READ_BUFFER_LENGTH, 0x02, 'L', &ms, sizeof(ms)) != 0) {
		debug("");
		return ret;
	}

	received = uart_tx_rx_data(card_reader_info->uart_info,
	                           card_reader_info->tx_buffer,
	                           zlg_frame_tx_header->frame_len,
	                           card_reader_info->rx_buffer,
	                           CARD_READ_BUFFER_LENGTH,
	                           100);

	if(received <= 0) {
		debug("");
		return ret;
	}

	if(decode_zlg_cmd(card_reader_info->rx_buffer,
	                  received,
	                  &type,
	                  &status,
	                  &data,
	                  &len) != 0) {
		debug("");
		return ret;
	}

	if(zlg_frame_rx_header->status != 0x00) {
		debug("");
		return ret;
	}

	ret = 0;

	return ret;
}

static int card_reader_auto_detect(card_reader_info_t *card_reader_info)
{
	int ret = -1;
	int received;

	zlg_frame_tx_header_t *zlg_frame_tx_header = (zlg_frame_tx_header_t *)card_reader_info->tx_buffer;
	zlg_frame_rx_header_t *zlg_frame_rx_header = (zlg_frame_rx_header_t *)card_reader_info->rx_buffer;
	uint8_t type;
	uint8_t status;
	uint8_t *data;
	uint8_t len;
	audo_detect_config_t audo_detect_config = {0};

	audo_detect_config.uart_auto_send = 1;
	audo_detect_config.alt = 0x03;
	audo_detect_config.req_code = 0x26;
	audo_detect_config.auth_mode = 'F';
	audo_detect_config.ab_keyab = 0x60;
	memset(audo_detect_config.key, 0xff, sizeof(audo_detect_config.key));
	audo_detect_config.block = 0x08;

	if(encode_zlg_cmd(card_reader_info->tx_buffer, CARD_READ_BUFFER_LENGTH, 0x02, 'N', (uint8_t *)&audo_detect_config, sizeof(audo_detect_config)) != 0) {
		debug("");
		return ret;
	}

	received = uart_tx_rx_data(card_reader_info->uart_info,
	                           card_reader_info->tx_buffer,
	                           zlg_frame_tx_header->frame_len,
	                           card_reader_info->rx_buffer,
	                           sizeof(zlg_frame_rx_header_t) + sizeof(zlg_frame_tail_t),
	                           100);

	if(received <= 0) {
		debug("");
		return ret;
	}

	if(decode_zlg_cmd(card_reader_info->rx_buffer,
	                  received,
	                  &type,
	                  &status,
	                  &data,
	                  &len) != 0) {
		debug("");
		return ret;
	}

	if(zlg_frame_rx_header->status != 0x00) {
		debug("");
		return ret;
	}

	ret = 0;

	return ret;
}

static int card_reader_card_data_read(card_reader_info_t *card_reader_info)
{
	int ret = -1;
	int received;

	card_reader_handler_ctx_t *card_reader_handler_ctx = (card_reader_handler_ctx_t *)card_reader_info->ctx;
	zlg_frame_rx_header_t *zlg_frame_rx_header = (zlg_frame_rx_header_t *)card_reader_info->rx_buffer;
	uint8_t *zlg_frame_data = (uint8_t *)(zlg_frame_rx_header + 1);
	uint8_t type;
	uint8_t status;
	uint8_t *data;
	uint8_t len;

	received = uart_rx_data(card_reader_info->uart_info,
	                        card_reader_info->rx_buffer,
	                        sizeof(zlg_frame_rx_header_t) + sizeof(card_data_t) + sizeof(zlg_frame_tail_t),
	                        card_reader_info->card_data_timeout);

	if(received <= 0) {
		debug("");
		return ret;
	}

	if(decode_zlg_cmd(card_reader_info->rx_buffer,
	                  received,
	                  &type,
	                  &status,
	                  &data,
	                  &len) != 0) {
		debug("");
		return ret;
	}

	if(zlg_frame_rx_header->status != 0x00) {
		debug("");
		return ret;
	}

	memcpy(&card_reader_handler_ctx->card_data, zlg_frame_data, sizeof(card_data_t));

	ret = 0;

	return ret;
}

static int card_reader_card_data_write(card_reader_info_t *card_reader_info)
{
	int ret = -1;
	int received;

	card_reader_handler_ctx_t *card_reader_handler_ctx = (card_reader_handler_ctx_t *)card_reader_info->ctx;
	zlg_frame_tx_header_t *zlg_frame_tx_header = (zlg_frame_tx_header_t *)card_reader_info->tx_buffer;
	zlg_frame_rx_header_t *zlg_frame_rx_header = (zlg_frame_rx_header_t *)card_reader_info->rx_buffer;
	uint8_t type;
	uint8_t status;
	uint8_t *data;
	uint8_t len;
	block_card_data_t block_card_data = {0};

	block_card_data.block = 0x08;
	memcpy(block_card_data.data, card_reader_handler_ctx->card_data.data, sizeof(block_card_data.data));

	if(encode_zlg_cmd(card_reader_info->tx_buffer, CARD_READ_BUFFER_LENGTH, 0x02, 'H', (uint8_t *)&block_card_data, sizeof(block_card_data)) != 0) {
		debug("");
		return ret;
	}

	received = uart_tx_rx_data(card_reader_info->uart_info,
	                           card_reader_info->tx_buffer,
	                           zlg_frame_tx_header->frame_len,
	                           card_reader_info->rx_buffer,
	                           sizeof(zlg_frame_rx_header_t) + sizeof(zlg_frame_tail_t),
	                           100);

	if(received <= 0) {
		debug("");
		return ret;
	}

	if(decode_zlg_cmd(card_reader_info->rx_buffer,
	                  received,
	                  &type,
	                  &status,
	                  &data,
	                  &len) != 0) {
		debug("");
		return ret;
	}

	if(zlg_frame_rx_header->status != 0x00) {
		debug("");
		return ret;
	}

	ret = 0;

	return ret;
}

static void handle_card_reader_state(card_reader_info_t *card_reader_info, card_reader_handler_ctx_t *card_reader_handler_ctx)
{
	int ret;

	//debug("state:%s", get_card_reader_handler_state_des(card_reader_handler_ctx->state));

	switch(card_reader_handler_ctx->state) {
		case CARD_READER_HANDLER_STATE_SET_BAUDRATE: {
			ret = card_reader_set_baudrate(card_reader_info);

			if(ret == 0) {
				card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_RESET;
			} else {
				card_reader_handler_ctx->init_retry++;

				if(card_reader_handler_ctx->init_retry >= 10) {
					card_reader_handler_ctx->init_retry = 0;
					card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_RESET;
					debug("");
				}
			}
		}
		break;

		case CARD_READER_HANDLER_STATE_RESET: {
			ret = card_reader_reset(card_reader_info);

			if(ret == 0) {
				card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_IDLE;
				set_card_reader_state(card_reader_info, CARD_READER_STATE_IDLE);
			} else {
				card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_SET_BAUDRATE;
			}
		}
		break;

		case CARD_READER_HANDLER_STATE_IDLE: {
			if(card_reader_handler_ctx->request_state == CARD_READER_HANDLER_STATE_AUTO_DETECT) {
				card_reader_handler_ctx->request_state = CARD_READER_HANDLER_STATE_NONE;

				card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_AUTO_DETECT;
				set_card_reader_state(card_reader_info, CARD_READER_STATE_RUNNING);
			}
		}
		break;

		case CARD_READER_HANDLER_STATE_AUTO_DETECT: {
			ret = card_reader_auto_detect(card_reader_info);

			if(ret != 0) {
				debug("");
			}

			card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_CARD_DATA_READ;
		}
		break;

		case CARD_READER_HANDLER_STATE_CARD_DATA_READ: {
			memset(&card_reader_info->card_reader_data, 0, sizeof(card_reader_data_t));
			ret = card_reader_card_data_read(card_reader_info);

			if(ret == 0) {
				debug("tx_dre:%02x", card_reader_handler_ctx->card_data.tx_dre);
				debug("req:%04x", card_reader_handler_ctx->card_data.req);
				debug("select:%02x", card_reader_handler_ctx->card_data.select);
				debug("sn_length:%02x", card_reader_handler_ctx->card_data.sn_length);
				debug("sn:%08x", card_reader_handler_ctx->card_data.sn);
				debug("data[0]:%08x", card_reader_handler_ctx->card_data.data[0]);
				debug("data[1]:%08x", card_reader_handler_ctx->card_data.data[1]);
				debug("data[2]:%08x", card_reader_handler_ctx->card_data.data[2]);
				debug("data[3]:%08x", card_reader_handler_ctx->card_data.data[3]);
				card_reader_info->card_reader_data.id = card_reader_handler_ctx->card_data.sn;
				do_callback_chain(card_reader_info->card_reader_callback_chain, &card_reader_info->card_reader_data);
			} else {
				do_callback_chain(card_reader_info->card_reader_callback_chain, NULL);
			}

			card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_IDLE;
			set_card_reader_state(card_reader_info, CARD_READER_STATE_IDLE);
		}
		break;

		case CARD_READER_HANDLER_STATE_CARD_DATA_WRITE: {
			ret = card_reader_card_data_write(card_reader_info);

			if(ret != 0) {
				debug("");
			}

			card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_IDLE;
			set_card_reader_state(card_reader_info, CARD_READER_STATE_IDLE);
		}
		break;

		default: {
		}
		break;
	}
}

static void uart_data_request(void *fn_ctx, void *chain_ctx)
{
	card_reader_info_t *card_reader_info = (card_reader_info_t *)fn_ctx;
	card_reader_handler_ctx_t *card_reader_handler_ctx = (card_reader_handler_ctx_t *)card_reader_info->ctx;

	handle_card_reader_state(card_reader_info, card_reader_handler_ctx);
}

static void card_reader_ctrl_cmd(void *fn_ctx, void *chain_ctx)
{
	card_reader_info_t *card_reader_info = (card_reader_info_t *)fn_ctx;
	card_reader_handler_ctx_t *card_reader_handler_ctx = (card_reader_handler_ctx_t *)card_reader_info->ctx;
	card_reader_ctrl_cmd_info_t *card_reader_ctrl_cmd_info = (card_reader_ctrl_cmd_info_t *)chain_ctx;

	card_reader_ctrl_cmd_info->code = -1;

	if(card_reader_handler_ctx->request_state != CARD_READER_HANDLER_STATE_NONE) {
		debug("");
		return;
	}

	switch(card_reader_ctrl_cmd_info->cmd) {
		case CARD_READER_CTRL_CMD_START: {
			if(card_reader_handler_ctx->state == CARD_READER_HANDLER_STATE_IDLE) {
				card_reader_start_t *card_reader_start = (card_reader_start_t *)card_reader_ctrl_cmd_info->args;

				if(remove_callback(card_reader_info->card_reader_callback_chain, &card_reader_info->card_reader_callback_item) != 0) {
				}

				card_reader_info->card_reader_callback_item.fn = card_reader_start->fn;
				card_reader_info->card_reader_callback_item.fn_ctx = card_reader_start->fn_ctx;

				if(register_callback(card_reader_info->card_reader_callback_chain, &card_reader_info->card_reader_callback_item) != 0) {
				}

				card_reader_info->card_data_timeout = card_reader_start->timeout;

				card_reader_handler_ctx->request_state = CARD_READER_HANDLER_STATE_AUTO_DETECT;
				card_reader_ctrl_cmd_info->code = 0;
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
	card_reader_handler_ctx_t *card_reader_handler_ctx;

	card_reader_handler_ctx = os_calloc(1, sizeof(card_reader_handler_ctx_t));
	OS_ASSERT(card_reader_handler_ctx != NULL);
	card_reader_handler_ctx->state = CARD_READER_HANDLER_STATE_SET_BAUDRATE;
	set_card_reader_state(card_reader_info, CARD_READER_STATE_INIT);

	uart_data_task_info = get_or_alloc_uart_data_task_info(channels_config->card_reader_config.huart_card_reader);
	OS_ASSERT(uart_data_task_info != NULL);

	remove_callback(card_reader_info->card_reader_ctrl_cmd_callback_chain, &card_reader_info->card_reader_ctrl_cmd_callback_item);
	remove_uart_data_task_info_cb(uart_data_task_info, &card_reader_info->uart_data_request_cb);

	if(card_reader_info->ctx != NULL) {
		os_free(card_reader_info->ctx);
	}

	card_reader_info->ctx = card_reader_handler_ctx;
	card_reader_info->uart_info = get_or_alloc_uart_info(channels_config->card_reader_config.huart_card_reader);
	OS_ASSERT(card_reader_info->uart_info != NULL);
	card_reader_info->uart_data_request_cb.fn = uart_data_request;
	card_reader_info->uart_data_request_cb.fn_ctx = card_reader_info;
	OS_ASSERT(add_uart_data_task_info_cb(uart_data_task_info, &card_reader_info->uart_data_request_cb) == 0);
	card_reader_info->card_reader_ctrl_cmd_callback_item.fn = card_reader_ctrl_cmd;
	card_reader_info->card_reader_ctrl_cmd_callback_item.fn_ctx = card_reader_info;
	OS_ASSERT(register_callback(card_reader_info->card_reader_ctrl_cmd_callback_chain, &card_reader_info->card_reader_ctrl_cmd_callback_item) == 0);

	return ret;
}

card_reader_handler_t card_reader_handler_zlg = {
	.card_reader_type = CARD_READER_TYPE_ZLG,
	.init = init,
};
