

/*================================================================
 *
 *
 *   文件名称：charger_bms_ac.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月19日 星期六 19时12分21秒
 *   修改日期：2021年06月21日 星期一 10时47分29秒
 *   描    述：
 *
 *================================================================*/
#include "charger_bms_ac.h"

#include "channel.h"
#include "charger_bms.h"
#include "hw_adc.h"

#include "log.h"

typedef struct {
	uint8_t state;
	uint32_t state_stamps;
	uint8_t state_0;
	uint32_t state_0_stamps;
} charger_bms_ctx_t;

typedef enum {
	CHARGER_BMS_STATE_IDLE = 0,
	CHARGER_BMS_STATE_STARTING,
	CHARGER_BMS_STATE_CHARGING,
	CHARGER_BMS_STATE_STOPPING,
} charger_bms_state_t;

typedef enum {
	CP_PWM_DUTY_STOP = 1000,//pwm占空比 100%
	CP_PWM_DUTY_16A = 266,//pwm占空比 26.67%
	CP_PWM_DUTY_32A = 533,//pwm占空比 53.33%
	CP_PWM_DUTY_63A = 892,//pwm占空比 89.2%
} cp_pwm_duty_t;

static void start_cp_pwm(charger_info_t *charger_info)
{
	uint16_t duty = CP_PWM_DUTY_STOP;
	channel_info_t *channel_info = charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;

	switch(channel_info->channel_settings.ac_current_limit) {
		case AC_CURRENT_LIMIT_16A: {
			duty = CP_PWM_DUTY_16A;
		}
		break;

		case AC_CURRENT_LIMIT_32A: {
			duty = CP_PWM_DUTY_32A;
		}
		break;

		case AC_CURRENT_LIMIT_63A: {
			duty = CP_PWM_DUTY_63A;
		}
		break;

		default: {
		}
		break;
	}

	__HAL_TIM_SET_COMPARE(channel_config->charger_config.cp_pwm_timer, channel_config->charger_config.cp_pwm_channel, duty);
}

static void stop_cp_pwm(charger_info_t *charger_info)
{
	channel_info_t *channel_info = charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;

	__HAL_TIM_SET_COMPARE(channel_config->charger_config.cp_pwm_timer, channel_config->charger_config.cp_pwm_channel, CP_PWM_DUTY_STOP);
}

