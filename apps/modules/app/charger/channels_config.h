

/*================================================================
 *
 *
 *   文件名称：channels_config.h
 *   创 建 者：肖飞
 *   创建日期：2021年01月18日 星期一 11时00分11秒
 *   修改日期：2021年06月06日 星期日 19时46分10秒
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

#ifdef __cplusplus
}
#endif

typedef enum {
	CHANNEL_TYPE_NONE = 0,
	CHANNEL_TYPE_DC,
	CHANNEL_TYPE_AC,
	CHANNEL_TYPE_PROXY,
} channel_type_t;

typedef enum {
	CHANNEL_CHARGER_TYPE_BMS_NONE = 0,
	CHANNEL_CHARGER_TYPE_BMS_PROXY,
	CHANNEL_CHARGER_TYPE_BMS_GB,
} channel_charger_type_t;

typedef struct {
	channel_charger_type_t channel_charger_type;

	//ac
	TIM_HandleTypeDef *cp_pwm_timer;
	uint32_t cp_pwm_channel;

	GPIO_TypeDef *kl_gpio;
	uint16_t kl_pin;
	GPIO_TypeDef *kn_gpio;
	uint16_t kn_pin;

	//dc
	CAN_HandleTypeDef *hcan_bms;
} channel_charger_config_t;

typedef enum {
	CHANNEL_ENERGY_METER_TYPE_NONE = 0,
	CHANNEL_ENERGY_METER_TYPE_PROXY,
	CHANNEL_ENERGY_METER_TYPE_DC,
	CHANNEL_ENERGY_METER_TYPE_AC,
} channel_energy_meter_type_t;

typedef struct {
	channel_energy_meter_type_t energy_meter_type;
	UART_HandleTypeDef *huart_energy_meter;
} channel_energy_meter_config_t;

typedef struct {
	channel_type_t channel_type;
	channel_charger_config_t charger_config;
	channel_energy_meter_config_t energy_meter_config;
	CAN_HandleTypeDef *hcan_channel_comm;
} channel_config_t;

typedef enum {
	CHANNELS_POWER_MODULE_TYPE_NONE = 0,
	CHANNELS_POWER_MODULE_TYPE_NATIVE,
	CHANNELS_POWER_MODULE_TYPE_PROXY,
} channels_power_module_type_t;

typedef struct {
	uint8_t channels_power_module_number;
	CAN_HandleTypeDef *hcan_power;
	channels_power_module_type_t channels_power_module_type;
} channels_power_module_config_t;

typedef struct {
	GPIO_TypeDef *data_port;
	uint16_t data_pin;
	GPIO_TypeDef *cs_port;
	uint16_t cs_pin;
	GPIO_TypeDef *clk_port;
	uint16_t clk_pin;
} voice_config_t;

typedef enum {
	CARD_READER_TYPE_ZLG = 0,
} card_reader_type_t;

typedef struct {
	card_reader_type_t card_reader_type;
	UART_HandleTypeDef *huart_card_reader;
} card_reader_config_t;

typedef struct {
	uint8_t id;
	uint8_t channel_number;
	channel_config_t **channel_config;
	channels_power_module_config_t power_module_config;
	voice_config_t voice_config;
	card_reader_config_t card_reader_config;
	CAN_HandleTypeDef *hcan_channel_comm;
} channels_config_t;

char *get_channel_config_channel_type(channel_type_t type);
char *get_channel_config_charger_type(channel_charger_type_t type);
char *get_channel_config_energy_meter_type(channel_energy_meter_type_t type);
channels_config_t *get_channels_config(uint8_t id);
#endif //_CHANNELS_CONFIG_H
