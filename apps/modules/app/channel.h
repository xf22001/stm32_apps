

/*================================================================
 *   
 *   
 *   文件名称：channel.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月30日 星期四 08时56分47秒
 *   修改日期：2020年04月30日 星期四 16时12分20秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CHANNEL_H
#define _CHANNEL_H
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

typedef struct {
	struct list_head list;

	uint8_t channel_id;
	channel_info_config_t *channel_info_config;

	uint8_t gun_connect_state;//插枪状态
	uint8_t gun_connect_state_debounce_count;//插枪状态防抖动值

	uint8_t charger_sn;//充电机编号
	uint8_t gb;//标准
	uint8_t test_mode;//测试模式
	uint8_t precharge_enable;//允许预充
	uint8_t fault;//充电机故障
	uint8_t charger_power_on;//充电机开机状态
	uint8_t manual;//手动模式
	uint8_t adhesion_test;//粘连检测
	uint8_t double_gun_one_car;//双枪充一车
	uint8_t cp_ad;//cp-ad采样
	uint8_t auxiliary_power_type;//12-24v选择
	uint16_t module_output_voltage;//模块充电电压
	uint16_t charnnel_max_output_power;//通道最大输出功率
	uint16_t module_output_current;//模块充电电流
} channel_info_t;

channel_info_t *get_or_alloc_channel_info(uint8_t channel_id);
int channel_set_channel_config(channel_info_t *channel_info, channel_info_config_t *channel_info_config);
#endif //_CHANNEL_H
