

/*================================================================
 *   
 *   
 *   文件名称：card_reader.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月22日 星期五 14时00分44秒
 *   修改日期：2020年05月22日 星期五 15时17分25秒
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
#include "list_utils.h"

#include "channel_config.h"

#ifdef __cplusplus
}
#endif

typedef enum {
} card_type_t;

typedef struct {
	struct list_head list;
	uart_info_t *uart_info;
	channels_info_config_t *channels_info_config;
	uint8_t channel_id;//0xff:没有选择channel
	
} card_reader_info_t;

void free_card_reader_info(channels_info_config_t *channels_info_config);
card_reader_info_t *get_or_alloc_card_reader_info(channels_info_config_t *channels_info_config);
int card_reader_tx_data(card_reader_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
int card_reader_rx_data(card_reader_info_t *info, uint8_t *data, uint16_t size, uint32_t timeout);
#endif //_CARD_READER_H
