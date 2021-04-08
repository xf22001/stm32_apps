

/*================================================================
 *
 *
 *   文件名称：duty_cycle_pattern.c
 *   创 建 者：肖飞
 *   创建日期：2021年03月23日 星期二 09时41分34秒
 *   修改日期：2021年04月08日 星期四 11时06分56秒
 *   描    述：
 *
 *================================================================*/
#include "duty_cycle_pattern.h"

static uint8_t fault_state = 0;

void set_work_led_fault_state(uint8_t state)
{
	fault_state = state;
}

uint16_t get_duty_cycle_pattern(pattern_state_t *state, uint16_t max, uint16_t min, uint16_t step)
{
	if(fault_state != 0) {
		step = step * 8;
	}

	switch(state->type) {
		case PWM_COMPARE_COUNT_UP: {
			if(state->duty_cycle + step <= max) {
				state->duty_cycle += step;

				if(state->duty_cycle >= max) {
					state->type = PWM_COMPARE_COUNT_DOWN;
				}
			} else {
				state->type = PWM_COMPARE_COUNT_DOWN;
			}
		}
		break;

		case PWM_COMPARE_COUNT_DOWN: {
			if(state->duty_cycle >= step + min) {
				state->duty_cycle -= step;

				if(state->duty_cycle <= step + min) {
					state->type = PWM_COMPARE_COUNT_UP;
				}
			} else {
				state->type = PWM_COMPARE_COUNT_UP;
			}
		}
		break;

		default:
			break;
	}

	return state->duty_cycle;
}