static void output_relay_on(charger_info_t *charger_info)
{
	channel_info_t *channel_info = charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;

	HAL_GPIO_WritePin(channel_config->charger_config.kl_gpio, channel_config->charger_config.kl_pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(channel_config->charger_config.kn_gpio, channel_config->charger_config.kn_pin, GPIO_PIN_SET);
}

static void output_relay_off(charger_info_t *charger_info)
{
	channel_info_t *channel_info = charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;

	HAL_GPIO_WritePin(channel_config->charger_config.kl_gpio, channel_config->charger_config.kl_pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(channel_config->charger_config.kn_gpio, channel_config->charger_config.kn_pin, GPIO_PIN_RESET);
}

typedef enum {
	STARTING_STATE_START_PWM = 0,
	STARTING_STATE_START_ADHESION_CHECK_L,
	STARTING_STATE_START_ADHESION_CHECK_N,
	STARTING_STATE_START_OUTPUT,
} starting_state_t;

static void relay_mode_adhesion_ln(charger_info_t *charger_info, uint8_t ln)
{
	channel_info_t *channel_info = charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;

	if(ln == 0) {
		HAL_GPIO_WritePin(channel_config->charger_config.rey3_gpio, channel_config->charger_config.rey3_pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(channel_config->charger_config.rey4_gpio, channel_config->charger_config.rey4_pin, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(channel_config->charger_config.rey3_gpio, channel_config->charger_config.rey3_pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(channel_config->charger_config.rey4_gpio, channel_config->charger_config.rey4_pin, GPIO_PIN_SET);
	}
}

static int ac_adhesion_check_ln(charger_info_t *charger_info, uint8_t ln)
{
	int ret = 1;
	channel_info_t *channel_info = charger_info->channel_info;
	charger_bms_ctx_t *charger_bms_ctx = (charger_bms_ctx_t *)charger_info->charger_bms_ctx;
	uint32_t ticks = osKernelSysTick();

	switch(charger_bms_ctx->state_0) {
		case 0: {
			relay_mode_adhesion_ln(charger_info, ln);
			charger_bms_ctx->state_0 = 1;
		}
		break;

		case 1: {
			if(ticks_duration(ticks, charger_bms_ctx->state_0_stamps) >= 100) {
				charger_bms_ctx->state_0 = 2;
			}
		}
		break;

		case 2: {
			adc_info_t *adc_info = get_or_alloc_adc_info(channel_info->channel_config->adhe_ad_adc);
			uint16_t adhe_ad_voltage;

			OS_ASSERT(adc_info != NULL);
			channel_info->adhe_ad = get_adc_value(adc_info, channel_info->channel_config->adhe_ad_adc_rank);
			adhe_ad_voltage = (int)(channel_info->adhe_ad * 3 * 95 * 10) >> 12;//0.1v

			if(adhe_ad_voltage <= 240) {//todo
				ret = 0;
			} else {
				ret = -1;
			}
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static void relay_mode_output_voltage(charger_info_t *charger_info)
{
	channel_info_t *channel_info = charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;
	HAL_GPIO_WritePin(channel_config->charger_config.rey3_gpio, channel_config->charger_config.rey3_pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(channel_config->charger_config.rey4_gpio, channel_config->charger_config.rey4_pin, GPIO_PIN_SET);
}

static void relay_mode_input_voltage(charger_info_t *charger_info)
{
	channel_info_t *channel_info = charger_info->channel_info;
	channel_config_t *channel_config = channel_info->channel_config;
	HAL_GPIO_WritePin(channel_config->charger_config.rey3_gpio, channel_config->charger_config.rey3_pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(channel_config->charger_config.rey4_gpio, channel_config->charger_config.rey4_pin, GPIO_PIN_RESET);
}

#define CP_AD_VOLTAGE_CONNECT_OFF 110
#define CP_AD_VOLTAGE_CONNECT_ON 80
#define CP_AD_VOLTAGE_READY 50
#define CP_AD_VOLTAGE_READY_3 20

void handle_charger_connect_state(charger_info_t *charger_info)
{
	channel_info_t *channel_info = charger_info->channel_info;
	adc_info_t *adc_info = get_or_alloc_adc_info(channel_info->channel_config->cp_ad_adc);
	uint16_t cp_ad_voltage = 0;
	uint8_t charger_connect_state;
	uint8_t cc1_ready;

	OS_ASSERT(adc_info != NULL);

	channel_info->cp_ad = get_adc_value(adc_info, channel_info->channel_config->cp_ad_adc_rank);

	cp_ad_voltage = (int)(channel_info->cp_ad * 33 * 4.5) >> 12;//0v-14.85v

	if(cp_ad_voltage >= CP_AD_VOLTAGE_CONNECT_OFF) {
		charger_connect_state = 0;
		cc1_ready = 0;
	} else if(cp_ad_voltage >= CP_AD_VOLTAGE_CONNECT_ON) {
		charger_connect_state = 1;
		cc1_ready = 0;
	} else if(cp_ad_voltage >= CP_AD_VOLTAGE_READY) {
		charger_connect_state = 1;
		cc1_ready = 1;
	} else if(cp_ad_voltage >= CP_AD_VOLTAGE_READY_3) {
		charger_connect_state = 1;
		cc1_ready = 1;
	} else {
		charger_connect_state = 0;
		cc1_ready = 0;
	}

	if(charger_info->charger_connect_state != charger_connect_state) {
		charger_info->charger_connect_state = charger_connect_state;
		do_callback_chain(channel_info->charger_connect_changed_chain, channel_info);
	}

	if(charger_info->vehicle_relay_state != cc1_ready) {
		charger_info->vehicle_relay_state = cc1_ready;
	}

	//debug("charger %d charger_connect_state:%d, cc1_ready:%d", channel_info->channel_id, charger_info->charger_connect_state, cc1_ready);
}

static int prepare_bms_state_idle(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_request_bms_state_idle(void *_charger_info)
{
	int ret = 0;

	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;

	if(charger_info->charger_bms_request_action == CHARGER_BMS_REQUEST_ACTION_START) {
		charger_info->charger_bms_request_action = CHARGER_BMS_REQUEST_ACTION_NONE;

		if(charger_info->charger_connect_state == 1) {
			set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_STARTING);
		} else {
			channel_set_stop_reason(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_CHARGER_NOT_CONNECTED);
			set_channel_request_state(channel_info, CHANNEL_STATE_END);
			set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_IDLE);//todo
		}
	}

	return ret;
}

static int prepare_bms_state_starting(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	charger_bms_ctx_t *charger_bms_ctx = (charger_bms_ctx_t *)charger_info->charger_bms_ctx;

	charger_bms_ctx->state = 0;

	return ret;
}

static int handle_request_bms_state_starting(void *_charger_info)
{
	int ret = 0;

	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;
	charger_bms_ctx_t *charger_bms_ctx = (charger_bms_ctx_t *)charger_info->charger_bms_ctx;
	uint32_t ticks = osKernelSysTick();

	switch(charger_bms_ctx->state) {
		case STARTING_STATE_START_PWM: {
			start_cp_pwm(charger_info);
			charger_bms_ctx->state = STARTING_STATE_START_ADHESION_CHECK_L;
			charger_bms_ctx->state_stamps = ticks;
			charger_bms_ctx->state_0 = 0;
			charger_bms_ctx->state_0_stamps = ticks;
		}
		break;

		case STARTING_STATE_START_ADHESION_CHECK_L: {
			if(ticks_duration(ticks, charger_bms_ctx->state_stamps) >= 5 * 1000) {
				channel_set_stop_reason(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_CHECK_L_TIMEOUT);
				set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
			} else {
				int ret = ac_adhesion_check_ln(charger_info, 0);

				if(ret == 0) {
					charger_bms_ctx->state = STARTING_STATE_START_ADHESION_CHECK_N;
					charger_bms_ctx->state_stamps = ticks;
					charger_bms_ctx->state_0 = 0;
					charger_bms_ctx->state_0_stamps = ticks;
				} else if(ret == -1) {
					channel_set_stop_reason(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_L);
					set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
				}
			}
		}
		break;

		case STARTING_STATE_START_ADHESION_CHECK_N: {
			if(ticks_duration(ticks, charger_bms_ctx->state_stamps) >= 5 * 1000) {
				channel_set_stop_reason(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_CHECK_N_TIMEOUT);
				set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
			} else {
				int ret = ac_adhesion_check_ln(charger_info, 1);

				if(ret == 0) {
					charger_bms_ctx->state = STARTING_STATE_START_OUTPUT;
					charger_bms_ctx->state_stamps = ticks;
				} else if(ret == -1) {
					channel_set_stop_reason(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_ADHESION_L);
					set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
				}
			}
		}
		break;

		case STARTING_STATE_START_OUTPUT: {
			if(ticks_duration(ticks, charger_bms_ctx->state_stamps) >= 5 * 1000) {
				channel_set_stop_reason(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_CC1_READY_1_TIMEOUT);
				set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
			} else {
				if(charger_info->vehicle_relay_state == 1) {
					relay_mode_output_voltage(charger_info);
					output_relay_on(charger_info);
					set_channel_request_state(channel_info, CHANNEL_STATE_CHARGING);
				}
			}
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static int prepare_bms_state_charging(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_request_bms_state_charging(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;

	if(charger_info->charger_bms_request_action == CHARGER_BMS_REQUEST_ACTION_STOP) {
		charger_info->charger_bms_request_action = CHARGER_BMS_REQUEST_ACTION_NONE;

		set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
		set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_STOPPING);
	}

	return ret;
}

static int prepare_bms_state_stopping(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	charger_bms_ctx_t *charger_bms_ctx = (charger_bms_ctx_t *)charger_info->charger_bms_ctx;

	charger_bms_ctx->state = 0;

	return ret;
}

static int handle_request_bms_state_stopping(void *_charger_info)
{
	int ret = 0;

	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;
	charger_bms_ctx_t *charger_bms_ctx = (charger_bms_ctx_t *)charger_info->charger_bms_ctx;
	uint32_t ticks = osKernelSysTick();

	switch(charger_bms_ctx->state) {
		case 0: {
			charger_bms_ctx->state = 1;
			charger_bms_ctx->state_stamps = ticks;
		}
		break;

		case 1: {
			if(ticks_duration(ticks, charger_bms_ctx->state_stamps) >= 5 * 1000) {
				channel_set_stop_reason(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_AC_CHARGER_CC1_READY_0_TIMEOUT);
				output_relay_off(charger_info);
				charger_bms_ctx->state = 2;
			} else {
				if(charger_info->vehicle_relay_state == 0) {
					output_relay_off(charger_info);
					relay_mode_input_voltage(charger_info);
					charger_bms_ctx->state = 2;
				}
			}
		}
		break;

		case 2: {
			stop_cp_pwm(charger_info);
			set_channel_request_state(channel_info, CHANNEL_STATE_END);
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static charger_bms_state_handler_t state_handler_sz[] = {
	{
		.bms_state = CHARGER_BMS_STATE_IDLE,
		.prepare = prepare_bms_state_idle,
		.handle_request = handle_request_bms_state_idle,
	},
	{
		.bms_state = CHARGER_BMS_STATE_STARTING,
		.prepare = prepare_bms_state_starting,
		.handle_request = handle_request_bms_state_starting,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CHARGING,
		.prepare = prepare_bms_state_charging,
		.handle_request = handle_request_bms_state_charging,
	},
	{
		.bms_state = CHARGER_BMS_STATE_STOPPING,
		.prepare = prepare_bms_state_stopping,
		.handle_request = handle_request_bms_state_stopping,
	},
};

static charger_bms_state_handler_t *get_charger_bms_state_handler(uint8_t bms_state)
{
	int i;
	charger_bms_state_handler_t *charger_bms_state_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(state_handler_sz); i++) {
		charger_bms_state_handler_t *charger_bms_state_handler_item = &state_handler_sz[i];

		if(charger_bms_state_handler_item->bms_state == bms_state) {
			charger_bms_state_handler = charger_bms_state_handler_item;
		}
	}

	return charger_bms_state_handler;
}

static void update_charger_bms_work_state(charger_info_t *charger_info)
{
	switch(charger_info->state) {
		case CHARGER_BMS_STATE_IDLE: {
			set_charger_bms_work_state(charger_info, CHARGER_BMS_WORK_STATE_IDLE);
		}
		break;

		case CHARGER_BMS_STATE_STARTING: {
			set_charger_bms_work_state(charger_info, CHARGER_BMS_WORK_STATE_STARTING);
		}
		break;

		case CHARGER_BMS_STATE_CHARGING: {
			set_charger_bms_work_state(charger_info, CHARGER_BMS_WORK_STATE_RUNNING);
		}
		break;

		case CHARGER_BMS_STATE_STOPPING: {
			set_charger_bms_work_state(charger_info, CHARGER_BMS_WORK_STATE_STOPPING);
		}
		break;

		default: {
		}
		break;
	}
}

static char *get_charger_bms_state_des(uint8_t state)
{
	char *des = NULL;

	switch(state) {
			add_des_case(CHARGER_BMS_STATE_IDLE);
			add_des_case(CHARGER_BMS_STATE_STARTING);
			add_des_case(CHARGER_BMS_STATE_CHARGING);
			add_des_case(CHARGER_BMS_STATE_STOPPING);

		default: {
			des = "unknow state";
		}
		break;
	}

	return des;
}

static void update_charger_bms_state(charger_info_t *charger_info)
{
	channel_info_t *channel_info = charger_info->channel_info;
	charger_bms_state_handler_t *charger_bms_state_handler = NULL;
	uint8_t request_state = charger_info->request_state;

	if((charger_info->state == request_state) && (charger_info->charger_bms_state_handler != NULL)) {
		return;
	}

	charger_bms_state_handler = get_charger_bms_state_handler(request_state);
	OS_ASSERT(charger_bms_state_handler != NULL);

	debug("charger %d change state: %s -> %s!", channel_info->channel_id, get_charger_bms_state_des(charger_info->state), get_charger_bms_state_des(request_state));

	charger_info->state = request_state;
	update_charger_bms_work_state(charger_info);

	if(charger_bms_state_handler->prepare != NULL) {
		charger_bms_state_handler->prepare(charger_info);
	}

	charger_info->charger_bms_state_handler = charger_bms_state_handler;
}

static int handle_request(charger_info_t *charger_info)
{
	int ret = 0;

	if(charger_info->charger_bms_state_handler == NULL) {
		debug("");
		return ret;
	}

	ret = charger_info->charger_bms_state_handler->handle_request(charger_info);

	return ret;
}

static void charger_handle_request(charger_info_t *charger_info)
{
	int ret;

	mutex_lock(charger_info->handle_mutex);

	ret = handle_request(charger_info);

	if(ret != 0) {
	}

	mutex_unlock(charger_info->handle_mutex);
}

static void charger_bms_periodic(void *_charger_info, void *_channels_info)
{
	charger_info_t *charger_info = (charger_info_t *)_charger_info;

	handle_charger_connect_state(charger_info);
	update_charger_bms_state(charger_info);
	charger_handle_request(charger_info);
}

static int handle_init_ac(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;
	channel_info_t *channel_info = charger_info->channel_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	charger_bms_ctx_t *charger_bms_ctx = os_calloc(1, sizeof(charger_bms_ctx_t));
	OS_ASSERT(charger_bms_ctx != NULL);
	charger_info->charger_bms_ctx = charger_bms_ctx;

	charger_info->charger_bms_state_handler = NULL;
	set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_IDLE);
	update_charger_bms_state(charger_info);

	charger_info->periodic_request_cb.fn = charger_bms_periodic;
	charger_info->periodic_request_cb.fn_ctx = charger_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &charger_info->periodic_request_cb) == 0);

	return ret;
}

charger_bms_handler_t charger_bms_handler_ac = {
	.channel_charger_type = CHANNEL_CHARGER_TYPE_BMS_AC,
	.handle_init = handle_init_ac,
};
