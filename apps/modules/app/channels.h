

/*================================================================
 *
 *
 *   文件名称：channels.h
 *   创 建 者：肖飞
 *   创建日期：2020年01月02日 星期四 08时53分41秒
 *   修改日期：2020年05月31日 星期日 14时28分52秒
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

typedef union {
	    uint8_t id[32];
	    uint32_t card_id;
	} account_id_t;

typedef struct {
	uint8_t channel_id;//枪口号
	uint8_t account_type;//账号类型 1:账号 2:卡号
	account_id_t account_id;//账号
	uint8_t serial_num[32];//流水号
	uint32_t start_stamp;//充电起始时间
	uint32_t stop_stamp;//结束时间
	uint16_t charge_time;//充电时长
	uint8_t start_soc;//起始soc
	uint8_t end_soc;//结束soc
	uint32_t charge_start_elcmeter;//开始电表读数
	uint32_t charge_energy;//充电电量
	uint32_t account_money;//充电开始时卡金额
	uint32_t gift_account_money;//充电开始时卡金额
	uint32_t spend_money;//消费金额
	uint32_t service_money;//服务费
	uint32_t park_money;//占桩费
	uint16_t park_duration;//占桩时长
	uint8_t stop_reason;//充电机停机原因
	uint8_t auxiliary_power_type;//辅助电源
	uint8_t vin[17];//车辆识别码
	uint16_t segment_charge_energy[12];//6小时连续充电量
	uint32_t plug_in_stamp;//拔枪时间  令狐充使用
	uint32_t pull_out_stamp;//插枪时间  令狐充使用
	uint8_t  charge_start_type;//充电机开机原因 1:刷卡开机 2:app开机  3:密码开机
	uint16_t bms_version;
	uint8_t battery_type;//0x01 : '铅酸电池', 0x02 : '镍氢电池', 0x03 : '磷酸电池', 0x04 : '锰酸锂电池', 0x05 : '钴酸锂电池', 0x06 : '三元材料电池', 0x07 : '聚合物电池', 0x08 : '钛酸锂电池', 0xff : '其他电池'
	uint16_t total_battery_rate_capicity;//0.1ah
	uint16_t total_battery_rate_voltage;//0.1v
	uint16_t max_charge_voltage_single_battery;//0.01v 0-24v
	uint8_t max_temperature;// -50
	uint16_t max_charge_voltage;//0.1v 最高允许充电总电压
	uint8_t synchronized;//是否上传
} channel_record_t;

#define EEPROM_CHANNEL_RECORDS_NUMBER 500

typedef struct {
	uint16_t index;
	channel_record_t channel_record;
	uint8_t crc;//校验码
} eeprom_channel_record_t;

typedef struct {
	uint16_t start_index;
	uint16_t end_index;
} eeprom_channel_records_info_t;

typedef struct {
	eeprom_channel_records_info_t eeprom_channel_records_info;
	eeprom_channel_record_t eeprom_channel_record[EEPROM_CHANNEL_RECORDS_NUMBER];
} eeprom_channel_records_t;

typedef struct {
	uint8_t charger_sn;//充电机编号
	uint8_t gb;//标准 参考bms_standard_t
	uint8_t test_mode;//测试模式
	uint8_t precharge_enable;//允许预充
	uint8_t manual;//手动模式
	uint8_t adhesion_test;//粘连检测//?
	uint8_t double_gun_one_car;//双枪充一车
	uint8_t cp_ad;//cp-ad采样
	uint16_t module_output_voltage;//0.1v 模块充电电压
	uint16_t module_output_current;//0.1a -400 模块充电电流
	uint16_t channel_max_output_power;//w 通道最大输出功率
	uint16_t max_output_voltage;//0.1v
	uint16_t min_output_voltage;//0.1v
	uint16_t max_output_current;//0.1a -400
	uint16_t min_output_current;//0.1a -400
	uint8_t auxiliary_power_type;//12-24v选择 0:12v 1 24v
} channel_settings_t;

typedef enum {
	CHANNEL_STATE_IDLE = 0,
	CHANNEL_STATE_START,
	CHANNEL_STATE_RUNNING,
	CHANNEL_STATE_STOP,
} channel_state_t;

typedef enum {
	CHANNEL_REQUEST_STATE_NONE = 0,
	CHANNEL_REQUEST_STATE_START,
	CHANNEL_REQUEST_STATE_STOP,
} channel_request_state_t;

typedef enum {
	CHANNEL_EVENT_TYPE_UNKNOW = 0,
} channel_event_type_t;

typedef struct {
	uint8_t channel_id;
	channel_event_type_t type;
} channel_event_t;

typedef int (*handle_channel_event_t)(void *channel_info, channel_event_t *channel_event);
typedef void (*handle_channel_periodic_t)(void *channel_info);

typedef struct {
	uint8_t channel_id;
	handle_channel_event_t handle_channel_event;
	handle_channel_periodic_t handle_channel_periodic;
} channel_callback_t;

typedef struct {
	uint8_t channel_id;
	channel_state_t channel_state;
	channel_request_state_t channel_request_state;
	channel_callback_t *callback;
	void *channels_info;
	struct list_head list;//running list

	//输入状态
	uint8_t gun_connect_state;//是否插枪
	uint8_t battery_available;//电池是否存在
	uint8_t output_state;//输出继电器有没有吸合
	uint8_t adhesion_p;//p粘连
	uint8_t adhesion_n;//n粘连
	uint8_t gun_lock_state;//枪是否上锁
	uint8_t bms_charge_enable;//允许充电
	uint8_t a_f_b_state;//辅助功能板连接状态
	uint8_t bms_state;//bms 状态
	uint8_t dc_p_temperature;//p端温度报警
	uint8_t dc_n_temperature;//n端温度报警
	uint8_t insulation_resistor_value;//绝缘电阻值
	uint8_t ver_h;//版本号h 辅板程序版本
	uint8_t ver_l;//版本号l 辅板程序版本
	uint16_t a_f_b_ver;//辅助功能板版本
	uint8_t bms_status;//bms工作状态码
	uint8_t door_state;//门禁报警
	uint8_t error_stop_state;//急停报警
	uint16_t precharge_voltage;//0.1v
	uint8_t precharge_action;//0-停止预充, 1-开始预充, 2-单模块预充
	uint16_t bms_version;//bms 版本
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

	uint8_t fault;//充电机故障 unused
	uint8_t charger_power_on;//充电机主板开机状态

	//输出状态
	uint16_t charger_output_voltage;//0.1v
	uint16_t charger_output_current;//0.1a -400
	uint16_t output_voltage;//0.1v
	uint16_t output_current;//0.1a -400
	uint16_t total_charge_time;//1min 0-600
	uint16_t charge_voltage;//0.1v
	uint16_t charge_current;//0.1a -400
	uint16_t remain_min;//0-600min
	uint16_t total_charge_energy;//0.1kwh 0-1000

	channel_settings_t channel_settings;
	bms_data_settings_t bms_data_settings;
} channel_info_t;

//请求切换
typedef enum {
	CHANNELS_OUTPUT_SWITCH_STATE_NONE = 0,//正常输出
	CHANNELS_OUTPUT_SWITCH_STATE_DISABLE_PWM,//清所有辅板PWM配置，关辅板所有脉冲
	CHANNELS_OUTPUT_SWITCH_STATE__WAIT_NO_PWM,//等待所有辅板请求关电源
	CHANNELS_OUTPUT_SWITCH_STATE_DISABLE_POWER_OUTPUT,//关电源模块输出;
	CHANNELS_OUTPUT_SWITCH_STATE_WAIT_NO_POWER_OUTPUT,//等输出电压为0
	CHANNELS_OUTPUT_SWITCH_STATE_START,//重新分配模块,打开已分配好的模块
	CHANNELS_OUTPUT_SWITCH_STATE_ENABLE_PWM,//更新分配好的PWM配置,恢复辅板PWM输出
	CHANNELS_OUTPUT_SWITCH_STATE_WAIT_PWM_READY,//等待所有辅板pwm配置更新完毕,辅板自己根据需求完成预充,并打开相应pwm
} channels_output_switch_state_t;//处理模块输出指令

typedef struct {
	struct list_head list;
	event_pool_t *event_pool;
	channels_output_switch_state_t channels_output_switch_state;
	channel_info_t channel_info[CHANNEL_INSTANCES_NUMBER];
	channels_info_config_t *channels_info_config;
	uint32_t periodic_expire;
} channels_info_t;

void free_channels_info(channels_info_t *channels_info);
channels_info_t *get_or_alloc_channels_info(channels_info_config_t *channels_info_config);
void channels_process_event(channels_info_t *channels_info);
void task_channels(void const *argument);
int send_channel_event(channels_info_t *channels_info, channel_event_t *channel_event, uint32_t timeout);
void task_channel_event(void const *argument);

#endif //_CHANNELS_H
