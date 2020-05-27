

/*================================================================
 *
 *
 *   文件名称：channels.h
 *   创 建 者：肖飞
 *   创建日期：2020年01月02日 星期四 08时53分41秒
 *   修改日期：2020年05月27日 星期三 15时17分52秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNELS_H
#define _CHANNELS_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "event_helper.h"
#include "list_utils.h"
#include "channels_config.h"
#include "bms_spec.h"

#ifdef __cplusplus
}
#endif

#define CHANNEL_INSTANCES_NUMBER 4
#define CHANNEL_TASK_PERIODIC (50)

typedef enum {
	CHANNEL_STATE_IDLE = 0,
	CHANNEL_STATE_DO_START,
	CHANNEL_STATE_RUNNING,
	CHANNEL_STATE_DO_STOP,
} channel_state_t;

typedef enum {
	CHANNEL_EVENT_TYPE_UNKNOW = 0,
} channel_event_type_t;

typedef struct {
	uint8_t channel_id;
	channel_event_type_t type;
} channel_event_t;

typedef int (*handle_channel_event_t)(channel_event_t *channel_event);

typedef struct {
	uint8_t channel_id;
	channel_state_t state;
	handle_channel_event_t handle_channel_event;

	//状态
	uint8_t gun_connect_state;
	uint8_t battery_available;
	uint8_t output_state;//输出继电器有没有吸合
	uint8_t adhesion_p;
	uint8_t adhesion_n;
	uint8_t gun_lock_state;
	uint8_t bms_charger_enable;
	uint8_t a_f_b_state;
	uint8_t bms_state;
	uint8_t dc_p_temperature;
	uint8_t dc_n_temperature;
	uint8_t insulation_resistor_value;
	uint8_t ver_h;//版本号h 辅板程序版本
	uint8_t ver_l;//版本号l 辅板程序版本
	uint8_t a_f_b_ver_h;
	uint8_t a_f_b_ver_l;
	uint8_t bms_status;//bms工作状态码
	uint8_t door_state;
	uint8_t error_stop_state;
	uint16_t precharge_voltage;//0.1v
	uint8_t precharge_action;//0-停止预充, 1-开始预充, 2-单模块预充
	uint16_t bms_version;
	uint8_t battery_type;//0x01 : '铅酸电池', 0x02 : '镍氢电池', 0x03 : '磷酸电池', 0x04 : '锰酸锂电池', 0x05 : '钴酸锂电池', 0x06 : '三元材料电池', 0x07 : '聚合物电池', 0x08 : '钛酸锂电池', 0xff : '其他电池'
	uint16_t total_battery_rate_capicity;//0.1ah
	uint16_t total_battery_rate_voltage;//0.1v
	uint16_t max_charge_voltage_single_battery;//0.01v 0-24v
	uint8_t max_temperature;// -50
	uint16_t max_charge_voltage;//0.1v 最高允许充电总电压
	uint16_t total_voltage;//0.1v
	uint16_t require_voltage;//0.1v
	uint16_t require_current;//0.1a -400
	uint8_t soc;//0-100%
	uint16_t single_battery_max_voltage;
	uint8_t battery_max_temperature;

	//设置
	uint8_t charger_sn;//充电机编号
	uint8_t gb;//标准 参考bms_standard_t
	uint8_t test_mode;//测试模式
	uint8_t precharge_enable;//允许预充
	uint8_t fault;//充电机故障 unused
	uint8_t charger_power_on;//充电机主板开机状态
	uint8_t manual;//手动模式
	uint8_t adhesion_test;//粘连检测
	uint8_t double_gun_one_car;//双枪充一车
	uint8_t cp_ad;//cp-ad采样
	uint16_t charger_output_voltage;//0.1v
	uint16_t charger_output_current;//0.1a -400
	uint8_t auxiliary_power_type;//12-24v选择 0:12v 1 24v
	uint16_t module_output_voltage;//模块充电电压
	uint16_t channel_max_output_power;//通道最大输出功率
	uint16_t module_output_current;//模块充电电流
	uint16_t max_output_voltage;//0.1v
	uint16_t min_output_voltage;//0.1v
	uint16_t max_output_current;//0.1a -400
	uint16_t min_output_current;//0.1a -400
	uint16_t output_voltage;//0.1v
	uint16_t output_current;//0.1a -400
	uint16_t total_charge_time;//1min 0-600
	uint16_t charge_voltage;//0.1v
	uint16_t charge_current;//0.1a -400
	uint16_t remain_min;//0-600min
	uint16_t total_charge_energy;//0.1kwh 0-1000

	bms_data_settings_t *settings;
} channel_info_t;

typedef struct {
	struct list_head list;
	event_pool_t *event_pool;
	channel_info_t channel_info[CHANNEL_INSTANCES_NUMBER];
	channels_info_config_t *channels_info_config;
} channels_info_t;

void free_channels_info(channels_info_t *channels_info);
channels_info_t *get_or_alloc_channels_info(channels_info_config_t *channels_info_config);
void channels_process_event(channels_info_t *channels_info);
void task_channels(void const *argument);
int send_channel_event(channels_info_t *channels_info, channel_event_t *channel_event, uint32_t timeout);
void task_channel_event(void const *argument);

#endif //_CHANNELS_H
