

/*================================================================
 *
 *
 *   文件名称：channel_handler_ac.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月11日 星期二 09时20分53秒
 *   修改日期：2021年06月02日 星期三 16时29分33秒
 *   描    述：
 *
 *================================================================*/
#include "channel_handler_ac.h"

#include "log.h"

typedef struct {
	callback_item_t handler_periodic_callback_item;
	callback_item_t handler_event_callback_item;

	callback_item_t idle_callback_item;
	callback_item_t start_callback_item;
	callback_item_t starting_callback_item;
	callback_item_t charging_callback_item;
	callback_item_t stopping_callback_item;
	callback_item_t stop_callback_item;
	callback_item_t state_changed_callback_item;

	uint8_t state;
	uint32_t state_stamps;

	uint8_t cc1_ready;
} channel_handler_ctx_t;

typedef enum {
	CP_PWM_DUTY_STOP = 1000,//pwm占空比 100%
	CP_PWM_DUTY_16A = 266,//pwm占空比 26.67%
	CP_PWM_DUTY_32A = 533,//pwm占空比 53.33%
	CP_PWM_DUTY_63A = 892,//pwm占空比 89.2%
} cp_pwm_duty_t;

static void idle(void *_channel_info, void *__channel_info)
{
}

static void start(void *_channel_info, void *__channel_info)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	if(channel_info->charger_connect_state == 1) {
		set_channel_request_state(channel_info, CHANNEL_STATE_STARTING);
	} else {
		set_fault(channel_info->faults, CHANNEL_FAULT_AC_CHARGER_CONNECT_STATE_OFF);
		set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
	}
}

static void start_cp_pwm(channel_info_t *channel_info)
{
	uint16_t duty = CP_PWM_DUTY_STOP;
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

static void stop_cp_pwm(channel_info_t *channel_info)
{
	channel_config_t *channel_config = channel_info->channel_config;

	__HAL_TIM_SET_COMPARE(channel_config->charger_config.cp_pwm_timer, channel_config->charger_config.cp_pwm_channel, CP_PWM_DUTY_STOP);
}

static void output_relay_on(channel_info_t *channel_info)
{
	channel_config_t *channel_config = channel_info->channel_config;

	HAL_GPIO_WritePin(channel_config->charger_config.kl_gpio, channel_config->charger_config.kl_pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(channel_config->charger_config.kn_gpio, channel_config->charger_config.kn_pin, GPIO_PIN_SET);
}

static void output_relay_off(channel_info_t *channel_info)
{
	channel_config_t *channel_config = channel_info->channel_config;

	HAL_GPIO_WritePin(channel_config->charger_config.kl_gpio, channel_config->charger_config.kl_pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(channel_config->charger_config.kn_gpio, channel_config->charger_config.kn_pin, GPIO_PIN_RESET);
}

static void starting(void *_channel_info, void *__channel_info)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channel_handler_ctx_t *channel_handler_ctx = (channel_handler_ctx_t *)channel_info->channel_handler_ctx;
	uint32_t ticks = osKernelSysTick();

	switch(channel_handler_ctx->state) {
		case 0: {
			start_cp_pwm(channel_info);
			channel_handler_ctx->state = 1;
			channel_handler_ctx->state_stamps = ticks;
		}
		break;

		case 1: {
			if(ticks_duration(ticks, channel_handler_ctx->state_stamps) >= 5 * 1000) {
				set_fault(channel_info->faults, CHANNEL_FAULT_AC_CHARGER_CC1_READY_1_TIMEOUT);
				set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
			} else {
				if(channel_handler_ctx->cc1_ready == 1) {
					output_relay_on(channel_info);
					set_channel_request_state(channel_info, CHANNEL_STATE_CHARGING);
				}
			}
		}
		break;

		default: {
		}
		break;
	}
}

static void charging(void *_channel_info, void *__channel_info)
{
}

static void stopping(void *_channel_info, void *__channel_info)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channel_handler_ctx_t *channel_handler_ctx = (channel_handler_ctx_t *)channel_info->channel_handler_ctx;
	uint32_t ticks = osKernelSysTick();

	switch(channel_handler_ctx->state) {
		case 0: {
			channel_handler_ctx->state = 1;
			channel_handler_ctx->state_stamps = ticks;
		}
		break;

		case 1: {
			if(ticks_duration(ticks, channel_handler_ctx->state_stamps) >= 5 * 1000) {
				set_fault(channel_info->faults, CHANNEL_FAULT_AC_CHARGER_CC1_READY_0_TIMEOUT);
				output_relay_off(channel_info);
				channel_handler_ctx->state = 2;
			} else {
				if(channel_handler_ctx->cc1_ready == 0) {
					output_relay_off(channel_info);
					channel_handler_ctx->state = 2;
				}
			}
		}
		break;

		case 2: {
			stop_cp_pwm(channel_info);
			set_channel_request_state(channel_info, CHANNEL_STATE_STOP);
		}
		break;

		default: {
		}
		break;
	}
}

static void stop(void *_channel_info, void *__channel_info)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	set_channel_request_state(channel_info, CHANNEL_STATE_IDLE);
}

