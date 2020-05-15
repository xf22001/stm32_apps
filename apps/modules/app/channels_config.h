

/*================================================================
 *   
 *   
 *   文件名称：channels_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 11时09分43秒
 *   修改日期：2020年05月15日 星期五 12时47分36秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_CONFIG_H
#define _CHANNELS_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "can_txrx.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	uint8_t id;

	CAN_HandleTypeDef *hcan_com;
} channels_info_config_t;

channels_info_config_t *get_channels_info_config(uint8_t id);
#endif //_CHANNELS_CONFIG_H
