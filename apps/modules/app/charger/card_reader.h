

/*================================================================
 *
 *
 *   文件名称：card_reader.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月24日 星期一 16时08分43秒
 *   修改日期：2021年07月07日 星期三 16时58分58秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CARD_READER_H
#define _CARD_READER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "usart_txrx.h"
#include "callback_chain.h"
#include "channels_config.h"
#include "channels.h"

#ifdef __cplusplus
}
#endif

#define CARD_READ_BUFFER_LENGTH 64

typedef enum {
	CARD_READER_CTRL_CMD_NONE = 0,
	CARD_READER_CTRL_CMD_START,
} card_reader_ctrl_cmd_t;

typedef struct {
	card_reader_ctrl_cmd_t cmd;
	void *args;
	int code;
} card_reader_ctrl_cmd_info_t;

typedef struct {
	card_reader_ctrl_cmd_t cmd;
	callback_fn_t fn;
	void *fn_ctx;
	uint32_t timeout;
} card_reader_start_t;

typedef int (*card_reader_handler_init_t)(void *_card_reader_info);
typedef int (*card_reader_handler_deinit_t)(void *_card_reader_info);

typedef struct {
	card_reader_type_t card_reader_type;
	card_reader_handler_init_t init;
	card_reader_handler_deinit_t deinit;
} card_reader_handler_t;

typedef struct {
	uint64_t id;
} card_reader_data_t;

typedef enum {
	CARD_READER_STATE_INIT = 0,
	CARD_READER_STATE_IDLE,
	CARD_READER_STATE_RUNNING,
} card_reader_state_t;

typedef struct {
	card_reader_state_t state;
	channels_info_t *channels_info;
	card_reader_handler_t *card_reader_handler;

	uart_info_t *uart_info;
	callback_item_t uart_data_request_cb;
	uint8_t tx_buffer[CARD_READ_BUFFER_LENGTH];
	uint8_t rx_buffer[CARD_READ_BUFFER_LENGTH];
	uint32_t card_data_timeout;

	//card_info;
	callback_chain_t *card_reader_callback_chain;
	callback_item_t card_reader_callback_item;

	callback_chain_t *card_reader_ctrl_cmd_callback_chain;
	callback_item_t card_reader_ctrl_cmd_callback_item;

	card_reader_data_t card_reader_data;
	void *ctx;
} card_reader_info_t;

int start_card_reader_cb(card_reader_info_t *card_reader_info, card_reader_start_t *card_reader_start);
void set_card_reader_state(card_reader_info_t *card_reader_info, card_reader_state_t state);
card_reader_state_t get_card_reader_state(card_reader_info_t *card_reader_info);
card_reader_info_t *alloc_card_reader_info(channels_info_t *channels_info);

#endif //_CARD_READER_H
