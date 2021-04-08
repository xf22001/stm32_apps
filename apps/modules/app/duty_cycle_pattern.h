

/*================================================================
 *   
 *   
 *   文件名称：duty_cycle_pattern.h
 *   创 建 者：肖飞
 *   创建日期：2021年03月23日 星期二 09时41分37秒
 *   修改日期：2021年04月08日 星期四 10时35分18秒
 *   描    述：
 *
 *================================================================*/
#ifndef _DUTY_CYCLE_PATTERN_H
#define _DUTY_CYCLE_PATTERN_H
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
	PWM_COMPARE_COUNT_UP = 0,
	PWM_COMPARE_COUNT_DOWN,
	PWM_COMPARE_COUNT_KEEP,
} compare_count_type_t;

typedef struct {
	compare_count_type_t type;
	uint16_t duty_cycle;
} pattern_state_t;

void set_work_led_fault_state(uint8_t state);
uint16_t get_duty_cycle_pattern(pattern_state_t *state, uint16_t max, uint16_t min, uint16_t step);

#endif //_DUTY_CYCLE_PATTERN_H
