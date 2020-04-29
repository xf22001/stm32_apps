

/*================================================================
 *   
 *   
 *   文件名称：can_config.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月17日 星期五 09时20分02秒
 *   修改日期：2020年04月29日 星期三 08时55分37秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CAN_CONFIG_H
#define _CAN_CONFIG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

typedef struct {
	CAN_HandleTypeDef *hcan;
	CAN_HandleTypeDef *config_can;
	uint32_t filter_number;
	uint32_t filter_fifo;
	uint32_t filter_id;
	uint32_t filter_maskid;
	uint8_t filter_rtr;
	uint8_t filter_ext;
} can_config_t;

typedef struct {
	uint32_t unused : 1;
	uint32_t rtr : 1;
	uint32_t ext : 1;
	uint32_t id : 29;
} can_filter_id_t;

typedef struct {
	uint16_t l;
	uint16_t h;
} can_filter_id_lh_t;

typedef union {
	can_filter_id_t s;
	can_filter_id_lh_t s_lh;
} u_can_filter_id_t;

can_config_t *get_can_config(CAN_HandleTypeDef *hcan);

#endif //_CAN_CONFIG_H
