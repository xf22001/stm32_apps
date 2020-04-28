

/*================================================================
 *   
 *   
 *   文件名称：auxiliary_function_board.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月28日 星期二 11时34分23秒
 *   修改日期：2020年04月28日 星期二 17时17分52秒
 *   描    述：
 *
 *================================================================*/
#ifndef _AUXILIARY_FUNCTION_BOARD_H
#define _AUXILIARY_FUNCTION_BOARD_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif
int request_discharge();
int response_discharge(void);
int request_insulation_check();
int response_insulation_check(void);
int request_a_f_b_status_data(void);
int response_discharge_running_status(void);
int response_battery_voltage(void);
int response_insulation_check_running_status(void);
#endif //_AUXILIARY_FUNCTION_BOARD_H
