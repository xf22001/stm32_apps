

/*================================================================
 *   
 *   
 *   文件名称：voice.h
 *   创 建 者：肖飞
 *   创建日期：2021年05月11日 星期二 16时50分01秒
 *   修改日期：2021年05月11日 星期二 17时36分50秒
 *   描    述：
 *
 *================================================================*/
#ifndef _VOICE_H
#define _VOICE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "channels_config.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	VOICE_STATE_IDLE = 0,
	VOICE_STATE_RUNNING,
} voice_state_t;

typedef struct {
	channels_config_t *channels_config;
	voice_state_t state;
	uint8_t data;
	uint8_t request_data;
} voice_info_t;

voice_info_t *get_or_alloc_voice_info(channels_config_t *channels_config);
void request_voice(voice_info_t *voice_info, uint8_t data);
void handle_voice(voice_info_t *voice_info);

#endif //_VOICE_H
