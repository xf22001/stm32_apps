

/*================================================================
 *   
 *   
 *   文件名称：power_modules_handler_increase.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 17时36分29秒
 *   修改日期：2020年05月16日 星期六 09时45分06秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_increase.h"

power_modules_handler_t power_modules_handler_increase = {
	.power_module_type = POWER_MODULE_TYPE_INCREASE,
	.set_out_voltage_current = NULL,
	.set_power_on_off =  NULL,
	.query_status =  NULL,
	.query_a_line_input_voltage = NULL,
	.query_b_line_input_voltage =  NULL,
	.query_c_line_input_voltage = NULL,
	.power_modules_init = NULL,
	.power_modules_request = NULL,
	.power_modules_decode = NULL,
};
