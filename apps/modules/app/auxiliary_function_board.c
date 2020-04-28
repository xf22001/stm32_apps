

/*================================================================
 *
 *
 *   文件名称：auxiliary_function_board.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月28日 星期二 11时34分17秒
 *   修改日期：2020年04月28日 星期二 17时10分36秒
 *   描    述：
 *
 *================================================================*/
#include "auxiliary_function_board.h"

#include "usart_txrx.h"

#include "os_utils.h"
#define UDP_LOG
#include "task_probe_tool.h"

#include <string.h>

extern UART_HandleTypeDef huart1;

static uint8_t tx_buffer[128];
static uint8_t tx_size = 0;
static uint8_t rx_buffer[128];
static uint8_t rx_size = 0;

typedef int (*request_callback_t)(void *ctx);
typedef int (*response_callback_t)(void *ctx);

typedef enum {
	A_F_B_STATE_IDLE = 0,
	A_F_B_STATE_REQUEST,
	A_F_B_STATE_ERROR,
} a_f_b_state_t;

typedef struct {
	a_f_b_state_t state;
	void *ctx;
	uint8_t request_code;
	request_callback_t request_callback;
	uint8_t response_code;
	response_callback_t response_callback;
} a_f_b_command_item_t;

typedef struct {
	uint8_t b0;//0xa5
	uint8_t b1;//0x5a
} a_f_b_magic_t;

typedef struct {
	a_f_b_magic_t magic;
	uint8_t device_id;
	uint8_t cmd;
	uint8_t len;
} a_f_b_head_t;

static uint8_t a_f_b_crc(uint8_t *data, uint8_t len)
{
	int i;
	uint8_t crc = 0;

	for(i = 0; i < len; i++) {
		crc += data[i];
	}

	return crc;
}

typedef struct {
	uint8_t start_discharge : 1;
	uint8_t start_insulation_check : 1;
	uint8_t start_adhesion : 1;
} a_f_b_request_15_data_t;

typedef struct {
	a_f_b_head_t head;
	a_f_b_request_15_data_t data;
	uint8_t crc;
} a_f_b_request_15_t;

typedef struct {
	uint8_t discharge_valid : 1;
	uint8_t insulation_check_valid : 1;
	uint8_t adhesion_valid : 1;
} a_f_b_reponse_95_data_t;

typedef struct {
	a_f_b_head_t head;
	a_f_b_reponse_95_data_t data;
	uint8_t crc;
} a_f_b_response_95_t;

typedef struct {
	a_f_b_request_15_data_t request_data;
	a_f_b_reponse_95_data_t response_data;
} a_f_b_0x15_0x95_ctx_t;

static a_f_b_0x15_0x95_ctx_t a_f_b_0x15_0x95_ctx;
static int request_0x15(void *ctx)
{
	int ret = 0;
	a_f_b_0x15_0x95_ctx_t *a_f_b_0x15_0x95_ctx = (a_f_b_0x15_0x95_ctx_t *)ctx;
	a_f_b_request_15_t *a_f_b_request_15 = (a_f_b_request_15_t *)tx_buffer;

	a_f_b_request_15->head.len = sizeof(a_f_b_request_15_data_t);
	a_f_b_request_15->data = a_f_b_0x15_0x95_ctx->request_data;
	a_f_b_request_15->crc = a_f_b_crc((uint8_t *)a_f_b_request_15, sizeof(a_f_b_request_15_t) - 1);
	tx_size = sizeof(a_f_b_request_15_t);
	rx_size = sizeof(a_f_b_response_95_t);

	return ret;
}

static int response_0x95(void *ctx)
{
	int ret = -1;
	a_f_b_0x15_0x95_ctx_t *a_f_b_0x15_0x95_ctx = (a_f_b_0x15_0x95_ctx_t *)ctx;
	a_f_b_response_95_t *a_f_b_reponse_95 = (a_f_b_response_95_t *)rx_buffer;

	if(a_f_b_reponse_95->crc != a_f_b_crc((uint8_t *)a_f_b_reponse_95, sizeof(a_f_b_response_95_t) - 1)) {
		return ret;
	}

	ret = 0;
	a_f_b_0x15_0x95_ctx->response_data = a_f_b_reponse_95->data;

	return ret;
}

