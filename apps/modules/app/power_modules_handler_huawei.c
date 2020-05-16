

/*================================================================
 *
 *
 *   文件名称：power_modules_handler_huawei.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月15日 星期五 17时23分55秒
 *   修改日期：2020年05月16日 星期六 09时45分53秒
 *   描    述：
 *
 *================================================================*/
#include "power_modules_handler_huawei.h"

typedef struct {
	uint32_t id0 : 16;
	uint32_t module_addr : 7;//1-127
	uint32_t id1 : 9;
} s_module_extid_t;

typedef union {
	s_module_extid_t s;
	uint32_t v;
} u_module_extid_t;

typedef struct {
	uint16_t cmd : 12;
	uint16_t unused : 4;
} s_module_cmd_t;

typedef union {
	s_module_cmd_t s;
	uint16_t v;
} u_module_cmd_t;

typedef enum {
	module_cmd_0 = 0,
	module_cmd_1,
	module_cmd_2,
	module_cmd_3,
	module_cmd_4,
	module_cmd_total,
} module_cmd_t;

power_modules_handler_t power_modules_handler_huawei = {
	.power_module_type = POWER_MODULE_TYPE_HUAWEI,
	.set_out_voltage_current = NULL,
	.set_power_on_off =  NULL,
	.query_status =  NULL,
	.query_a_line_input_voltage = NULL,
	.query_b_line_input_voltage =  NULL,
	.query_c_line_input_voltage = NULL,
	.power_modules_init = NULL,//check size
	.power_modules_request = NULL,
	.power_modules_decode = NULL,
};
