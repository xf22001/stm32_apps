

/*================================================================
 *
 *
 *   文件名称：card_reader.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月22日 星期五 14时00分44秒
 *   修改日期：2020年05月25日 星期一 08时37分45秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CARD_READER_H
#define _CARD_READER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "os_utils.h"

#include "channels_config.h"

#ifdef __cplusplus
}
#endif

#define CARD_READER_BUFFER_SIZE 128

typedef enum {
	CARD_TYPE_NONE = 0,
	CARD_TYPE_ZHUIRI_ZLG,
} card_type_t;

typedef enum {
	CARD_READER_STATE_NONE = 0,
	CARD_READER_STATE_INIT,
	CARD_READER_STATE_CONFIG,
	CARD_READER_STATE_DETECT,
} card_reader_state_t;

typedef struct {
	uint8_t card_id[32];
	uint8_t remain;
} account_info_t;

typedef struct {
	struct list_head list;
	uart_info_t *uart_info;
	channels_info_config_t *channels_info_config;
	uint8_t tx_buffer[CARD_READER_BUFFER_SIZE];
	uint8_t tx_size;
	uint8_t rx_buffer[CARD_READER_BUFFER_SIZE];
	uint8_t rx_size;

	card_reader_state_t card_reader_state;
	uint8_t channel_id;//0xff:没有选择channel
	card_type_t card_type;
	account_info_t account_info;
} card_reader_info_t;

void free_card_reader_info(card_reader_info_t *card_reader_info);
card_reader_info_t *get_or_alloc_card_reader_info(channels_info_config_t *channels_info_config);
int card_reader_tx_data(card_reader_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
int card_reader_rx_data(card_reader_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
#endif //_CARD_READER_H