static void state_changed(void *_channel_info, void *_pre_state)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channel_handler_ctx_t *channel_handler_ctx = (channel_handler_ctx_t *)channel_info->channel_handler_ctx;

	channel_handler_ctx->state = 0;
}

static void handle_channel_handler_periodic(void *_channel_info, void *_channels_info)
{
	//debug("channel_id %d handler periodic!", ((channel_info_t *)_channel_info)->channel_id);
}

static int _handle_channel_handler_event(channel_info_t *channel_info, channel_event_t *channel_event)
{
	int ret = -1;

	debug("channel_id %d process handler event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

	switch(channel_event->type) {
		default: {
		}
		break;
	}

	return ret;
}

static void handle_channel_handler_event(void *_channel_info, void *_channels_event)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;
	channel_event_t *channel_event;
	uint8_t match = 0;

	if(channels_event->type != CHANNELS_EVENT_CHANNEL_EVENT) {
		return;
	}

	channel_event = &channels_event->event.channel_event;

	if(channel_event->channel_id == 0xff) { //broadcast
		match = 1;
	}

	if(channel_event->channel_id == channel_info->channel_id) {
		match = 1;
	}

	if(match == 0) {
		return;
	}

	_handle_channel_handler_event(channel_info, channel_event);
}

static int init(void *_channel_info)
{
	int ret = 0;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	channel_handler_ctx_t *channel_handler_ctx = os_calloc(1, sizeof(channel_handler_ctx_t));

	OS_ASSERT(channel_handler_ctx != NULL);

	channel_handler_ctx->idle_callback_item.fn = idle;
	channel_handler_ctx->idle_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->idle_chain, &channel_handler_ctx->idle_callback_item) == 0);

	channel_handler_ctx->start_callback_item.fn = start;
	channel_handler_ctx->start_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->start_chain, &channel_handler_ctx->start_callback_item) == 0);

	channel_handler_ctx->starting_callback_item.fn = starting;
	channel_handler_ctx->starting_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->starting_chain, &channel_handler_ctx->starting_callback_item) == 0);

	channel_handler_ctx->charging_callback_item.fn = charging;
	channel_handler_ctx->charging_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->charging_chain, &channel_handler_ctx->charging_callback_item) == 0);

	channel_handler_ctx->stopping_callback_item.fn = stopping;
	channel_handler_ctx->stopping_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->stopping_chain, &channel_handler_ctx->stopping_callback_item) == 0);

	channel_handler_ctx->stop_callback_item.fn = stop;
	channel_handler_ctx->stop_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->stop_chain, &channel_handler_ctx->stop_callback_item) == 0);

	channel_handler_ctx->state_changed_callback_item.fn = state_changed;
	channel_handler_ctx->state_changed_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->state_changed_chain, &channel_handler_ctx->state_changed_callback_item) == 0);

	channel_handler_ctx->handler_periodic_callback_item.fn = handle_channel_handler_periodic;
	channel_handler_ctx->handler_periodic_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channel_handler_ctx->handler_periodic_callback_item) == 0);

	channel_handler_ctx->handler_event_callback_item.fn = handle_channel_handler_event;
	channel_handler_ctx->handler_event_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channel_handler_ctx->handler_event_callback_item) == 0);

	channel_info->channel_handler_ctx = channel_handler_ctx;

	return ret;
}

static int channel_start(void *_channel_info)
{
	int ret = -1;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	switch(channel_info->state) {
		case CHANNEL_STATE_IDLE: {
			set_channel_request_state(channel_info, CHANNEL_STATE_START);
			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static int channel_stop(void *_channel_info)
{
	int ret = -1;
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	switch(channel_info->state) {
		case CHANNEL_STATE_START:
		case CHANNEL_STATE_STARTING:
		case CHANNEL_STATE_CHARGING: {
			set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

channel_handler_t channel_handler_ac = {
	.channel_type = CHANNEL_TYPE_AC,
	.init = init,
	.channel_start = channel_start,
	.channel_stop = channel_stop,
};