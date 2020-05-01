

/*================================================================
 *
 *
 *   文件名称：auxiliary_function_board.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月28日 星期二 11时34分23秒
 *   修改日期：2020年05月01日 星期五 16时58分48秒
 *   描    述：
 *
 *================================================================*/
#ifndef _AUXILIARY_FUNCTION_BOARD_H
#define _AUXILIARY_FUNCTION_BOARD_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "list_utils.h"
#include "usart_txrx.h"

#include "channel_config.h"

#ifdef __cplusplus
}
#endif

#define A_F_B_BUFFER_SIZE 128
#define A_F_B_CONNECT_STATE_SIZE 10
#define A_F_B_CONNECT_STATE_OK_SIZE 6

typedef enum {
	A_F_B_STATE_IDLE = 0,
	A_F_B_STATE_REQUEST,
	A_F_B_STATE_ERROR,
} a_f_b_state_t;

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

//------------------------------------------
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
//------------------------------------------
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
//------------------------------------------

typedef struct {
	a_f_b_state_t state;
} a_f_b_cmd_ctx_t;

typedef enum {
	A_F_B_CMD_0X15_0X95 = 0,
	A_F_B_CMD_0X11_0X91,
	A_F_B_CMD_TOTAL,
} a_f_b_cmd_t;

typedef struct {
	struct list_head list;
	uart_info_t *uart_info;
	channel_info_config_t *channel_info_config;

	uint8_t tx_buffer[A_F_B_BUFFER_SIZE];
	uint8_t tx_size;
	uint8_t rx_buffer[A_F_B_BUFFER_SIZE];
	uint8_t rx_size;
	a_f_b_cmd_ctx_t cmd_ctx[A_F_B_CMD_TOTAL];
	a_f_b_0x15_0x95_ctx_t a_f_b_0x15_0x95_ctx;
	a_f_b_0x11_0x91_ctx_t a_f_b_0x11_0x91_ctx;
	uint8_t connect_state[A_F_B_CONNECT_STATE_SIZE];
	uint8_t connect_state_index;
} a_f_b_info_t;

typedef int (*a_f_b_request_callback_t)(a_f_b_info_t *a_f_b_info);
typedef int (*a_f_b_response_callback_t)(a_f_b_info_t *a_f_b_info);

typedef struct {
	a_f_b_cmd_t cmd;
	uint8_t request_code;
	a_f_b_request_callback_t request_callback;
	uint8_t response_code;
	a_f_b_response_callback_t response_callback;
} a_f_b_command_item_t;

void free_a_f_b_info(a_f_b_info_t *a_f_b_info);
a_f_b_info_t *get_or_alloc_a_f_b_info(channel_info_config_t *channel_info_config);
uint8_t get_a_f_b_connect_state(a_f_b_info_t *a_f_b_info);
int request_discharge(a_f_b_info_t *a_f_b_info);
int response_discharge(a_f_b_info_t *a_f_b_info);
int request_insulation_check(a_f_b_info_t *a_f_b_info);
int response_insulation_check(a_f_b_info_t *a_f_b_info);
int request_a_f_b_status_data(a_f_b_info_t *a_f_b_info);
int response_discharge_running_status(a_f_b_info_t *a_f_b_info);
int response_insulation_check_running_status(a_f_b_info_t *a_f_b_info);
a_f_b_reponse_91_data_t *get_a_f_b_status_data(a_f_b_info_t *a_f_b_info);
uint8_t get_battery_available_state(a_f_b_info_t *a_f_b_info);
#endif //_AUXILIARY_FUNCTION_BOARD_H
