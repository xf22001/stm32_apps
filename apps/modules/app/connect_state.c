

/*================================================================
 *   
 *   
 *   文件名称：connect_state.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月13日 星期二 16时41分21秒
 *   修改日期：2021年04月13日 星期二 16时58分56秒
 *   描    述：
 *
 *================================================================*/
#include "connect_state.h"

void update_connect_state(connect_state_t *connect_state, uint8_t state)
{
	uint32_t ticks = osKernelSysTick();

	if(state == 1) {
		connect_state->stamp = ticks;
	}

	connect_state->state[connect_state->index++] = state;

	if(connect_state->index >= CONNECT_STATE_SIZE) {
		connect_state->index = 0;
	}
}

uint8_t get_connect_state(connect_state_t *connect_state)
{
	uint8_t count = 0;
	int i;

	for(i = 0; i < CONNECT_STATE_SIZE; i++) {
		if(connect_state->state[i] != 0) {
			count++;
		}
	}

	return count;
}

uint32_t get_connect_stamp(connect_state_t *connect_state)
{
	return connect_state->stamp;
}
