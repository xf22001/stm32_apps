

/*================================================================
 *   
 *   
 *   文件名称：channels_power_module.h
 *   创 建 者：肖飞
 *   创建日期：2021年03月26日 星期五 15时31分23秒
 *   修改日期：2021年04月09日 星期五 15时46分21秒
 *   描    述：
 *
 *================================================================*/
#ifndef _POWER_MANAGER_H
#define _POWER_MANAGER_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#include "channels.h"

#ifdef __cplusplus
}
#endif

typedef enum {
	POWER_MODULE_ITEM_STATE_IDLE = 0,
	POWER_MODULE_ITEM_STATE_PREPARE_ACTIVE,//准备接通
	POWER_MODULE_ITEM_STATE_READY,//达到接通条件
	POWER_MODULE_ITEM_STATE_ACTIVE,//接通
	POWER_MODULE_ITEM_STATE_ACTIVE_RELAY_CHECK,//自检供电
	POWER_MODULE_ITEM_STATE_PREPARE_DEACTIVE,//准备关断
	POWER_MODULE_ITEM_STATE_DEACTIVE,//关断
	POWER_MODULE_ITEM_STATE_DISABLE,//禁用
} power_module_item_state_t;//状态要与开关板同步

typedef struct {
	uint8_t fault : 1;
	uint8_t connect_timeout : 1;
} power_module_item_error_t;

typedef struct {
	power_module_item_state_t state;

	//模块需求
	uint16_t require_output_voltage;//电源模块需求电压0.1v
	uint16_t require_output_current;//电源模块需求电流0.1a

	//模块设置输出
	uint16_t setting_output_voltage;//电源模块需求电压0.1v
	uint16_t setting_output_current;//电源模块需求电流0.1a

	uint16_t smooth_setting_output_current;//平滑电源模块需求电流0.1a

	//自检输出电压
	uint16_t require_relay_check_voltage;//电源模块需求电压0.1v
	uint16_t require_relay_check_current;//电源模块需求电流0.1a

	//模块输出
	uint16_t module_output_voltage;//电源模块输出电压0.1v
	uint16_t module_output_current;//电源模块输出电流0.1a

	uint16_t module_status;
	uint16_t connect_state;
} power_module_item_status_t;

typedef struct {
	int module_id;
	power_module_item_status_t status;
	power_module_item_error_t error;
} power_module_item_info_t;

typedef int (*channels_power_module_init_t)(void *channels_power_module);
typedef int (*channel_power_module_set_output_t)(channel_info_t *channel_info, uint32_t voltage, uint32_t current);
typedef power_module_item_info_t *(*get_power_module_item_info_t)(channels_info_t *channels_info, uint8_t module_id);

typedef struct {
	channels_power_module_type_t type;
	channels_power_module_init_t init;
	channel_power_module_set_output_t set_output;
	get_power_module_item_info_t get_power_module_item_info;
} channels_power_module_callback_t;

typedef struct {
	channels_info_t *channels_info;
	channels_power_module_callback_t *channels_power_module_callback;
	void *ctx;
	callback_item_t periodic_callback_item;
} channels_power_module_t;

channels_power_module_t *alloc_channels_power_module(channels_info_t *channels_info);

#endif //_POWER_MANAGER_H
