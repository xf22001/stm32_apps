

/*================================================================
 *
 *
 *   文件名称：power_modules.h
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 15时37分07秒
 *   修改日期：2020年05月18日 星期一 13时30分50秒
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

#include "channels_config.h"

#ifdef __cplusplus
}
#endif

#define POWER_MODULES_SIZE 20
#define POWER_MODULES_CMD_STATE_SIZE 10
#define CONNECT_STATE_SIZE 10

typedef enum {
	POWER_MODULE_TYPE_UNKNOW = 0,
	POWER_MODULE_TYPE_HUAWEI,
	POWER_MODULE_TYPE_INCREASE,
} power_module_type_t;

typedef struct {
          uint16_t onoff : 1;//1:模块关机 0:模块运行
          uint16_t fault : 1;//1:模块故障 0:模块正常
          uint16_t output_state : 1;//1:模块限流 0:模块恒压
          uint16_t fan_state : 1;//1:风扇故障 0:风扇正常
          uint16_t input_overvoltage : 1;//1:输入过压 0:输入正常
          uint16_t input_lowvoltage : 1;//1:输入欠压 0:输入正常
          uint16_t output_overvoltage : 1;//1:输出过压 0:输出正常
          uint16_t output_lowvoltage : 1;//1:输出欠压 0:输出正常
          uint16_t protect_overcurrent : 1;//1:过流保护 0:正常
          uint16_t protect_overtemperature : 1;//1:过温保护 0:正常
          uint16_t setting_onoff : 1;//1:设置关机 0:设置开机
} power_module_status_t;

typedef enum {
	MODULE_CMD_STATE_IDLE = 0,
	MODULE_CMD_STATE_REQUEST,
	MODULE_CMD_STATE_RESPONSE,
	MODULE_CMD_STATE_ERROR,
} module_cmd_state_t;

typedef struct {
	channel_com_state_t state;
	uint32_t stamp;
	uint32_t retry;
} module_cmd_ctx_t;

typedef struct {
	uint16_t setting_voltage;//模块设置输出电压 0.1v
	uint16_t setting_current;//模块设置输出电流 0.1a
	uint16_t output_voltage;//模块输出电压 0.1v
	uint16_t output_current;//模块输出电流 0.1a

	power_module_status_t power_module_status;//模块状态

	module_cmd_ctx_t module_cmd_ctx[POWER_MODULES_CMD_STATE_SIZE];
	uint8_t connect_state[CONNECT_STATE_SIZE];//连接状态
	uint8_t connect_state_index;//连接状态索引
} power_module_info_t;

typedef struct {
	struct list_head list;
	can_info_t *can_info;
	channels_info_config_t *channels_info_config;
	can_tx_msg_t can_tx_msg;

	power_module_type_t power_module_type;
	void *power_modules_handler;
	uint8_t power_modules_valid;//初始化是否成功
	power_module_info_t power_module_info[POWER_MODULES_SIZE];
} power_modules_info_t;

typedef void (*set_out_voltage_current_t)(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint32_t current);
typedef void (*set_power_on_off_t)(power_modules_info_t *power_modules_info, int module_id, uint8_t onoff);
typedef void (*query_status_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*query_a_line_input_voltage_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*query_b_line_input_voltage_t)(power_modules_info_t *power_modules_info, int module_id);
typedef void (*query_c_line_input_voltage_t)(power_modules_info_t *power_modules_info, int module_id);
typedef int (*power_modules_init_t)(power_modules_info_t *power_modules_info);
typedef int (*power_modules_request_t)(power_modules_info_t *power_modules_info);
typedef int (*power_modules_response_t)(power_modules_info_t *power_modules_info);

typedef struct {
	power_module_type_t power_module_type;
	set_out_voltage_current_t set_out_voltage_current;
	set_power_on_off_t set_power_on_off;
	query_status_t query_status;
	query_a_line_input_voltage_t query_a_line_input_voltage;
	query_b_line_input_voltage_t query_b_line_input_voltage;
	query_c_line_input_voltage_t query_c_line_input_voltage;
	power_modules_init_t power_modules_init;
	power_modules_request_t power_modules_request;
	power_modules_response_t power_modules_response;
} power_modules_handler_t;

void free_power_modules_info(power_modules_info_t *power_modules_info);
power_modules_info_t *get_or_alloc_power_modules_info(channels_info_config_t *channels_info_config);
void power_modules_handler_update(power_modules_info_t *power_modules_info);
void set_out_voltage_current(power_modules_info_t *power_modules_info, int module_id, uint32_t voltage, uint32_t current);
void set_power_on_off(power_modules_info_t *power_modules_info, int module_id, uint8_t onoff);
void query_status(power_modules_info_t *power_modules_info, int module_id);
void query_a_line_input_voltage(power_modules_info_t *power_modules_info, int module_id);
void query_b_line_input_voltage(power_modules_info_t *power_modules_info, int module_id);
void query_c_line_input_voltage(power_modules_info_t *power_modules_info, int module_id);
int power_modules_init(power_modules_info_t *power_modules_info);
int power_modules_request(power_modules_info_t *power_modules_info);
int power_modules_response(power_modules_info_t *power_modules_info);
#endif //_POWER_MODULES_H
