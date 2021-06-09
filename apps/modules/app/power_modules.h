

/*================================================================
 *
 *
 *   文件名称：power_modules.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 15时37分07秒
 *   修改日期：2021年06月09日 星期三 15时13分33秒
 *   描    述：
 *
 *================================================================*/
#ifndef _POWER_MODULES_H
#define _POWER_MODULES_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "cmsis_os.h"
#include "app_platform.h"
#include "list_utils.h"

#include "can_txrx.h"
#include "can_command.h"
#include "callback_chain.h"
#include "connect_state.h"

#ifdef __cplusplus
}
#endif

#define CONNECT_STATE_SIZE 10
#define LINE_TO_PHASE_COEFFICIENT 0.5773502691896258f

typedef enum {
	POWER_MODULE_TYPE_UNKNOW = 0,
	POWER_MODULE_TYPE_PSEUDO,
	POWER_MODULE_TYPE_HUAWEI,
	POWER_MODULE_TYPE_INCREASE,
	POWER_MODULE_TYPE_INFY,
	POWER_MODULE_TYPE_STATEGRID,
	POWER_MODULE_TYPE_YYLN,
	POWER_MODULE_TYPE_WINLINE,
	POWER_MODULE_TYPE_ZTE,
} power_module_type_t;

typedef struct {
          uint16_t poweroff : 1;//1:模块关机 0:模块运行
          uint16_t fault : 1;//1:模块故障 0:模块正常
          uint16_t output_state : 1;//1:模块限流 0:模块恒压
          uint16_t fan_state : 1;//1:风扇故障 0:风扇正常
          uint16_t input_overvoltage : 1;//1:输入过压 0:输入正常
          uint16_t input_lowvoltage : 1;//1:输入欠压 0:输入正常
          uint16_t output_overvoltage : 1;//1:输出过压 0:输出正常
          uint16_t output_lowvoltage : 1;//1:输出欠压 0:输出正常
          uint16_t protect_overcurrent : 1;//1:过流保护 0:正常
          uint16_t protect_overtemperature : 1;//1:过温保护 0:正常
          uint16_t setting_poweroff : 1;//1:设置关机 0:设置开机
} power_module_status_t;

typedef struct {
	uint8_t channel_id;
	uint32_t battery_voltage;//电池电压 mv

	uint32_t setting_voltage;//模块设置输出电压 mv
	uint32_t setting_current;//模块设置输出电流 ma

	uint16_t output_voltage;//模块输出电压 0.1v
	uint16_t output_current;//模块输出电流 0.1a

	uint8_t poweroff;//输入
	uint8_t automode;//输入,华为
	uint32_t input_aline_voltage;//输出 0.1v
	uint32_t input_bline_voltage;//输出 0.1v
	uint32_t input_cline_voltage;//输出 0.1v

	power_module_status_t power_module_status;//模块状态

	command_status_t *cmd_ctx;//os_alloc
	connect_state_t connect_state;
} power_module_info_t;

typedef struct {
	can_info_t *can_info;
	void *channels_config;
	can_tx_msg_t can_tx_msg;
	can_rx_msg_t *can_rx_msg;

	uint16_t rate_current;//华为模块参考电流 a
	uint32_t periodic_stamp;

	power_module_type_t power_module_type;
	void *power_modules_handler;
	uint8_t power_module_number;
	power_module_info_t *power_module_info;//os_alloc
	callback_item_t can_data_request_cb;
	callback_item_t can_data_response_cb;
} power_modules_info_t;

typedef void (*power_modules_init_t)(power_modules_info_t *power_modules_info);
typedef void (*set_out_voltage_current_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*set_poweroff_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*query_status_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*query_a_line_input_voltage_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*query_b_line_input_voltage_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*query_c_line_input_voltage_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*power_modules_request_t)(power_modules_info_t *power_modules_info);
typedef int (*power_modules_response_t)(power_modules_info_t *power_modules_info, can_rx_msg_t *can_rx_msg);

typedef struct {
	power_module_type_t power_module_type;
	uint8_t cmd_size;
	power_modules_init_t init;
	set_out_voltage_current_t set_out_voltage_current;
	set_poweroff_t set_poweroff;
	query_status_t query_status;
	query_a_line_input_voltage_t query_a_line_input_voltage;
	query_b_line_input_voltage_t query_b_line_input_voltage;
	query_c_line_input_voltage_t query_c_line_input_voltage;
	power_modules_request_t power_modules_request;
	power_modules_response_t power_modules_response;
} power_modules_handler_t;

int power_modules_set_type(power_modules_info_t *power_modules_info, power_module_type_t power_module_type);
void power_modules_set_channel_id(power_modules_info_t *power_modules_info, int module_id, uint8_t channel_id);
void power_modules_set_battery_voltage(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage);
void power_modules_init(power_modules_info_t *power_modules_info);
void power_modules_set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint32_t current);
void power_modules_set_poweroff(power_modules_info_t *power_modules_info, int module_id, uint8_t poweroff);
void power_modules_query_status(power_modules_info_t *power_modules_info, int module_id);
void power_modules_query_a_line_input_voltage(power_modules_info_t *power_modules_info, int module_id);
void power_modules_query_b_line_input_voltage(power_modules_info_t *power_modules_info, int module_id);
void power_modules_query_c_line_input_voltage(power_modules_info_t *power_modules_info, int module_id);
uint8_t get_power_module_connect_state(power_module_info_t *power_module_info);
uint32_t get_power_module_connect_stamp(power_module_info_t *power_module_info);
power_modules_info_t *get_or_alloc_power_modules_info(void *ctx);
#endif //_POWER_MODULES_H