static a_f_b_command_item_t a_f_b_command_item_0x15_0x95 = {
	.state = A_F_B_STATE_IDLE,
	.ctx = &a_f_b_0x15_0x95_ctx,
	.request_code = 0x15,
	.request_callback = request_0x15,
	.response_code = 0x95,
	.response_callback = response_0x95,
};

int request_discharge()
{
	int ret = 0;

	if(a_f_b_command_item_0x15_0x95.state == A_F_B_STATE_REQUEST) {
		return ret;
	}

	memset(&a_f_b_0x15_0x95_ctx.request_data, 0, sizeof(a_f_b_0x15_0x95_ctx.request_data));
	a_f_b_0x15_0x95_ctx.request_data.start_discharge = 1;
	a_f_b_command_item_0x15_0x95.state = A_F_B_STATE_REQUEST;
	ret = 0;

	return ret;
}

int response_discharge(void)
{
	int ret = -1;

	if(a_f_b_command_item_0x15_0x95.state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	if(a_f_b_0x15_0x95_ctx.response_data.discharge_valid == 0) {
		return ret;
	}

	ret = 0;

	return ret;
}

int request_insulation_check()
{
	int ret = 0;

	if(a_f_b_command_item_0x15_0x95.state == A_F_B_STATE_REQUEST) {
		return ret;
	}

	memset(&a_f_b_0x15_0x95_ctx.request_data, 0, sizeof(a_f_b_0x15_0x95_ctx.request_data));
	a_f_b_0x15_0x95_ctx.request_data.start_insulation_check = 1;
	a_f_b_command_item_0x15_0x95.state = A_F_B_STATE_REQUEST;
	ret = 0;

	return ret;
}

int response_insulation_check(void)
{
	int ret = -1;

	if(a_f_b_command_item_0x15_0x95.state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	if(a_f_b_0x15_0x95_ctx.response_data.insulation_check_valid == 0) {
		return ret;
	}

	ret = 0;

	return ret;
}

typedef struct {
	uint8_t data;
} a_f_b_request_11_data_t;

typedef struct {
	a_f_b_head_t head;
	a_f_b_request_11_data_t data;
	uint8_t crc;
} a_f_b_request_11_t;

typedef struct {
	uint8_t b0;
	uint8_t b1;
} a_f_b_reponse_91_version_t;

typedef struct {
	uint8_t discharge_running : 1;
	uint8_t insulation_check_running : 1;
	uint8_t adhesion_check_running : 1;
	uint8_t unused : 2;
	uint8_t discharge_resistor_over_temperature : 1;
	uint8_t adhesion_n : 1;
	uint8_t adhesion_p : 1;
} a_f_b_reponse_91_running_state_t;

#pragma pack(push, 1)
typedef struct {
	a_f_b_reponse_91_version_t version;
	a_f_b_reponse_91_running_state_t running_state;
	uint8_t charger_output_voltage;//4.44v每位
	uint8_t battery_voltage;//4.44v每位
	uint16_t charger_output_voltage_1;//1v每位
	uint8_t insulation_resistor_value;//0.1M欧每位
	uint8_t dc_p_temperature;//-20-220 +20偏移
	uint8_t dc_n_temperature;//-20-220 +20偏移
	uint8_t system_error;//
	uint8_t addr_485;//
} a_f_b_reponse_91_data_t;
#pragma pack(pop)

typedef struct {
	a_f_b_head_t head;
	a_f_b_reponse_91_data_t data;
	uint8_t crc;
} a_f_b_response_91_t;

typedef struct {
	a_f_b_reponse_91_data_t response_data;
} a_f_b_0x11_0x91_ctx_t;

static a_f_b_0x11_0x91_ctx_t a_f_b_0x11_0x91_ctx;

static int request_0x11(void *ctx)
{
	int ret = 0;
	//a_f_b_0x11_0x91_ctx_t *a_f_b_0x11_0x91_ctx = (a_f_b_0x11_0x91_ctx_t *)ctx;
	a_f_b_request_11_t *a_f_b_request_11 = (a_f_b_request_11_t *)tx_buffer;

	a_f_b_request_11->data.data = 0x00;
	a_f_b_request_11->crc = a_f_b_crc((uint8_t *)a_f_b_request_11, sizeof(a_f_b_request_11_t) - 1);
	tx_size = sizeof(a_f_b_request_11_t);
	rx_size = sizeof(a_f_b_response_91_t);

	return ret;
}

static int response_0x91(void *ctx)
{
	int ret = -1;
	a_f_b_0x11_0x91_ctx_t *a_f_b_0x11_0x91_ctx = (a_f_b_0x11_0x91_ctx_t *)ctx;
	a_f_b_response_91_t *a_f_b_response_91 = (a_f_b_response_91_t *)rx_buffer;

	if(a_f_b_response_91->crc != a_f_b_crc((uint8_t *)a_f_b_response_91, sizeof(a_f_b_response_91_t) - 1)) {
		return ret;
	}

	ret = 0;
	a_f_b_0x11_0x91_ctx->response_data = a_f_b_response_91->data;

	return ret;
}

static a_f_b_command_item_t a_f_b_command_item_0x11_0x91 = {
	.state = A_F_B_STATE_IDLE,
	.ctx = &a_f_b_0x11_0x91_ctx,
	.request_code = 0x11,
	.request_callback = request_0x11,
	.response_code = 0x91,
	.response_callback = response_0x91,
};

int request_a_f_b_status_data(void)
{
	int ret = 0;

	if(a_f_b_command_item_0x11_0x91.state == A_F_B_STATE_REQUEST) {
		return ret;
	}

	a_f_b_command_item_0x11_0x91.state = A_F_B_STATE_REQUEST;
	ret = 0;

	return ret;
}

int response_discharge_running_status(void)
{
	int ret = -1;

	if(a_f_b_command_item_0x15_0x95.state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	if(a_f_b_0x11_0x91_ctx.response_data.running_state.discharge_running == 1) {
		return ret;
	}

	ret = 0;

	return ret;
}

int response_battery_voltage(void)
{
	int ret = -1;

	if(a_f_b_command_item_0x15_0x95.state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	ret = a_f_b_0x11_0x91_ctx.response_data.battery_voltage * 4.44;

	return ret;
}

int response_insulation_check_running_status(void)
{
	int ret = -1;

	if(a_f_b_command_item_0x15_0x95.state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	if(a_f_b_0x11_0x91_ctx.response_data.running_state.insulation_check_running == 1) {
		return ret;
	}

	ret = a_f_b_0x11_0x91_ctx.response_data.insulation_resistor_value;

	return ret;
}

static a_f_b_command_item_t *a_f_b_command_table[] = {
	&a_f_b_command_item_0x15_0x95,
	&a_f_b_command_item_0x11_0x91,
};

void test_a_f_b(void)
{
	udp_log_printf("sizeof(a_f_b_response_91_t):%d\n", sizeof(a_f_b_response_91_t));
}

void task_auxiliary_function_board_decode(void const *argument)
{
	int ret = 0;
	int i;

	uart_info_t *uart_info = get_or_alloc_uart_info(&huart1);

	if(uart_info == NULL) {
		app_panic();
	}

	while(1) {
		for(i = 0; i < sizeof(a_f_b_command_table) / sizeof(a_f_b_command_item_t *); i++) {
			a_f_b_command_item_t *item = a_f_b_command_table[i];
			a_f_b_head_t *tx_head = (a_f_b_head_t *)tx_buffer;
			a_f_b_head_t *rx_head = (a_f_b_head_t *)rx_buffer;


			if(item->state == A_F_B_STATE_IDLE) {
				continue;
			}

			tx_head->magic.b0 = 0xa5;
			tx_head->magic.b1 = 0x5a;
			tx_head->device_id = 0x00;
			tx_head->cmd = item->request_code;

			memset(rx_buffer, 0, 128);

			ret = item->request_callback(item->ctx);
			ret = uart_tx_rx_data(uart_info, rx_buffer, tx_size, rx_buffer, rx_size, 1000);

			if(ret != rx_size) {
				item->state = A_F_B_STATE_ERROR;
				continue;
			}

			if(rx_head->cmd != item->response_code) {
				item->state = A_F_B_STATE_ERROR;
				continue;
			}

			ret = item->response_callback(item->ctx);

			if(ret == 0) {
				item->state = A_F_B_STATE_IDLE;
			}
		}
	}
}
