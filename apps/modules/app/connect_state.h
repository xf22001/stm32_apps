

/*================================================================
 *   
 *   
 *   文件名称：connect_state.h
 *   创 建 者：肖飞
 *   创建日期：2021年04月13日 星期二 16时41分24秒
 *   修改日期：2021年04月13日 星期二 16时58分01秒
 *   描    述：
 *
 *================================================================*/
#ifndef _CONNECT_STATE_H
#define _CONNECT_STATE_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"

#ifdef __cplusplus
}
#endif

#define CONNECT_STATE_SIZE 10

typedef struct {
	uint8_t state[CONNECT_STATE_SIZE];
	uint8_t index;
	uint32_t stamp;
} connect_state_t;

void update_connect_state(connect_state_t *connect_state, uint8_t state);
uint8_t get_connect_state(connect_state_t *connect_state);
uint32_t get_connect_stamp(connect_state_t *connect_state);

#endif //_CONNECT_STATE_H
