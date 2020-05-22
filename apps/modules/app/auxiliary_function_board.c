

/*================================================================
 *
 *
 *   文件名称：auxiliary_function_board.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月28日 星期二 11时34分17秒
 *   修改日期：2020年05月22日 星期五 09时43分01秒
 *   描    述：
 *
 *================================================================*/
#include "auxiliary_function_board.h"

#define LOG_NONE
#include "log.h"

#include <string.h>

static LIST_HEAD(a_f_b_info_list);
static osMutexId a_f_b_info_list_mutex = NULL;

static a_f_b_info_t *get_a_f_b_info(channel_info_config_t *channel_info_config)
{
	a_f_b_info_t *a_f_b_info = NULL;
	a_f_b_info_t *a_f_b_info_item = NULL;
	osStatus os_status;

	if(a_f_b_info_list_mutex == NULL) {
		return a_f_b_info;
	}

	os_status = osMutexWait(a_f_b_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(a_f_b_info_item, &a_f_b_info_list, a_f_b_info_t, list) {
		if(a_f_b_info_item->uart_info->huart == channel_info_config->huart_a_f_b) {
			a_f_b_info = a_f_b_info_item;
			break;
		}
	}

	os_status = osMutexRelease(a_f_b_info_list_mutex);

	if(os_status != osOK) {
	}

	return a_f_b_info;
}

void free_a_f_b_info(a_f_b_info_t *a_f_b_info)
{
	osStatus os_status;

	if(a_f_b_info == NULL) {
		return;
	}

	if(a_f_b_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(a_f_b_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&a_f_b_info->list);

	os_status = osMutexRelease(a_f_b_info_list_mutex);

	if(os_status != osOK) {
	}

	os_free(a_f_b_info);
}

static int a_f_b_info_set_channel_config(a_f_b_info_t *a_f_b_info, channel_info_config_t *channel_info_config)
{
	int ret = -1;
	uart_info_t *uart_info;

	uart_info = get_or_alloc_uart_info(channel_info_config->huart_a_f_b);

	if(uart_info == NULL) {
		return ret;
	}

	a_f_b_info->uart_info = uart_info;

	ret = 0;
	return ret;
}

a_f_b_info_t *get_or_alloc_a_f_b_info(channel_info_config_t *channel_info_config)
{
	a_f_b_info_t *a_f_b_info = NULL;
	osStatus os_status;
	int i;

	a_f_b_info = get_a_f_b_info(channel_info_config);

	if(a_f_b_info != NULL) {
		return a_f_b_info;
	}

	if(a_f_b_info_list_mutex == NULL) {
		osMutexDef(a_f_b_info_list_mutex);
		a_f_b_info_list_mutex = osMutexCreate(osMutex(a_f_b_info_list_mutex));

		if(a_f_b_info_list_mutex == NULL) {
			return a_f_b_info;
		}
	}

	a_f_b_info = (a_f_b_info_t *)os_alloc(sizeof(a_f_b_info_t));

	if(a_f_b_info == NULL) {
		return a_f_b_info;
	}

	memset(a_f_b_info, 0, sizeof(a_f_b_info_t));

	a_f_b_info->channel_info_config = channel_info_config;

	for(i = 0; i < A_F_B_CMD_TOTAL; i++) {
		a_f_b_info->cmd_ctx[i].state = A_F_B_STATE_IDLE;
	}

	memset(a_f_b_info->connect_state, 0, A_F_B_CONNECT_STATE_SIZE);

	a_f_b_info->connect_state_index = 0;

	os_status = osMutexWait(a_f_b_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&a_f_b_info->list, &a_f_b_info_list);

	os_status = osMutexRelease(a_f_b_info_list_mutex);

	if(os_status != osOK) {
	}

	if(a_f_b_info_set_channel_config(a_f_b_info, channel_info_config) != 0) {
		goto failed;
	}

	return a_f_b_info;
failed:
	free_a_f_b_info(a_f_b_info);
	a_f_b_info = NULL;

	return a_f_b_info;
}


static uint8_t a_f_b_crc(uint8_t *data, uint8_t len)
{
	int i;
	uint8_t crc = 0;

	for(i = 0; i < len; i++) {
		crc += data[i];
	}

	return crc;
}

uint8_t get_a_f_b_connect_state(a_f_b_info_t *a_f_b_info)
{
	uint8_t ret = 0;
	uint8_t count = 0;
	int i;

	for(i = 0; i < A_F_B_CONNECT_STATE_SIZE; i++) {
		if(a_f_b_info->connect_state[i] != 0) {
			count++;
		}
	}

	if(count >= 8) {
		ret = 0;
	}

	return ret;
}

static int request_0x15(a_f_b_info_t *a_f_b_info)
{
	int ret = 0;
	a_f_b_0x15_0x95_ctx_t *a_f_b_0x15_0x95_ctx = (a_f_b_0x15_0x95_ctx_t *)&a_f_b_info->a_f_b_0x15_0x95_ctx;
	a_f_b_request_15_t *a_f_b_request_15 = (a_f_b_request_15_t *)a_f_b_info->tx_buffer;

	a_f_b_request_15->head.len = sizeof(a_f_b_request_15_data_t);
	a_f_b_request_15->data = a_f_b_0x15_0x95_ctx->request_data;
	a_f_b_request_15->crc = a_f_b_crc((uint8_t *)a_f_b_request_15, sizeof(a_f_b_request_15_t) - 1);
	a_f_b_info->tx_size = sizeof(a_f_b_request_15_t);
	a_f_b_info->rx_size = sizeof(a_f_b_response_95_t);

	return ret;
}

static int response_0x95(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;
	a_f_b_0x15_0x95_ctx_t *a_f_b_0x15_0x95_ctx = (a_f_b_0x15_0x95_ctx_t *)&a_f_b_info->a_f_b_0x15_0x95_ctx;
	a_f_b_response_95_t *a_f_b_reponse_95 = (a_f_b_response_95_t *)a_f_b_info->rx_buffer;

	if(a_f_b_reponse_95->crc != a_f_b_crc((uint8_t *)a_f_b_reponse_95, sizeof(a_f_b_response_95_t) - 1)) {
		return ret;
	}

	ret = 0;
	a_f_b_0x15_0x95_ctx->response_data = a_f_b_reponse_95->data;

	return ret;
}

static a_f_b_command_item_t a_f_b_command_item_0x15_0x95 = {
	.cmd = A_F_B_CMD_0X15_0X95,
	.request_code = 0x15,
	.request_callback = request_0x15,
	.response_code = 0x95,
	.response_callback = response_0x95,
};

int request_discharge(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;

	if(a_f_b_info->cmd_ctx[A_F_B_CMD_0X15_0X95].state == A_F_B_STATE_REQUEST) {
		return ret;
	}

	memset(&a_f_b_info->a_f_b_0x15_0x95_ctx.request_data, 0, sizeof(a_f_b_info->a_f_b_0x15_0x95_ctx.request_data));
	a_f_b_info->a_f_b_0x15_0x95_ctx.request_data.start_discharge = 1;
	a_f_b_info->cmd_ctx[A_F_B_CMD_0X15_0X95].state = A_F_B_STATE_REQUEST;
	ret = 0;

	return ret;
}

int response_discharge(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;

	if(a_f_b_info->cmd_ctx[A_F_B_CMD_0X15_0X95].state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	if(a_f_b_info->a_f_b_0x15_0x95_ctx.response_data.discharge_valid == 0) {
		return ret;
	}

	ret = 0;

	return ret;
}

int request_insulation_check(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;

	if(a_f_b_info->cmd_ctx[A_F_B_CMD_0X15_0X95].state == A_F_B_STATE_REQUEST) {
		return ret;
	}

	memset(&a_f_b_info->a_f_b_0x15_0x95_ctx.request_data, 0, sizeof(a_f_b_info->a_f_b_0x15_0x95_ctx.request_data));
	a_f_b_info->a_f_b_0x15_0x95_ctx.request_data.start_insulation_check = 1;
	a_f_b_info->cmd_ctx[A_F_B_CMD_0X15_0X95].state = A_F_B_STATE_REQUEST;
	ret = 0;

	return ret;
}

int response_insulation_check(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;

	if(a_f_b_info->cmd_ctx[A_F_B_CMD_0X15_0X95].state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	if(a_f_b_info->a_f_b_0x15_0x95_ctx.response_data.insulation_check_valid == 0) {
		return ret;
	}

	ret = 0;

	return ret;
}

static int request_0x11(a_f_b_info_t *a_f_b_info)
{
	int ret = 0;
	//a_f_b_0x11_0x91_ctx_t *a_f_b_0x11_0x91_ctx = (a_f_b_0x11_0x91_ctx_t *)&a_f_b_info->a_f_b_0x11_0x91_ctx;
	a_f_b_request_11_t *a_f_b_request_11 = (a_f_b_request_11_t *)a_f_b_info->tx_buffer;

	a_f_b_request_11->data.data = 0x00;
	a_f_b_request_11->crc = a_f_b_crc((uint8_t *)a_f_b_request_11, sizeof(a_f_b_request_11_t) - 1);
	a_f_b_info->tx_size = sizeof(a_f_b_request_11_t);
	a_f_b_info->rx_size = sizeof(a_f_b_response_91_t);

	return ret;
}

static int response_0x91(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;
	a_f_b_0x11_0x91_ctx_t *a_f_b_0x11_0x91_ctx = (a_f_b_0x11_0x91_ctx_t *)&a_f_b_info->a_f_b_0x11_0x91_ctx;
	a_f_b_response_91_t *a_f_b_response_91 = (a_f_b_response_91_t *)a_f_b_info->rx_buffer;

	if(a_f_b_response_91->crc != a_f_b_crc((uint8_t *)a_f_b_response_91, sizeof(a_f_b_response_91_t) - 1)) {
		return ret;
	}

	ret = 0;
	a_f_b_0x11_0x91_ctx->response_data = a_f_b_response_91->data;

	return ret;
}

static a_f_b_command_item_t a_f_b_command_item_0x11_0x91 = {
	.cmd = A_F_B_CMD_0X11_0X91,
	.request_code = 0x11,
	.request_callback = request_0x11,
	.response_code = 0x91,
	.response_callback = response_0x91,
};

int request_a_f_b_status_data(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;

	if(a_f_b_info->cmd_ctx[A_F_B_CMD_0X11_0X91].state == A_F_B_STATE_REQUEST) {
		return ret;
	}

	a_f_b_info->cmd_ctx[A_F_B_CMD_0X11_0X91].state = A_F_B_STATE_REQUEST;
	ret = 0;

	return ret;
}

int response_discharge_running_status(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;

	if(a_f_b_info->cmd_ctx[A_F_B_CMD_0X11_0X91].state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	if(a_f_b_info->a_f_b_0x11_0x91_ctx.response_data.running_state.discharge_running == 1) {//checking
		return ret;
	}

	ret = 0;

	return ret;
}

int response_insulation_check_running_status(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;

	if(a_f_b_info->cmd_ctx[A_F_B_CMD_0X11_0X91].state != A_F_B_STATE_IDLE) {//busy
		return ret;
	}

	if(a_f_b_info->a_f_b_0x11_0x91_ctx.response_data.running_state.insulation_check_running == 1) {//checking
		return ret;
	}

	ret = a_f_b_info->a_f_b_0x11_0x91_ctx.response_data.insulation_resistor_value;

	return ret;
}

a_f_b_reponse_91_data_t *get_a_f_b_status_data(a_f_b_info_t *a_f_b_info)
{
	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = NULL;

	if(get_a_f_b_connect_state(a_f_b_info) >= A_F_B_CONNECT_STATE_OK_SIZE) {
		a_f_b_reponse_91_data = &a_f_b_info->a_f_b_0x11_0x91_ctx.response_data;
	}

	return a_f_b_reponse_91_data;
}

uint8_t get_battery_available_state(a_f_b_info_t *a_f_b_info)
{
	uint8_t state = 0;
	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(a_f_b_info);
	int voltage = (a_f_b_reponse_91_data != NULL) ? a_f_b_info->a_f_b_0x11_0x91_ctx.response_data.battery_voltage * 4.44 : 0;

	if(voltage > 20) {
		state = 1;
	}

	_printf("%s:%s:%d state:%d\n", __FILE__, __func__, __LINE__, state);

	return state;
}

static a_f_b_command_item_t *a_f_b_command_table[] = {
	&a_f_b_command_item_0x15_0x95,
	&a_f_b_command_item_0x11_0x91,
};

static void a_f_b_process_requesst(a_f_b_info_t *a_f_b_info)
{
	int ret = -1;
	int i;

	for(i = 0; i < sizeof(a_f_b_command_table) / sizeof(a_f_b_command_item_t *); i++) {
		a_f_b_command_item_t *item = a_f_b_command_table[i];
		a_f_b_head_t *tx_head = (a_f_b_head_t *)a_f_b_info->tx_buffer;
		a_f_b_head_t *rx_head = (a_f_b_head_t *)a_f_b_info->rx_buffer;
		uint8_t connect_state_index;


		if(a_f_b_info->cmd_ctx[item->cmd].state == A_F_B_STATE_IDLE) {
			continue;
		}

		connect_state_index = a_f_b_info->connect_state_index;

		a_f_b_info->connect_state_index++;

		if(a_f_b_info->connect_state_index >= A_F_B_CONNECT_STATE_SIZE) {
			a_f_b_info->connect_state_index = 0;
		}

		a_f_b_info->connect_state[connect_state_index] = 0;

		tx_head->magic.b0 = 0xa5;
		tx_head->magic.b1 = 0x5a;
		tx_head->device_id = 0x00;
		tx_head->cmd = item->request_code;

		memset(a_f_b_info->rx_buffer, 0, A_F_B_BUFFER_SIZE);

		ret = item->request_callback(a_f_b_info);
		ret = uart_tx_rx_data(a_f_b_info->uart_info, a_f_b_info->rx_buffer, a_f_b_info->tx_size, a_f_b_info->rx_buffer, a_f_b_info->rx_size, 1000);

		if(ret != a_f_b_info->rx_size) {
			a_f_b_info->cmd_ctx[item->cmd].state = A_F_B_STATE_ERROR;
			continue;
		}

		if(rx_head->cmd != item->response_code) {
			a_f_b_info->cmd_ctx[item->cmd].state = A_F_B_STATE_ERROR;
			continue;
		}

		ret = item->response_callback(a_f_b_info);

		if(ret == 0) {
			a_f_b_info->cmd_ctx[item->cmd].state = A_F_B_STATE_IDLE;
			a_f_b_info->connect_state[connect_state_index] = 1;
		} else {
			a_f_b_info->cmd_ctx[item->cmd].state = A_F_B_STATE_ERROR;
		}

	}
}

static void a_f_b_periodic(a_f_b_info_t *a_f_b_info)
{
	int ret;
	ret = request_a_f_b_status_data(a_f_b_info);

	if(ret != 0) {
	}
}

void task_auxiliary_function_board_decode(void const *argument)
{
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)argument;

	if(a_f_b_info == NULL) {
		app_panic();
	}

	while(1) {
		a_f_b_process_requesst(a_f_b_info);
		a_f_b_periodic(a_f_b_info);
		osDelay(10);
	}
}
