

/*================================================================
 *
 *
 *   文件名称：request_sse.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月27日 星期四 13时09分48秒
 *   修改日期：2021年05月28日 星期五 17时45分31秒
 *   描    述：
 *
 *================================================================*/
#include <string.h>
#include <ctype.h>

#include "net_client.h"
#include "modbus_spec.h"
#include "command_status.h"

#include "log.h"

#pragma pack(push, 1)

typedef struct {
	uint16_t magic;//0xf5aa
	uint16_t frame_len;
	uint8_t version;
	uint16_t serial;
	struct {
		uint8_t cmd : 7;
		uint8_t type : 1;//0:request 1:response
	} cmd;
	uint8_t data[0];
} sse_frame_header_t;

typedef struct {
	uint8_t crc;
} sse_frame_crc_t;

typedef enum {
	SSE_CONST_FRAME_HEADER_LEN = sizeof(sse_frame_header_t),
	SSE_CONST_FRAME_CRC_LEN = sizeof(sse_frame_crc_t),
	SSE_CONST_FRAME_TYPE_REQUEST = 0,
	SSE_CONST_FRAME_TYPE_RESPONSE = 1,
} sse_const_t;

//充电桩定时上报工作状态
typedef struct {
	uint8_t insulation : 1;
	uint8_t telemeter : 1;
	uint8_t card_reader : 1;
	uint8_t display : 1;
	uint8_t fault : 1;
	uint8_t unused : 3;
	uint8_t channel_connect_0 : 1;
	uint8_t channel_connect_1 : 1;
	uint8_t channel_connect_2 : 1;
	uint8_t channel_connect_3 : 1;
	uint8_t channel_connect_4 : 1;
	uint8_t channel_connect_5 : 1;
	uint8_t channel_connect_6 : 1;
	uint8_t channel_connect_7 : 1;
} sse_device_state_t;

typedef union {
	sse_device_state_t s;
	uint16_t v;
} u_sse_device_state_t

typedef enum {
	SSE_DEVICE_FAULT_TYPE_NONE = 0,
	SSE_DEVICE_FAULT_TYPE_MODULE,
	SSE_DEVICE_FAULT_TYPE_CARD_READER,
	SSE_DEVICE_FAULT_TYPE_CONTROL_BOARD_B,
	SSE_DEVICE_FAULT_TYPE_CONTROL_BOARD_C,
	SSE_DEVICE_FAULT_TYPE_CONTROL_BOARD_D,
	SSE_DEVICE_FAULT_TYPE_FUNCTION_BOARD_A,
	SSE_DEVICE_FAULT_TYPE_FUNCTION_BOARD_B,
	SSE_DEVICE_FAULT_TYPE_FUNCTION_BOARD_C,
	SSE_DEVICE_FAULT_TYPE_FUNCTION_BOARD_D,
	SSE_DEVICE_FAULT_TYPE_TELEMETER_A,
	SSE_DEVICE_FAULT_TYPE_TELEMETER_B,
	SSE_DEVICE_FAULT_TYPE_TELEMETER_C,
	SSE_DEVICE_FAULT_TYPE_TELEMETER_D,
	SSE_DEVICE_FAULT_TYPE_DOOR,
	SSE_DEVICE_FAULT_TYPE_RELAY_ADHESION,
	SSE_DEVICE_FAULT_TYPE_FORCE_STOP,
	SSE_DEVICE_FAULT_TYPE_INPUT_OVER_VOLTAGE,
	SSE_DEVICE_FAULT_TYPE_INPUT_LOW_VOLTAGE,
	SSE_DEVICE_FAULT_TYPE_CHANNEL_OVER_TEMPERATURE,
} sse_device_fault_type_t;

typedef struct {
	uint8_t connect : 1;
	uint8_t auxiliary_power_12 : 1;//12v
	uint8_t auxiliary_power_24 : 1;//24v
	uint8_t vehicle_relay_state : 1;
	uint8_t channel_lock_state : 1;
	uint8_t unused : 3;
} sse_channel_device_state_t;

typedef union {
	sse_channel_device_state_t s;
	uint8_t v;
} u_sse_channel_device_state_t;

typedef enum {
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_NONE = 0,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_BRM_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_BCP_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_BRO_READY_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_BCS_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_BCL_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_BST_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_BSD_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_CRO_NOT_READY,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_RUNNING,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_IDLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_INSULATION_CHECK_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_INSULATION_FAULT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_DISCHARGE_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_DISCHARGE_FAULT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_ADHESION_CHECK_TIMEOUT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_ADHESION_FAULT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_BCS_READY,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_INSULATION_CHECK_WITH_ELECTRIFIED,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_STARTING,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_ABNORMAL_VOLTAGE,
} sse_report_channel_charger_bms_state_t;

typedef enum {
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SOC_ARCHIEVED = 1,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SOC_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_VOLTAGE_ARCHIEVED,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_VOLTAGE_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SINGLE_VOLTAGE_ARCHIEVED,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SINGLE_VOLTAGE_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_INSULATION_FAULT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_INSULATION_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CONNECTOR_OVER_TEMPERATURE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CONNECTOR_TEMPERATURE_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BMS_CONNECTOR_OVER_TEMPERATURE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CONNECTOR_FAULT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CONNECTOR_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BATTERY_OVER_TEMPERATURE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BATTERY_TEMPERATURE_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_OTHER_FAULT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_OTHER_FAULT_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_OVER_CURRENT,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CURRENT_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_ABNORMAL_VOLTAGE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_ABNORMAL_VOLTAGE_NOT_CREDIBLE,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BMS_OTHER_FAULT,
} sse_report_channel_charger_bms_stop_reason_t;

//dc
#if !defined(SSE_AC_CHARGER)
typedef struct {
	uint8_t soc;
	uint16_t bcl_require_voltage;
	uint16_t bcl_require_current;
	uint16_t output_voltage;
	uint16_t output_current;
	uint16_t bcs_charge_voltage;
	uint16_t bcs_charge_current;
	uint32_t telemeter_total;//0.01kWh <=V23 0.0001kWh V24
	uint32_t charge_energy;//0.01kWh <=V23 0.0001kWh V24
	uint32_t charge_amount;
	uint16_t bcs_single_battery_max_voltage;
	uint16_t bsm_battery_max_temperature;
	uint16_t bcs_remain_min;
	uint8_t dc_p_temperature;
	uint8_t dc_n_temperature;
} sse_report_channel_charge_info_t;
#else//#if !defined(SSE_AC_CHARGER)
typedef struct {
	uint16_t output_voltage;
	uint16_t output_current;
	uint32_t telemeter_total;
	uint32_t charge_energy;
	uint32_t charge_amount;
} sse_report_channel_charge_info_t;
#endif//#if !defined(SSE_AC_CHARGER)

typedef struct {
	uint8_t channel_id;
	uint8_t channel_stop_reason;//0:充电机未主动停机 1:人工停机 2:电池温度超限停机 3:超过最大允许充电电压停机
	u_sse_channel_device_state_t u_sse_channel_device_state;
	uint8_t channel_work_state;//0:待机 1:枪已插好
	uint8_t sse_report_channel_charger_bms_state;//sse_report_channel_charger_bms_state_t
	uint8_t sse_report_channel_charger_bms_stop_reason;//sse_report_channel_charger_bms_stop_reason_t
	sse_report_channel_charge_info_t sse_report_channel_charge_info;
} sse_channel_report_t;

typedef struct {
	uint8_t device_id[32];//1 桩编码
	uint8_t device_type;//2 桩类型 1 直流 2 交流
	uint8_t date_time[8];//装置时间 bcd 20161223135922ff
	u_sse_device_state_t device_state;
	uint16_t power_threshold;//单位 0.1kW V22
	uint16_t sse_device_fault_type;//V23 sse_device_fault_type_t
	uint8_t app;//0:原始版本 1:升级版本 V23
	uint32_t module_state_mask;//模块总数掩码 V23
	uint32_t module_state_value;//按位表示对于序号的模块工作状态 0 表示故障 1 正常 V23
	uint32_t float_percision;//字节数顺序描述 V24
	uint8_t channel_number;
	sse_channel_report_t sse_channel_report[0];
} sse_0x00_request_report_t;

typedef struct {
	uint8_t status;//0 异常 1 正常
} sse_0x00_response_report_t;

typedef struct {
	uint8_t channel_id;
	uint8_t vin[17];//字符串
	uint16_t chm_bms_version;//chm 低两个字节
	uint16_t brm_battery_type;
	uint16_t bcp_rate_total_power;
	uint16_t bcp_total_voltage;
	uint16_t bcp_max_charge_voltage_single_battery;
	uint16_t bcp_max_temperature;//无偏移
	uint16_t bcp_max_charge_voltage;
	uint8_t card_id[32];//字符串
	uint32_t start_type;//0 手动开机 1 刷卡开机 2 遥控开机 3 VIN 开机 5 BMS 请求开机 6 插枪自动启动
	uint8_t start_dt[8];//bcd 20161223135922ff
	uint32_t telemeter_total;//0.01kWh <=V23 0.0001kWh V24
	uint32_t withholding;//告诉后台此卡的预扣款是多少 0.01 元
	uint8_t soc;
	uint8_t vehicle_number[16];
} sse_request_event_start_charge_t;

typedef struct {
	uint32_t display : 1;
	uint32_t control_board_1 : 1;
	uint32_t control_board_2 : 1;
	uint32_t control_board_3 : 1;
	uint32_t control_board_4 : 1;
	uint32_t control_board_5 : 1;
	uint32_t control_board_6 : 1;
	uint32_t control_board_7 : 1;
	uint32_t control_board_8 : 1;
	uint32_t function_board_1 : 1;
	uint32_t function_board_2 : 1;
	uint32_t function_board_3 : 1;
	uint32_t function_board_4 : 1;
	uint32_t function_board_5 : 1;
	uint32_t function_board_6 : 1;
	uint32_t function_board_7 : 1;
	uint32_t function_board_8 : 1;
	uint32_t telemeter_1 : 1;
	uint32_t telemeter_2 : 1;
	uint32_t telemeter_3 : 1;
	uint32_t telemeter_4 : 1;
	uint32_t card_reader : 1;
	uint32_t display1 : 1;
} sse_event_fault_type_t;

typedef union {
	sse_event_fault_type_t s;
	uint32_t v;
} u_sse_event_fault_type_t;

typedef union {
	sse_device_state_t s;
	uint16_t v;
} u_sse_event_fault_status_t;

typedef struct {
	u_sse_event_fault_type_t type;
	u_sse_event_fault_status_t status;
} sse_request_event_fault_t;

typedef enum {
	SSE_RECORD_STOP_REASON_NONE = 0,
	SSE_RECORD_STOP_REASON_MANUAL,
	SSE_RECORD_STOP_REASON_BATTERY_OVER_TEMPERATURE,
	SSE_RECORD_STOP_REASON_OVER_VOLTAGE,
	SSE_RECORD_STOP_REASON_DURATION_ARCHIEVED,
	SSE_RECORD_STOP_REASON_OVER_SETTING_VOLTAGE,
	SSE_RECORD_STOP_REASON_CONNECTOR_FAULT,
	SSE_RECORD_STOP_REASON_OVER_SINGLE_VOLTAGE,
	SSE_RECORD_STOP_REASON_REMOTE,
	SSE_RECORD_STOP_REASON_OFFLINE,
	SSE_RECORD_STOP_REASON_TELEMETER,
	SSE_RECORD_STOP_REASON_OVER_CHARGER_TEMPERATURE,
	SSE_RECORD_STOP_REASON_OVER_INPUT_VOLTAGE,
	SSE_RECORD_STOP_REASON_LOW_INPUT_VOLTAGE,
	SSE_RECORD_STOP_REASON_CHARGER_LOCK_FAULT,
	SSE_RECORD_STOP_REASON_BMS_FAULT,
	SSE_RECORD_STOP_REASON_POWER_MODULE,
	SSE_RECORD_STOP_REASON_BMS_TIMEOUT,
	SSE_RECORD_STOP_REASON_CONTROL_BOARD_OFFLINE,
	SSE_RECORD_STOP_REASON_NO_LOAD,
	SSE_RECORD_STOP_REASON_INSUFFICIENT_AMOUNT,
	SSE_RECORD_STOP_REASON_AMOUNT_ARCHIEVED,
	SSE_RECORD_STOP_REASON_SOC_ARCHIEVED,
	SSE_RECORD_STOP_REASON_DATE_TIME_ARCHIEVED,
	SSE_RECORD_STOP_REASON_REQUIRE_LOW_VOLTAGE,
	SSE_RECORD_STOP_REASON_CARD,
	SSE_RECORD_STOP_REASON_DISPLAY_OFFLINE,
	SSE_RECORD_STOP_REASON_NO_POWER,
	SSE_RECORD_STOP_REASON_TEMPERATURE_SENSOR_OFFLINE,
	SSE_RECORD_STOP_REASON_RELAY_OFFLINE,
	SSE_RECORD_STOP_REASON_APP,
	SSE_RECORD_STOP_REASON_OVER_CURRENT,
	SSE_RECORD_STOP_REASON_FORCE_STOP,
	SSE_RECORD_STOP_REASON_CP_DISCONNECT,
	SSE_RECORD_STOP_REASON_INSULATION,
} sse_record_stop_reason_t;

typedef struct {
	uint8_t section_id;
	uint32_t energy;//2字节 <=V23
} sse_record_section_t;

typedef struct {
	uint16_t record_id;
	uint8_t channel_id;
	uint8_t vin[17];//字符串
	uint16_t chm_bms_version;//chm 低两个字节
	uint16_t brm_battery_type;
	uint16_t bcp_rate_total_power;
	uint16_t bcp_total_voltage;
	uint16_t bcp_max_charge_voltage_single_battery;
	uint16_t bcp_max_temperature;//无偏移
	uint16_t bcp_max_charge_voltage;
	uint8_t card_id[32];//字符串
	uint32_t start_type;//0 手动开机 1 刷卡开机 2 遥控开机 3 VIN 开机 5 BMS 请求开机 6 插枪自动启动
	uint8_t start_dt[8];//bcd 20161223135922ff
	uint32_t telemeter_total;//0.01kWh <=V23 0.0001kWh V24
	uint32_t withholding;//告诉后台此卡的预扣款是多少 0.01 元
	uint8_t stop_reason;//sse_record_stop_reason_t;
	uint8_t stop_dt[8];//bcd 20161223135922ff
	uint32_t telemeter_total;//0.01kWh <=V23 0.0001kWh V24
	uint32_t charge_amount;//0.01元
	uint32_t charge_energy;//0.01kWh <=V23 0.0001kWh V24
	uint8_t start_soc;
	uint8_t stop_soc;
	uint8_t vehicle_number[16];
	uint8_t section_number;//动态量有几个填几个,跨天按时间序号填
	sse_record_section_t sse_record_section[0];
} sse_request_event_record_t;

typedef struct {
	uint8_t device_id[32];//1 桩编码
	uint8_t device_type;//2 桩类型 1 直流 2 交流
	uint32_t float_percision;//字节数顺序描述 V24
	uint8_t event_type;//开始充电 0x00;异常 0x02;未上传充电记录 0x03;
	uint8_t event_info[0];
} sse_0x01_request_event_t;

typedef struct {
	uint8_t status;//0 异常 1 正常
	uint8_t device_id[32];//1 桩编码
	uint8_t event_type;
	uint16_t record_id;
} sse_0x01_response_event_t;

typedef struct {
	uint16_t start_hour_min;//BCD 码
	uint16_t end_hour_min;//BCD 码
	uint32_t price;
} sse_query_price_item_info_t;

typedef struct {
	uint8_t count;
	sse_query_price_item_info_t item[0];
} sse_query_price_info_t;

typedef struct {
	uint16_t start_hour_min;//BCD 码
	uint16_t end_hour_min;//BCD 码
} sse_query_ad_item_info_t;

typedef struct {
	uint8_t count;
	sse_query_ad_item_info_t item[0];
} sse_query_ad_info_t;

typedef struct {
	uint8_t device_id[32];//1 桩编码
	uint8_t card_id[32];//字符串
	uint8_t card_password[32];//字符串
	uint32_t withholding;//0.01 元
} sse_query_card_account_info_t;

typedef enum {
	SSE_QUERY_CARD_ERROR_REASON_PASSWORD = 0;
	SSE_QUERY_CARD_ERROR_REASON_AMOUNT,
	SSE_QUERY_CARD_ERROR_REASON_STOP,
	SSE_QUERY_CARD_ERROR_REASON_UNUSED,
	SSE_QUERY_CARD_ERROR_REASON_NOT_RECOGNIZE,
} sse_query_card_error_reason_t;

typedef struct {
	uint8_t valid;
	uint8_t reason;//sse_query_card_error_reason_t
	uint32_t card_balance;
} sse_query_card_account_confirm_t;

typedef struct {
	uint8_t device_id[32];//1 桩编码
	uint8_t channel_id;//从 1 开始计数
	uint32_t withholding;//0.01 元
} sse_query_qr_code_info_t;

typedef enum {
	sse_query_qr_code_error_reason_failed = 0,
	sse_query_qr_code_error_reason_no_para,
	sse_query_qr_code_error_reason_data_error,
	sse_query_qr_code_error_reason_order_failed,
} sse_query_qr_code_error_reason_t;

typedef struct {
	uint8_t valid;
	uint8_t reason;//sse_query_qr_code_error_reason_t
	uint8_t channel_id;//从 1 开始计数
	uint32_t withholding;//0.01 元
	uint8_t qr_code[100];
} sse_query_qr_code_confirm_t;

typedef struct {
	uint8_t type;//0:device information 1:device ad 2:card account information 3:qr code start
	uint8_t id;
	uint8_t query[0];
} sse_0x02_request_query_t;

typedef struct {
	uint8_t type;//0:device information 1:device ad 2:card account information 3:qr code start
	uint8_t id;
	uint8_t query[0];
} sse_0x02_response_query_t;

typedef struct {
	uint8_t status;
} sse_0x03_request_settings_t;

typedef struct {
	uint8_t type;//0:device information 1:device ad 2:ad data 3:power threshold--0.1kw
	uint8_t id;
	uint8_t query[0];
} sse_0x03_response_settings_t;

typedef struct {
	uint8_t status;//0 异常 1 正常
	uint8_t type;
	uint8_t result;//0 执行失败 1 执行成功
	uint8_t reason;//遥控类型对应的失败原因
} sse_0x04_request_remote_t;

typedef struct {
	uint8_t device_id[32];//1 桩编码
	uint8_t device_type;//2 桩类型 1 直流 2 交流
	uint8_t cmd;//0 结束充电 1 锁定 2 解锁 3 开始充电当预约成功时需要由后台发 送锁定命令。
	uint8_t mode;//解锁/开始充电时有效,一般是按金额充电模式 0 充满位置 1 按金额充 2 按时间充 3 按电量充 4 VIN 充电
	uint32_t data;//解锁/开始充电时有效,充满为止时为0, 按金额充时为金额(0.01元), 按时间充时高16位BCD码开始时分,低16位BCD码结束时分,按电量充时为电 量(0.01kWh)
} sse_remote_start_stop_t;

typedef struct {
	uint8_t device_id[32];//1 桩编码
	uint8_t on_off;//0 关 1 开
} sse_remote_ad_on_off_t;

typedef struct {
	uint8_t type;//0:start/stop 1:ad on/off
	uint8_t id;
	uint8_t remote[0];
} sse_0x04_response_remote_t;

typedef struct {
	uint8_t status;//0 异常 1 正常
	uint32_t offset;//本次固件数据起始地址
} sse_0x05_request_upgrade_t;

typedef struct {
	uint32_t file_len;//固件总字节数
	uint32_t offset;//从 0 开始计数
	uint8_t data[256];
} sse_0x05_response_upgrade_t;

#pragma pack(pop)

typedef struct {
	uint8_t start_finish;
	uint8_t start_code;
	uint8_t start_failed_reason;

	command_status_t *channel_cmd_ctx;
} net_client_channel_data_ctx_t;

typedef struct {
	uint8_t request_retry;
	uint16_t serial;
	uint16_t transaction_record_id;
	channels_info_t *channels_info;

	command_status_t *device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx;
} net_client_data_ctx_t;

typedef enum {
	NET_CLIENT_DEVICE_COMMAND_REPORT = 0,
	NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD,
} net_client_device_command_t;

typedef enum {
	NET_CLIENT_CHANNEL_COMMAND_REPORT = 0,
} net_client_channel_command_t;

typedef int (*net_client_request_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_response_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_timeout_callback_t)(net_client_info_t *net_client_info, void *_command_item);

typedef struct {
	net_client_command_t cmd;
	uint32_t periodic;
	uint16_t request_frame;
	net_client_request_callback_t request_callback;
	uint16_t response_frame;
	net_client_response_callback_t response_callback;
	net_client_timeout_callback_t timeout_callback;
} net_client_command_item_t;

static net_client_data_ctx_t *net_client_data_ctx = NULL;

static uint8_t sse_crc(uint8_t *data, size_t len)
{
	uint8_t crc = 0;
	int i;

	for(i = 0; i < len; i++) {
		crc += data[i];
	}

	return crc;
}

static int send_frame(net_client_info_t *net_client_info, uint16_t serial, uint8_t frame, uint8_t type, uint8_t *message, uint8_t len)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)message;
	sse_frame_crc_t *sse_frame_crc = (sse_frame_crc_t *)(message + len);

	sse_frame_header -= 1;

	sse_frame_header->magic = 0xf5aa;
	sse_frame_header->frame_len = len + SSE_CONST_FRAME_HEADER_LEN + SSE_CONST_FRAME_CRC_LEN;
	sse_frame_header->version = 0x24;
	sse_frame_header->serial = serial;
	sse_frame_header->cmd.cmd = frame;
	sse_frame_header->cmd.type = type;
	sse_frame_crc->crc = sse_crc((uint8_t *)sse_frame_header, sse_frame_header->frame_len - SSE_CONST_FRAME_CRC_LEN);

	if(send_to_server(net_client_info, (uint8_t *)sse_frame_header, sse_frame_header->frame_len) == sse_frame_header->frame_len) {
		ret = 0;
	}

	return ret;
}

static void report_transaction_record(uint16_t record_id)
{
	if(net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD].state != COMMAND_STATE_IDLE) {
		return;
	}

	net_client_data_ctx->transaction_record_id = record_id;
	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD].state = COMMAND_STATE_REQUEST;
}

static void sync_transaction_record(void)
{
	uint8_t AA = 0;
	int i;
	int channel_id;

	if(net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD].state != COMMAND_STATE_IDLE) {
		return;
	}

	//find record to upload
	//report_transaction_record(record_id);
}

static uint16_t get_device_state(channels_info_t *channels_info)
{
	u_sse_device_state_t device_state;
	u_uint16_bytes_t *u_uint16_bytes = (u_uint16_bytes_t *)&device_state;
	uint8_t connects = 0;
	int i;
	device_state.v = 0;
	device_state.s.faults.insulation = get_channels_info_fault(channels_info, CHANNELS_EVENT_CHANNEL_INSULATION);
	device_state.s.faults.telemeter = get_channels_info_fault(channels_info, CHANNELS_EVENT_CHANNEL_TELEMETER);
	device_state.s.faults.card_reader = get_channels_info_fault(channels_info, CHANNELS_EVENT_CHANNEL_CARD_READER);
	device_state.s.faults.display = get_channels_info_fault(channels_info, CHANNELS_EVENT_CHANNEL_DISPLAY);
	device_state.s.faults.fault = (get_channels_info_first_fault(channels_info) >= CHANNELS_FAULT_SIZE) ? 0 : 1;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;
		charger_info_t *charger_info = (charger_info_t *)channel_info->channel_info;

		connects = set_u8_bits(connects, i, charger_info->connect_state);
	}

	u_uint16_bytes->s.byte1 = connects;

	return device_state.v;
}

static uint16_t get_sse_device_fault_type(channels_info_t *channels_info)
{
	uint16_t fault = SSE_DEVICE_FAULT_TYPE_NONE;

	channels_fault_t channels_fault = get_channels_info_first_fault(channels_info);

	if(channels_fault >= CHANNELS_FAULT_SIZE) {
		return fault;
	}

	//todo 

	return fault;
}

static uint32_t get_sse_module_state_mask(channels_info_t *channels_info)
{
	uint32_t mask = 0;
	uint8_t *cells = (uint8_t *)&mask;
	channels_config_t *channels_config = channels_info->channels_config;
	channels_power_module_t *channels_power_module = (channels_power_module_t *)channels_info->channels_power_module;
	channels_power_module_callback_t *channels_power_module_callback = channels_power_module->channels_power_module_callback;
	int i = 0;
	int channels_power_module_number = channels_config->power_module_config.channels_power_module_number;

	if(channels_power_module_callback == NULL) {
		return mask;
	}

	if(channels_power_module_callback->get_power_module_item_info == NULL) {
		return mask;
	}

	for(i = 0; i < channels_power_module_number; i++) {
		int j = i / 8;
		int k = i % 8;
		cells[j] = set_u8_bits(cells[j], k, 1);
	}

	return mask;
}

static int request_callback_report(net_client_info_t *net_client_info, void *_command_item, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x00_request_report_t *sse_0x00_request_report = (sse_0x00_request_report_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	struct tm *tm = localtime(get_time());
	char dt[20];

	snprintf(sse_0x00_request_report->device_id, 32, "%s", channels_settings->device_id);
	sse_0x00_request_report->device_type = channels_settings->device_type;
	memset(sse_0x00_request_report->date_time, 0xff, sizeof(sse_0x00_request_report->date_time));
	strftime(dt, sizeof(dt), "%Y%m%d%H%M%S", tm);
	ascii_to_bcd(dt, strlen(dt), sse_0x00_request_report->date_time, sizeof(sse_0x00_request_report->date_time));
	sse_0x00_request_report->device_state.v = get_device_state(channels_info);
	sse_0x00_request_report->power_threshold = channels_settings->power_threshold;
	sse_0x00_request_report->sse_device_fault_type = get_sse_device_fault_type(channels_info);
	sse_0x00_request_report->app = is_app();
	sse_0x00_request_report->module_state_mask = get_sse_module_state_mask(channels_info);
	sse_0x00_request_report->module_state_value = get_sse_module_state_value(channels_info);

	send_frame(net_client_info, net_client_data_ctx->serial++, item->request_frame, (uint8_t *), sizeof());

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_LOGIN].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_callback_report(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x02_response_login_t *sse_0x02_response_login = (sse_0x02_response_login_t *)(sse_frame_header + 1);
	uint8_t device_id[7];

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x02_response_login->device_id, 7) != 0) {
		net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_LOGIN].state = COMMAND_STATE_REQUEST;
		return ret;
	}

	if(sse_0x02_response_login->code != 0x00) {
		net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_LOGIN].state = COMMAND_STATE_REQUEST;
		return ret;
	}

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_LOGIN].state = COMMAND_STATE_IDLE;
	ret = 0;

	//login successful!!!
	login_callback();
	return ret;
}

static net_client_command_item_t net_client_command_item_login = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_LOGIN,
	.request_frame = 0x01,//通信中断后上电复位
	.request_callback = request_callback_login,
	.response_frame = 0x02,
	.response_callback = response_callback_login,
};

//xiaofei
static void get_device_billing_mode(void)
{
	//device_billing_mode_t *device_billing_mode;

	OS_ASSERT(net_client_data_ctx != NULL);
	//device_billing_mode = &net_client_data_ctx->device_billing_mode;
}

//xiaofei

static void set_modbus_price(int index, uint16_t start_hour, uint16_t start_min, uint16_t service_price, uint32_t price)
{
	if(Channel_A_Charger.pChannel_Display->Extend_Setting.Extend_Sys_Setting.Elc_Price_Setting == 2) { //4位小数
		uint16_t *addr;

		Channel_A_Charger.pChannel_Display->Extend_Setting.Extend_Sys_Setting.Price[index].Start_Time_Hour = start_hour;
		Channel_A_Charger.pChannel_Display->Extend_Setting.Extend_Sys_Setting.Price[index].Start_Time_Min = start_min;
		Channel_A_Charger.pChannel_Display->Extend_Setting.Extend_Sys_Setting.Price[index].Energy_Price_L = get_u16_0_from_u32(price / 10);
		Channel_A_Charger.pChannel_Display->Extend_Setting.Extend_Sys_Setting.Price[index].Energy_Price_H = get_u16_1_from_u32(price / 10);

		addr = &Channel_A_Charger.pChannel_Display->Extend_Setting.Extend_Sys_Setting.Price[index].Start_Time_Hour;
		ModBus_Write_eeprom(((uint32_t)addr - (uint32_t)Modbus_Record_Table) / 2 + 2000, addr, 4);
	} else {
		if(index == 0) {
			pModBus_Data->System.Setting_Info.Price_1.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_1.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_1.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_1.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_1.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_1.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_1.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_1.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_1.Energy_Price, 1);
		} else if(index == 1) {
			pModBus_Data->System.Setting_Info.Price_2.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_2.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_2.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_2.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_2.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_2.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_2.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_2.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_2.Energy_Price, 1);
		} else if(index == 2) {
			pModBus_Data->System.Setting_Info.Price_3.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_3.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_3.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_3.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_3.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_3.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_3.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_3.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_3.Energy_Price, 1);
		} else if(index == 3) {
			pModBus_Data->System.Setting_Info.Price_4.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_4.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_4.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_4.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_4.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_4.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_4.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_4.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_4.Energy_Price, 1);
		} else if(index == 4) {
			pModBus_Data->System.Setting_Info.Price_5.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_5.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_5.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_5.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_5.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_5.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_5.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_5.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_5.Energy_Price, 1);
		} else if(index == 5) {
			pModBus_Data->System.Setting_Info.Price_6.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_6.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_6.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_6.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_6.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_6.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_6.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_6.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_6.Energy_Price, 1);
		} else if(index == 6) {
			pModBus_Data->System.Setting_Info.Price_7.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_7.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_7.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_7.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_7.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_7.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_7.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_7.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_7.Energy_Price, 1);
		} else if(index == 7) {
			pModBus_Data->System.Setting_Info.Price_8.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_8.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_8.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_8.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_8.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_8.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_8.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_8.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_8.Energy_Price, 1);
		} else if(index == 8) {
			pModBus_Data->System.Setting_Info.Price_9.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_9.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_9.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_9.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_9.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_9.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_9.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_9.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_9.Energy_Price, 1);
		} else if(index == 9) {
			pModBus_Data->System.Setting_Info.Price_10.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_10.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_10.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_10.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_10.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_10.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_10.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_10.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_10.Energy_Price, 1);
		} else if(index == 10) {
			pModBus_Data->System.Setting_Info.Price_11.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_11.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_11.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_11.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_11.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_11.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_11.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_11.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_11.Energy_Price, 1);
		} else if(index == 11) {
			pModBus_Data->System.Setting_Info.Price_12.Start_Time_Hour = start_hour;
			pModBus_Data->System.Setting_Info.Price_12.Start_Time_Min = start_min;
			pModBus_Data->System.Setting_Info.Price_12.Energy_Price = price / 1000;
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_12.Start_Time_Hour - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_12.Start_Time_Hour, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_12.Start_Time_Min - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_12.Start_Time_Min, 1);
			Write_ModBus_Setting_Data_One((((uint32_t)&pModBus_Data->System.Setting_Info.Price_12.Energy_Price - (uint32_t)ModBus_Table) / 2), &pModBus_Data->System.Setting_Info.Price_12.Energy_Price, 1);
		}
	}

}

static int set_device_billing_mode(void)
{
	int ret = 0;
	int i;
	int index;
	int price_segment;
	uint8_t price_index;
	device_billing_mode_t *device_billing_mode;
	uint32_t price;
	time_t start;
	struct tm *tm;

	OS_ASSERT(net_client_data_ctx != NULL);
	device_billing_mode = &net_client_data_ctx->device_billing_mode;


	price_segment = 0;
	index = 0;
	price_index = device_billing_mode->section[index];

	for(i = 0; i < 48; i++) {
		if((i + 1 < 48) && (device_billing_mode->section[i] == price_index)) {
			continue;
		}

		if(price_segment >= 12) {
			ret = -1;
			break;
		}

		switch(price_index) {
			case 0: {
				price = device_billing_mode->tip;
			}
			break;

			case 1: {
				price = device_billing_mode->peak;
			}
			break;

			case 2: {
				price = device_billing_mode->flat;
			}
			break;

			case 3: {
				price = device_billing_mode->valley;
			}
			break;

			default: {
				price = 0;
				ret = -1;
			}
			break;
		}

		if(ret != 0) {
			break;
		}

		start = index * 30 * 60;
		tm = localtime(&start);

		set_modbus_price(price_segment, tm->tm_hour, tm->tm_min, 0, price);

		price_segment++;
		index = i;
		price_index = device_billing_mode->section[index];

	}

	for(i = price_segment; i < 12; i++) {
		set_modbus_price(i, 0, 0, 0, 0);
	}

	return ret;
}

static int request_callback_billing_mode(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x05_request_billing_mode_t *sse_0x05_request_billing_mode = (sse_0x05_request_billing_mode_t *)(sse_frame_header + 1);
	device_billing_mode_t *device_billing_mode = &net_client_data_ctx->device_billing_mode;

	set_device_id_bcd(sse_0x05_request_billing_mode->device_id, 7);

	get_device_billing_mode();
	sse_0x05_request_billing_mode->billing_mode_id = device_billing_mode->billing_mode_id;

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x05_request_billing_mode, sizeof(sse_0x05_request_billing_mode_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_MODE].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_callback_billing_mode(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x06_response_billing_mode_t *sse_0x06_response_billing_mode = (sse_0x06_response_billing_mode_t *)(sse_frame_header + 1);
	uint8_t device_id[7];
	device_billing_mode_t *device_billing_mode;

	device_billing_mode = &net_client_data_ctx->device_billing_mode;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x06_response_billing_mode->device_id, 7) != 0) {
		return ret;
	}

	if(sse_0x06_response_billing_mode->code != 0x00) {
		net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_RULES].state = COMMAND_STATE_REQUEST;
	}

	if(sse_0x06_response_billing_mode->billing_mode_id != device_billing_mode->billing_mode_id) {
		net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_RULES].state = COMMAND_STATE_REQUEST;
	}

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_MODE].state = COMMAND_STATE_IDLE;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_billing_mode = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_BILLING_MODE,
	.request_frame = 0x05,//通信中断后上电复位
	.request_callback = request_callback_billing_mode,
	.response_frame = 0x06,
	.response_callback = response_callback_billing_mode,
};

static int request_callback_billing_rules(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	int i;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x07_request_billing_rules_t *sse_0x07_request_billing_rules = (sse_0x07_request_billing_rules_t *)(sse_frame_header + 1);

	set_device_id_bcd(sse_0x07_request_billing_rules->device_id, 7);

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x07_request_billing_rules, sizeof(sse_0x07_request_billing_rules_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_RULES].state = COMMAND_STATE_RESPONSE;

	for(i  = 0; i < get_device_gun_number(); i++) {
		net_client_data_ctx->channel_data_ctx[i].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].available = 0;
		net_client_data_ctx->channel_data_ctx[i].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REMOTE_START].available = 0;
	}

	return ret;
}

static int response_callback_billing_rules(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x08_response_billing_rules_t *sse_0x08_response_billing_rules = (sse_0x08_response_billing_rules_t *)(sse_frame_header + 1);
	uint8_t device_id[7];
	device_billing_mode_t *device_billing_mode = &net_client_data_ctx->device_billing_mode;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x08_response_billing_rules->device_id, 7) != 0) {
		net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_RULES].state = COMMAND_STATE_REQUEST;
		return ret;
	}

	device_billing_mode->billing_mode_id = sse_0x08_response_billing_rules->billing_mode_id;
	device_billing_mode->tip = sse_0x08_response_billing_rules->tip;
	device_billing_mode->peak = sse_0x08_response_billing_rules->peak;
	device_billing_mode->flat = sse_0x08_response_billing_rules->flat;
	device_billing_mode->valley = sse_0x08_response_billing_rules->valley;
	device_billing_mode->loss = sse_0x08_response_billing_rules->loss;
	memcpy(device_billing_mode->section, sse_0x08_response_billing_rules->section, sizeof(sse_0x08_response_billing_rules->section));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_RULES].state = COMMAND_STATE_IDLE;
	set_device_billing_mode();
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_billing_rules = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_BILLING_RULES,
	.request_frame = 0x07,//通信中断后上电复位
	.request_callback = request_callback_billing_rules,
	.response_frame = 0x08,
	.response_callback = response_callback_billing_rules,
};

typedef struct {
	uint8_t year_h;
	uint8_t year_l;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t second;
} record_bcd_time_t;

static void record_bcd_time_to_tm(record_bcd_time_t *time, struct tm *tm)
{
	memset(tm, 0, sizeof(struct tm));
	tm->tm_year = get_u16_from_bcd_b01(time->year_l, time->year_h) - 1900;
	tm->tm_mon = get_u8_from_bcd(time->month) - 1;
	tm->tm_mday = get_u8_from_bcd(time->day);
	tm->tm_hour = get_u8_from_bcd(time->hour);
	tm->tm_min = get_u8_from_bcd(time->min);
	tm->tm_sec = get_u8_from_bcd(time->second);
}

static uint32_t sum_record_energy(Channel_Record_TypeDef *record, uint8_t type)
{
	device_billing_mode_t *device_billing_mode = &net_client_data_ctx->device_billing_mode;
	int i;
	uint8_t energy = 0;

	for(i = 0; i < 48; i++) {
		if(device_billing_mode->section[i] == type) {
			energy += record->ElecQuantity[i];
		}
	}

	return energy;
}

static struct tm *local_time_to_tm(void)
{
	static struct tm tm = {0};
	tm.tm_sec = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_second);
	tm.tm_min = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_min);
	tm.tm_hour = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_hour);
	tm.tm_mday = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_day);
	tm.tm_mon = get_u8_from_bcd(pModBus_Data->System.Data_Info.time_month) - 1;
	tm.tm_year = get_u16_from_bcd_b01(get_u8_l_from_u16(pModBus_Data->System.Data_Info.time_year), get_u8_h_from_u16(pModBus_Data->System.Data_Info.time_year)) - 1900;
	return &tm;
}

static uint8_t get_record_stop_reason(Channel_Record_TypeDef *record)
{
	uint8_t code = 0x90;

	switch(record->Charge_Stop_Reason) {
		case APP_Stop: {
			code = 0x40;
		}
		break;

		case NORMAL_STOP: {
			code = 0x41;
		}
		break;

		case CONDITION_DURATION_STOP: {
			code = 0x44;
		}
		break;

		case PERSON_STOP: {
			code = 0x45;
		}
		break;

		case START_FAILED_STOP: {
			code = 0x4a;
		}
		break;

		case CHANNEL_DISCONNECT_STOP: {
			code = 0x4b;
		}
		break;

		case ELCMETER_DISCON_STOP: {
			code = 0x4d;
		}
		break;

		case CONDITION_NO_ENOUGH_MONEY_STOP: {
			code = 0x4e;
		}
		break;

		case POWER_NO_CONNECT_STOP: {
			code = 0x4f;
		}
		break;

		case E_STOP_STOP: {
			code = 0x50;
		}
		break;

		case BMS_TIMEOUT_STOP: {
			code = 0x52;
		}
		break;

		case TEMP_ERROR_STOP: {
			code = 0x53;
		}
		break;

		case GUN_LOCK_ERROR_STOP: {
			code = 0x55;
		}
		break;

		case INSULATION_ERR_STOP: {
			code = 0x57;
		}
		break;

		case BMS_ERROR_STOP: {
			code = 0x5f;
		}
		break;

		case MAX_I_THRESHOLD_STOP: {
			code = 0x6a;
		}
		break;

		default: {
		}
		break;
	}

	return code;
}

extern Record_Data_TypeDef Record_Data;
static int get_record_from_eeprom_by_id(uint16_t id, Channel_Record_TypeDef *record)
{
	int ret = -1;
	uint32_t addr;

	if(id >= Record_Data.Record_Num) {
		return ret;
	}

	addr = Record_Data.Record_Start_Addr + 4 + (id * sizeof(Channel_Record_TypeDef));


	Read_EEprom(addr, (uint8_t *)record, sizeof(Channel_Record_TypeDef));
	ret = 0;

	return ret;
}

static void tm_to_cp56time2a(struct tm *tm, cp56time2a_t *time)
{
	time->ms.v = tm->tm_sec * 1000;
	time->min.min = tm->tm_min;
	time->hour.hour = tm->tm_hour;
	time->day.day = tm->tm_mday;
	time->day.weedday = tm->tm_wday;
	time->month.month = tm->tm_mon + 1;
	time->year.year = tm->tm_year + 1900 - 2000;
}

static void cp56time2a_to_tm(struct tm *tm, cp56time2a_t *time)
{
	tm->tm_sec = time->ms.v / 1000;
	tm->tm_min = time->min.min;
	tm->tm_hour = time->hour.hour;
	tm->tm_mday = time->day.day;
	tm->tm_mon = time->month.month - 1;
	tm->tm_year = time->year.year + 2000 - 1900;
}

static int request_callback_transaction_record(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x37_request_transaction_record_t *sse_0x37_request_transaction_record = (sse_0x37_request_transaction_record_t *)(sse_frame_header + 1);
	struct tm tm;
	Channel_Record_TypeDef *record = os_calloc(1, sizeof(Channel_Record_TypeDef));
	uint32_t ticks = osKernelSysTick();

	OS_ASSERT(record != NULL);


	if(ticks_duration(ticks, net_client_data_ctx->transaction_record_disable_stamps) >= TRANSACTION_RECORD_DISABLE_DURATION) {
		goto finish;
	}

	if(net_client_data_ctx->transaction_record_id == 0xffff) {
		net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD].state = COMMAND_STATE_IDLE;
		goto finish;
	}

	ret = get_record_from_eeprom_by_id(net_client_data_ctx->transaction_record_id, record);

	if(ret != 0) {
		net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD].state = COMMAND_STATE_IDLE;
		goto finish;
	}

	if(Record_Check((uint8_t *)record, sizeof(Channel_Record_TypeDef) - 2) != record->Check) { //校验不对
		uint8_t AA = 1;
		Write_EEprom(Record_Data.Record_Start_Addr + 4 + (net_client_data_ctx->transaction_record_id * sizeof(Channel_Record_TypeDef)) + sizeof(Channel_Record_TypeDef) - 1, &AA, 1);
		net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD].state = COMMAND_STATE_IDLE;
		goto finish;
	}

	memcpy(sse_0x37_request_transaction_record->charge_serial_no, record->Serial_Num, sizeof(sse_0x37_request_transaction_record->charge_serial_no));

	set_device_id_bcd(sse_0x37_request_transaction_record->device_id, 7);
	sse_0x37_request_transaction_record->channel_id = get_bcd_from_u8(channel_id + 1);

	record_bcd_time_to_tm((record_bcd_time_t *)record->Charge_Start_Time, &tm);
	tm_to_cp56time2a(&tm, &sse_0x37_request_transaction_record->start_time);

	record_bcd_time_to_tm((record_bcd_time_t *)record->Charge_End_Time, &tm);
	tm_to_cp56time2a(&tm, &sse_0x37_request_transaction_record->end_time);

	sse_0x37_request_transaction_record->tip_price = net_client_data_ctx->device_billing_mode.tip;
	sse_0x37_request_transaction_record->tip_energy = sum_record_energy(record, 0x00);
	sse_0x37_request_transaction_record->tip_loss_energy = sse_0x37_request_transaction_record->tip_energy * net_client_data_ctx->device_billing_mode.loss / 100;
	sse_0x37_request_transaction_record->tip_expenses = sse_0x37_request_transaction_record->tip_price * sse_0x37_request_transaction_record->tip_energy / 100000;

	sse_0x37_request_transaction_record->peak_price = net_client_data_ctx->device_billing_mode.peak;
	sse_0x37_request_transaction_record->peak_energy = sum_record_energy(record, 0x01);
	sse_0x37_request_transaction_record->peak_loss_energy = sse_0x37_request_transaction_record->peak_energy * net_client_data_ctx->device_billing_mode.loss / 100;
	sse_0x37_request_transaction_record->peak_expenses = sse_0x37_request_transaction_record->peak_price * sse_0x37_request_transaction_record->peak_energy / 100000;

	sse_0x37_request_transaction_record->flat_price = net_client_data_ctx->device_billing_mode.flat;
	sse_0x37_request_transaction_record->flat_energy = sum_record_energy(record, 0x02);
	sse_0x37_request_transaction_record->flat_loss_energy = sse_0x37_request_transaction_record->flat_energy * net_client_data_ctx->device_billing_mode.loss / 100;
	sse_0x37_request_transaction_record->flat_expenses = sse_0x37_request_transaction_record->flat_price * sse_0x37_request_transaction_record->flat_energy / 100000;

	sse_0x37_request_transaction_record->valley_price = net_client_data_ctx->device_billing_mode.valley;
	sse_0x37_request_transaction_record->valley_energy = sum_record_energy(record, 0x03);
	sse_0x37_request_transaction_record->valley_loss_energy = sse_0x37_request_transaction_record->valley_energy * net_client_data_ctx->device_billing_mode.loss / 100;
	sse_0x37_request_transaction_record->valley_expenses = sse_0x37_request_transaction_record->valley_price * sse_0x37_request_transaction_record->valley_energy / 100000;

	sse_0x37_request_transaction_record->total_energy = record->Charge_Energy;
	sse_0x37_request_transaction_record->total_loss_energy = record->Charge_Energy * net_client_data_ctx->device_billing_mode.loss / 100;
	sse_0x37_request_transaction_record->total_expenses = record->Pay_Money;

	memcpy(sse_0x37_request_transaction_record->vin, record->VIN, sizeof(sse_0x37_request_transaction_record->vin));

	switch(record->Charge_Start_Reason) {
		case START_REMOTE: {
			sse_0x37_request_transaction_record->start_type = 0x01;
		}
		break;

		case START_CARD: {
			sse_0x37_request_transaction_record->start_type = 0x02;
		}
		break;

		case START_VIN: {
			sse_0x37_request_transaction_record->start_type = 0x05;
		}
		break;

		default: {
			sse_0x37_request_transaction_record->start_type = 0x04;
		}
		break;
	}

	tm_to_cp56time2a(local_time_to_tm(), &sse_0x37_request_transaction_record->transaction_time);
	sse_0x37_request_transaction_record->stop_reason = get_record_stop_reason(record);

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x37_request_transaction_record, sizeof(sse_0x37_request_transaction_record_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_TRANSACTION_RECORD].state = COMMAND_STATE_RESPONSE;

finish:
	os_free(record);
	return ret;
}

static int response_callback_transaction_record(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x38_response_transaction_record_t *sse_0x38_response_transaction_record = (sse_0x38_response_transaction_record_t *)(sse_frame_header + 1);
	Channel_Record_TypeDef *record = os_calloc(1, sizeof(Channel_Record_TypeDef));
	uint8_t AA = 1;
	uint32_t ticks = osKernelSysTick();

	OS_ASSERT(record != NULL);

	if(net_client_data_ctx->transaction_record_id == 0xffff) {
		debug("");
		goto finish;
	}

	ret = get_record_from_eeprom_by_id(net_client_data_ctx->transaction_record_id, record);

	if(ret != 0) {
		debug("");
		goto finish;
	}

	if(memcmp(record->Serial_Num, sse_0x38_response_transaction_record->charge_serial_no, sizeof(sse_0x38_response_transaction_record->charge_serial_no)) != 0) {
		debug("");
		goto finish;
	}

	if(sse_0x38_response_transaction_record->code != 0x00) {
		debug("");
		goto finish;
	}

	Write_EEprom(Record_Data.Record_Start_Addr + 4 + (net_client_data_ctx->transaction_record_id * sizeof(Channel_Record_TypeDef)) + sizeof(Channel_Record_TypeDef) - 1, &AA, 1);
	net_client_data_ctx->transaction_record_id = 0xffff;
	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_TRANSACTION_RECORD].state = COMMAND_STATE_IDLE;
	net_client_data_ctx->transaction_record_disable_stamps = ticks - TRANSACTION_RECORD_DISABLE_DURATION;
	ret = 0;

finish:
	os_free(record);
	return ret;
}

static int timeout_callback_transaction_record(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();
	net_client_data_ctx->transaction_record_disable_stamps = ticks;
	return ret;
}

static net_client_command_item_t net_client_command_item_transaction_record = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD,
	.request_frame = 0x37,
	.request_callback = request_callback_transaction_record,
	.response_frame = 0x38,
	.response_callback = response_callback_transaction_record,
	.timeout_callback = timeout_callback_transaction_record,
};

static int request_callback_card_data(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x43_request_card_data_t *sse_0x43_request_card_data = (sse_0x43_request_card_data_t *)(sse_frame_header + 1);

	set_device_id_bcd(sse_0x43_request_card_data->device_id, 7);

	sse_0x43_request_card_data->code = net_client_data_ctx->card_data_code;
	sse_0x43_request_card_data->reason = net_client_data_ctx->card_data_reason;

	send_frame(net_client_info, net_client_data_ctx->sse_card_data_serial, 0x00, item->request_frame, (uint8_t *)sse_0x43_request_card_data, sizeof(sse_0x43_request_card_data_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_CARD_DATA].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_card_data(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x44_response_card_data_t *sse_0x44_response_card_data = (sse_0x44_response_card_data_t *)(sse_frame_header + 1);
	uint8_t device_id[7];
	uint8_t count;
	int i;
	int j;
	card_data_t *card_data = sse_0x44_response_card_data->card_data;
	card_data_t *card_data_local = net_client_data_ctx->card_data;

	net_client_data_ctx->sse_card_data_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x44_response_card_data->device_id, 7) != 0) {
		return ret;
	}

	count = sse_0x44_response_card_data->card_number;
	net_client_data_ctx->card_data_code = 0x01;
	net_client_data_ctx->card_data_reason = 0x00;

	for(i = 0; i < count; i++) {
		uint8_t found = 0;

		for(j = 0; j < 32; j++) {
			if(card_data_local[j].physical_account == 0) {
				found = 1;
				break;
			}

			if(card_data[i].physical_account == card_data_local[j].physical_account) {
				found = 1;
				break;
			}
		}

		if(found == 1) {
			card_data_local[j] = card_data[i];
		} else {//存储空间不足
			net_client_data_ctx->card_data_code = 0x00;
			net_client_data_ctx->card_data_reason = 0x02;
			break;
		}
	}

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_CARD_DATA].state = COMMAND_STATE_REQUEST;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_card_data = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_CARD_DATA,
	.request_frame = 0x43,
	.request_callback = request_callback_card_data,
	.response_frame = 0x44,
	.response_callback = response_callback_card_data,
};

static int request_callback_set_output(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x51_request_set_output_t *sse_0x51_request_set_output = (sse_0x51_request_set_output_t *)(sse_frame_header + 1);

	set_device_id_bcd(sse_0x51_request_set_output->device_id, 7);

	sse_0x51_request_set_output->code = 0x01;

	send_frame(net_client_info, net_client_data_ctx->sse_set_output_serial, 0x00, item->request_frame, (uint8_t *)sse_0x51_request_set_output, sizeof(sse_0x51_request_set_output_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_SET_OUTPUT].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_set_output(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x52_response_set_output_t *sse_0x52_response_set_output = (sse_0x52_response_set_output_t *)(sse_frame_header + 1);
	uint8_t device_id[7];

	net_client_data_ctx->sse_set_output_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x52_response_set_output->device_id, 7) != 0) {
		return ret;
	}

	net_client_data_ctx->device_enable = sse_0x52_response_set_output->enable;
	net_client_data_ctx->max_power = sse_0x52_response_set_output->max_power;

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_SET_OUTPUT].state = COMMAND_STATE_REQUEST;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_set_output = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_SET_OUTPUT,
	.request_frame = 0x51,
	.request_callback = request_callback_set_output,
	.response_frame = 0x52,
	.response_callback = response_callback_set_output,
};

static int request_callback_set_billing_mode(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x53_request_set_billing_mode_t *sse_0x53_request_set_billing_mode = (sse_0x53_request_set_billing_mode_t *)(sse_frame_header + 1);

	set_device_id_bcd(sse_0x53_request_set_billing_mode->device_id, 7);

	sse_0x53_request_set_billing_mode->code = 0x01;

	send_frame(net_client_info, net_client_data_ctx->sse_set_billing_mode_serial, 0x00, item->request_frame, (uint8_t *)sse_0x53_request_set_billing_mode, sizeof(sse_0x53_request_set_billing_mode_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_SET_BILLING_MODE].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_set_billing_mode(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x54_response_set_billing_mode_t *sse_0x54_response_set_billing_mode = (sse_0x54_response_set_billing_mode_t *)(sse_frame_header + 1);
	device_billing_mode_t *device_billing_mode = &net_client_data_ctx->device_billing_mode;
	uint8_t device_id[7];

	net_client_data_ctx->sse_set_billing_mode_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x54_response_set_billing_mode->device_id, 7) != 0) {
		return ret;
	}

	device_billing_mode->billing_mode_id = get_u16_from_bcd_b01(sse_0x54_response_set_billing_mode->billing_mode_id[1], sse_0x54_response_set_billing_mode->billing_mode_id[0]);
	device_billing_mode->tip = sse_0x54_response_set_billing_mode->tip;
	device_billing_mode->peak = sse_0x54_response_set_billing_mode->peak;
	device_billing_mode->flat = sse_0x54_response_set_billing_mode->flat;
	device_billing_mode->valley = sse_0x54_response_set_billing_mode->valley;
	device_billing_mode->loss = sse_0x54_response_set_billing_mode->loss;
	memcpy(device_billing_mode->section, sse_0x54_response_set_billing_mode->section, sizeof(sse_0x54_response_set_billing_mode->section));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_SET_BILLING_MODE].state = COMMAND_STATE_REQUEST;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_set_billing_mode = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_SET_BILLING_MODE,
	.request_frame = 0x53,
	.request_callback = request_callback_set_billing_mode,
	.response_frame = 0x54,
	.response_callback = response_callback_set_billing_mode,
};

static int request_callback_set_time(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x55_request_set_time_t *sse_0x55_request_set_time = (sse_0x55_request_set_time_t *)(sse_frame_header + 1);

	set_device_id_bcd(sse_0x55_request_set_time->device_id, 7);

	tm_to_cp56time2a(local_time_to_tm(), &sse_0x55_request_set_time->time);

	send_frame(net_client_info, net_client_data_ctx->sse_set_time_serial, 0x00, item->request_frame, (uint8_t *)sse_0x55_request_set_time, sizeof(sse_0x55_request_set_time_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_SET_TIME].state = COMMAND_STATE_IDLE;
	return ret;
}

static uint16_t get_bcd_u16_from_u16(uint16_t u16)
{
	u_uint16_bytes_t u_uint16_bytes_bcd;
	uint8_t bcd_h = u16 / 100;
	uint8_t bcd_l = u16 % 100;
	u_uint16_bytes_bcd.s.byte0 = get_bcd_from_u8(bcd_h);
	u_uint16_bytes_bcd.s.byte1 = get_bcd_from_u8(bcd_l);

	return u_uint16_bytes_bcd.v;
}

static void set_local_time(struct tm *tm)
{
	Channel_A_Charger.Modbus_System->Data_Info.Set_time_year = get_bcd_u16_from_u16(tm->tm_year + 1900);
	Channel_A_Charger.Modbus_System->Data_Info.Set_time_month = get_bcd_u16_from_u16(tm->tm_mon + 1);
	Channel_A_Charger.Modbus_System->Data_Info.Set_time_day = get_bcd_u16_from_u16(tm->tm_mday);
	Channel_A_Charger.Modbus_System->Data_Info.Set_time_hour = get_bcd_u16_from_u16(tm->tm_hour);
	Channel_A_Charger.Modbus_System->Data_Info.Set_time_min = get_bcd_u16_from_u16(tm->tm_min);
	Channel_A_Charger.Modbus_System->Data_Info.Set_time_second = get_bcd_u16_from_u16(tm->tm_sec);
	Channel_A_Charger.Modbus_System->Data_Info.time_updata = 1;
}

static int response_callback_set_time(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x56_response_set_time_t *sse_0x56_response_set_time = (sse_0x56_response_set_time_t *)(sse_frame_header + 1);
	uint8_t device_id[7];
	struct tm tm;

	net_client_data_ctx->sse_set_time_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x56_response_set_time->device_id, 7) != 0) {
		return ret;
	}

	cp56time2a_to_tm(&tm, &sse_0x56_response_set_time->time);
	set_local_time(&tm);

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_SET_TIME].state = COMMAND_STATE_REQUEST;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_set_time = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_SET_TIME,
	.request_frame = 0x55,
	.request_callback = request_callback_set_time,
	.response_frame = 0x56,
	.response_callback = response_callback_set_time,
};

static int request_callback_reboot(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x91_request_reboot_t *sse_0x91_request_reboot = (sse_0x91_request_reboot_t *)(sse_frame_header + 1);

	set_device_id_bcd(sse_0x91_request_reboot->device_id, 7);

	sse_0x91_request_reboot->code = 0x01;
	//执行重启指令xiaofei

	send_frame(net_client_info, net_client_data_ctx->sse_reboot_serial, 0x00, item->request_frame, (uint8_t *)sse_0x91_request_reboot, sizeof(sse_0x91_request_reboot_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_REBOOT].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_reboot(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x92_response_reboot_t *sse_0x92_response_reboot = (sse_0x92_response_reboot_t *)(sse_frame_header + 1);
	uint8_t device_id[7];

	net_client_data_ctx->sse_reboot_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x92_response_reboot->device_id, 7) != 0) {
		return ret;
	}

	net_client_data_ctx->reboot = sse_0x92_response_reboot->command;

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_REBOOT].state = COMMAND_STATE_REQUEST;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_reboot = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_REBOOT,
	.request_frame = 0x91,
	.request_callback = request_callback_reboot,
	.response_frame = 0x92,
	.response_callback = response_callback_reboot,
};

static int request_callback_update(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x93_request_update_t *sse_0x93_request_update = (sse_0x93_request_update_t *)(sse_frame_header + 1);
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, net_client_data_ctx->firmware_info.stamps) >= net_client_data_ctx->firmware_info.timeout) {
		set_device_id_bcd(sse_0x93_request_update->device_id, 7);

		sse_0x93_request_update->code = 0x03;//返回超时
	} else {
		if(net_client_data_ctx->firmware_info.state != FIRMWARE_UPDATE_STATE_FINISH) {
			return ret;
		}

		set_device_id_bcd(sse_0x93_request_update->device_id, 7);

		sse_0x93_request_update->code = net_client_data_ctx->firmware_info.code;
	}

	send_frame(net_client_info, net_client_data_ctx->sse_update_serial, 0x00, item->request_frame, (uint8_t *)sse_0x93_request_update, sizeof(sse_0x93_request_update_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_UPDATE].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_update(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x94_response_update_t *sse_0x94_response_update = (sse_0x94_response_update_t *)(sse_frame_header + 1);
	uint8_t device_id[7];

	net_client_data_ctx->sse_update_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x94_response_update->device_id, 7) != 0) {
		return ret;
	}

	snprintf((char *)net_client_data_ctx->firmware_info.server, sizeof(net_client_data_ctx->firmware_info.server), "%s", sse_0x94_response_update->server);
	net_client_data_ctx->firmware_info.port = sse_0x94_response_update->port;
	snprintf((char *)net_client_data_ctx->firmware_info.user, sizeof(net_client_data_ctx->firmware_info.user), "%s", sse_0x94_response_update->user);
	snprintf((char *)net_client_data_ctx->firmware_info.password, sizeof(net_client_data_ctx->firmware_info.password), "%s", sse_0x94_response_update->password);
	snprintf((char *)net_client_data_ctx->firmware_info.path, sizeof(net_client_data_ctx->firmware_info.path), "%s", sse_0x94_response_update->path);
	net_client_data_ctx->firmware_info.command = sse_0x94_response_update->command;
	net_client_data_ctx->firmware_info.timeout = sse_0x94_response_update->timeout;
	net_client_data_ctx->firmware_info.stamps = osKernelSysTick();
	//发升级指令xiaofei

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_UPDATE].state = COMMAND_STATE_REQUEST;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_update = {
	.cmd = NET_CLIENT_COMMAND_DEVICE_UPDATE,
	.request_frame = 0x93,
	.request_callback = request_callback_update,
	.response_frame = 0x94,
	.response_callback = response_callback_update,
};

static net_client_command_item_t *net_client_command_item_device_table[] = {
	&net_client_command_item_login,
	&net_client_command_item_billing_mode,
	&net_client_command_item_billing_rules,
	&net_client_command_item_transaction_record,
	&net_client_command_item_card_data,
	&net_client_command_item_set_output,
	&net_client_command_item_set_billing_mode,
	&net_client_command_item_set_time,
	&net_client_command_item_reboot,
	&net_client_command_item_update,
};

static int request_callback_heartbeat(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x03_request_heartbeat_t *sse_0x03_request_heartbeat = (sse_0x03_request_heartbeat_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);

	set_device_id_bcd(sse_0x03_request_heartbeat->device_id, 7);
	sse_0x03_request_heartbeat->channel_id = channel_id + 1;
	sse_0x03_request_heartbeat->channel_state = (channel->Modbus_System->Data_Info.Sys_Err == SYS_ERR_OK) ? 0x00 : 0x01;//不能检测枪故障

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x03_request_heartbeat, sizeof(sse_0x03_request_heartbeat_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_HEARTBEAT].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_callback_heartbeat(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x04_response_heartbeat_t *sse_0x04_response_heartbeat = (sse_0x04_response_heartbeat_t *)(sse_frame_header + 1);
	uint8_t device_id[7];

	//常规过滤,必走
	if(get_u8_from_bcd(sse_0x04_response_heartbeat->channel_id) != (channel_id + 1)) {
		ret = -2;
		return ret;
	}

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x04_response_heartbeat->device_id, 7) == 0) {
		net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_HEARTBEAT].state = COMMAND_STATE_IDLE;
		net_client_data_ctx->channel_data_ctx[channel_id].heartbeat_timeout = 0;
		ret = 0;
	}

	return ret;
}

static void logout_callback(void);
static int timeout_callback_heartbeat(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	net_client_data_ctx->channel_data_ctx[channel_id].heartbeat_timeout++;

	if(net_client_data_ctx->channel_data_ctx[channel_id].heartbeat_timeout >= 3) {
		logout_callback();
	}

	return ret;
}

static net_client_command_item_t net_client_command_item_heartbeat = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_HEARTBEAT,
	.periodic = 10 * 1000,
	.request_frame = 0x03,
	.request_callback = request_callback_heartbeat,
	.response_frame = 0x04,
	.response_callback = response_callback_heartbeat,
	.timeout_callback = timeout_callback_heartbeat,
};

static uint16_t sse_get_hard_fault(Sys_Err_Type fault)//xiaofei bit 从0开始
{
	u_uint16_t_bits_t u_uint16_t_bits;
	u_uint16_t_bits.v = 0;

	switch(fault) {
		case SYS_ERR_E_STOP: {
			u_uint16_t_bits.s.bit1 = 1;
		}
		break;

		case SYS_ERR_GUN_TEMP_HIGH: {
			u_uint16_t_bits.s.bit3 = 1;
		}
		break;

		case SYS_ERR_POWER: {
			u_uint16_t_bits.s.bit5 = 1;
		}
		break;

		case SYS_ERR_FUN_A:
		case SYS_ERR_FUN_B:
		case SYS_ERR_FUN_C:
		case SYS_ERR_FUN_D: {
			u_uint16_t_bits.s.bit6 = 1;
		}
		break;

		case SYS_ERR_ELCMETER_A:
		case SYS_ERR_ELCMETER_B:
		case SYS_ERR_ELCMETER_C:
		case SYS_ERR_ELCMETER_D: {
			u_uint16_t_bits.s.bit7 = 1;
		}
		break;

		case SYS_ERR_CARD: {
			u_uint16_t_bits.s.bit8 = 1;
		}
		break;

		case SYS_ERR_CONTACTOR: {
			u_uint16_t_bits.s.bit12 = 1;
		}
		break;

		case SYS_ERR_DOOR: {
			u_uint16_t_bits.s.bit13 = 1;
		}
		break;

		default: {
		}
		break;
	}

	return u_uint16_t_bits.v;
}

static int request_callback_realtime_data(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x11_request_realtime_data_t *sse_0x11_request_realtime_data = (sse_0x11_request_realtime_data_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];
	device_billing_mode_t *device_billing_mode = &net_client_data_ctx->device_billing_mode;
	uint32_t ticks = osKernelSysTick();
	uint32_t delay = 15 * 1000;

	if(channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_IDLE) {
		delay = 5 * 60 * 1000;
	}

	if(ticks_duration(ticks, channel_data_ctx->realtime_data_request_stamps) < delay) {
		net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REALTIME_DATA].state = COMMAND_STATE_IDLE;
		return ret;
	}

	channel_data_ctx->realtime_data_request_stamps = ticks;

	if(channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) {
		memcpy(sse_0x11_request_realtime_data->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x11_request_realtime_data->charge_serial_no));
	}

	set_device_id_bcd(sse_0x11_request_realtime_data->device_id, 7);
	sse_0x11_request_realtime_data->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x11_request_realtime_data->state = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? 0x03 : 0x02;
	sse_0x11_request_realtime_data->available = (channel->Modbus_Channel->Status_Info.Gun_Status == 0) ? 0x01 : 0x00;
	sse_0x11_request_realtime_data->connected = (channel->Modbus_Channel->Status_Info.Gun_Status == 0) ? 0x00 : 0x01;
	sse_0x11_request_realtime_data->output_voltage = channel->Modbus_Channel->Display_Info.Charge_U;
	sse_0x11_request_realtime_data->output_current = channel->Modbus_Channel->Display_Info.Charge_I;
	sse_0x11_request_realtime_data->temperature = channel->Modbus_Channel->Status_Info.Tmp_DC_P + 30;
	sse_0x11_request_realtime_data->soc = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Modbus_Channel->Display_Info.SOC : 0;
	sse_0x11_request_realtime_data->max_temperature = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Modbus_Channel->BMS_Info.Single_Battery_Highest_Temp : 0;
	sse_0x11_request_realtime_data->total_time = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Modbus_Channel->Display_Info.Charge_Time : 0;
	sse_0x11_request_realtime_data->remain_time = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Modbus_Channel->Display_Info.Remain_Time : 0;
	sse_0x11_request_realtime_data->charged_energy = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? get_u32_from_u16_01(channel->Modbus_Channel->Display_Info.Charge_Energy_L, channel->Modbus_Channel->Display_Info.Charge_Energy_H) / 10 : 0;
	sse_0x11_request_realtime_data->loss_charged_energy = sse_0x11_request_realtime_data->charged_energy * device_billing_mode->loss / 100;
	sse_0x11_request_realtime_data->charged_expenses = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Channel_Record.Pay_Money : 0;
	sse_0x11_request_realtime_data->hard_fault = sse_get_hard_fault(channel->Modbus_System->Data_Info.Sys_Err);

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x11_request_realtime_data, sizeof(sse_0x11_request_realtime_data_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REALTIME_DATA].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_realtime_data(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x12_response_realtime_data_t *sse_0x12_response_realtime_data = (sse_0x12_response_realtime_data_t *)(sse_frame_header + 1);
	uint8_t device_id[7];

	//常规过滤,必走
	if(get_u8_from_bcd(sse_0x12_response_realtime_data->channel_id) != (channel_id + 1)) {
		ret = -2;
		return ret;
	}

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x12_response_realtime_data->device_id, 7) != 0) {
		net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REALTIME_DATA].state = COMMAND_STATE_REQUEST;
		return ret;
	}

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REALTIME_DATA].state = COMMAND_STATE_REQUEST;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_realtime_data = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_REALTIME_DATA,
	.periodic = 15 * 1000,
	.request_frame = 0x11,
	.request_callback = request_callback_realtime_data,
	.response_frame = 0x12,
	.response_callback = response_callback_realtime_data,
};

static int request_callback_offline_data(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x13_request_offline_data_t *sse_0x13_request_offline_data = (sse_0x13_request_offline_data_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];
	device_billing_mode_t *device_billing_mode = &net_client_data_ctx->device_billing_mode;
	time_t *ptime = get_time();
	time_t time = *ptime + TS_TO_CALENDAR_OFFSET;
	struct tm *tm = localtime(&time);

	if(channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) {
		memcpy(sse_0x13_request_offline_data->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x13_request_offline_data->charge_serial_no));
	}

	set_device_id_bcd(sse_0x13_request_offline_data->device_id, 7);
	sse_0x13_request_offline_data->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x13_request_offline_data->state = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? 0x03 : 0x02;
	sse_0x13_request_offline_data->pulgin = (channel->Modbus_Channel->Status_Info.Gun_Status == 0) ? 0x00 : 0x01;
	sse_0x13_request_offline_data->output_voltage = channel->Modbus_Channel->Display_Info.Charge_U;
	sse_0x13_request_offline_data->output_current = channel->Modbus_Channel->Display_Info.Charge_I;
	sse_0x13_request_offline_data->temperature = channel->Modbus_Channel->Status_Info.Tmp_DC_P + 30;
	sse_0x13_request_offline_data->soc = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Modbus_Channel->Display_Info.SOC : 0;
	sse_0x13_request_offline_data->max_temperature = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Modbus_Channel->BMS_Info.Single_Battery_Highest_Temp : 0;
	sse_0x13_request_offline_data->total_time = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Modbus_Channel->Display_Info.Charge_Time : 0;
	sse_0x13_request_offline_data->remain_time = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Modbus_Channel->Display_Info.Remain_Time : 0;
	sse_0x13_request_offline_data->charged_energy = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? get_u32_from_u16_01(channel->Modbus_Channel->Display_Info.Charge_Energy_L, channel->Modbus_Channel->Display_Info.Charge_Energy_H) / 10 : 0;
	sse_0x13_request_offline_data->loss_charged_energy = sse_0x13_request_offline_data->charged_energy * device_billing_mode->loss / 100;
	sse_0x13_request_offline_data->charged_expenses = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? channel->Channel_Record.Pay_Money : 0;
	sse_0x13_request_offline_data->hard_fault = sse_get_hard_fault(channel->Modbus_System->Data_Info.Sys_Err);
	tm_to_cp56time2a(tm, &sse_0x13_request_offline_data->time);

	ret = send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x13_request_offline_data, sizeof(sse_0x13_request_offline_data_t));

	if(ret == 0) {
		net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_OFFLINE_DATA].state = COMMAND_STATE_IDLE;
	}

	return ret;
}

static net_client_command_item_t net_client_command_item_offline_data = {//xiaofei
	.cmd = NET_CLIENT_COMMAND_CHANNEL_OFFLINE_DATA,
	.request_frame = 0x13,
	.request_callback = request_callback_offline_data,
	.response_frame = 0x14,
	.response_callback = NULL,
};

static int request_callback_bms_handshake(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x15_request_bms_handshake_t *sse_0x15_request_bms_handshake = (sse_0x15_request_bms_handshake_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	//net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	memcpy(sse_0x15_request_bms_handshake->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x15_request_bms_handshake->charge_serial_no));

	set_device_id_bcd(sse_0x15_request_bms_handshake->device_id, 7);
	sse_0x15_request_bms_handshake->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x15_request_bms_handshake->version_1 = channel->Channel_BMS_Message.pMessage_BRM->BMS_Version[0];
	sse_0x15_request_bms_handshake->version_0 = get_u16_from_u8_lh(channel->Channel_BMS_Message.pMessage_BRM->BMS_Version[1], channel->Channel_BMS_Message.pMessage_BRM->BMS_Version[2]);
	sse_0x15_request_bms_handshake->battery_type = channel->Channel_BMS_Message.pMessage_BRM->Battery_type;
	sse_0x15_request_bms_handshake->total_battery_rate_capicity = channel->Channel_BMS_Message.pMessage_BRM->Rated_Capacity;
	sse_0x15_request_bms_handshake->total_battery_rate_voltage = channel->Channel_BMS_Message.pMessage_BRM->Rated_U;
	memcpy(sse_0x15_request_bms_handshake->battery_vendor, channel->Channel_BMS_Message.pMessage_BRM->Battery_Manufacturer, sizeof(sse_0x15_request_bms_handshake->battery_vendor));
	memcpy(&sse_0x15_request_bms_handshake->battery_vendor_sn, channel->Channel_BMS_Message.pMessage_BRM->Battery_Num, sizeof(sse_0x15_request_bms_handshake->battery_vendor_sn));
	sse_0x15_request_bms_handshake->battery_year = channel->Channel_BMS_Message.pMessage_BRM->Year;
	sse_0x15_request_bms_handshake->battery_month = channel->Channel_BMS_Message.pMessage_BRM->Month;
	sse_0x15_request_bms_handshake->battery_day = channel->Channel_BMS_Message.pMessage_BRM->Day;
	memcpy(sse_0x15_request_bms_handshake->battery_charge_times, channel->Channel_BMS_Message.pMessage_BRM->Battery_Times, sizeof(sse_0x15_request_bms_handshake->battery_charge_times));
	sse_0x15_request_bms_handshake->battery_property = channel->Channel_BMS_Message.pMessage_BRM->Battery_Property;
	sse_0x15_request_bms_handshake->reserved1 = channel->Channel_BMS_Message.pMessage_BRM->Reserved;
	memcpy(sse_0x15_request_bms_handshake->vin, channel->Channel_BMS_Message.pMessage_BRM->VIN, sizeof(sse_0x15_request_bms_handshake->vin));
	memcpy(&sse_0x15_request_bms_handshake->serial, channel->Channel_BMS_Message.pMessage_BRM->Reserved_2, 8);

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x15_request_bms_handshake, sizeof(sse_0x15_request_bms_handshake_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_BMS_HANDSHAKE].state = COMMAND_STATE_IDLE;
	return ret;
}

static net_client_command_item_t net_client_command_item_bms_handshake = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_BMS_HANDSHAKE,
	.request_frame = 0x15,
	.request_callback = request_callback_bms_handshake,
	.response_frame = 0x16,
	.response_callback = NULL,
};

static int request_callback_bms_config(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x17_request_bms_config_t *sse_0x17_request_bms_config = (sse_0x17_request_bms_config_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	//net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	memcpy(sse_0x17_request_bms_config->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x17_request_bms_config->charge_serial_no));
	set_device_id_bcd(sse_0x17_request_bms_config->device_id, 7);
	sse_0x17_request_bms_config->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x17_request_bms_config->max_charge_voltage_single_battery = channel->Channel_BMS_Message.pMessage_BCP->Single_Battery_Highest_U;
	sse_0x17_request_bms_config->max_charge_current = channel->Channel_BMS_Message.pMessage_BCP->Charge_Highest_I;
	sse_0x17_request_bms_config->rate_total_power = channel->Channel_BMS_Message.pMessage_BCP->Total_Energy;
	sse_0x17_request_bms_config->max_charge_voltage = channel->Channel_BMS_Message.pMessage_BCP->Charge_Highest_U;
	sse_0x17_request_bms_config->max_temperature = channel->Channel_BMS_Message.pMessage_BCP->Highest_Temp;
	sse_0x17_request_bms_config->soc = channel->Channel_BMS_Message.pMessage_BCP->Battery_SOC;
	sse_0x17_request_bms_config->total_voltage = channel->Channel_BMS_Message.pMessage_BCP->Battery_Total_U;
	sse_0x17_request_bms_config->max_output_voltage = channel->Channel_Common_Data->BMS_Max_Output_U;
	sse_0x17_request_bms_config->min_output_voltage = channel->Channel_Common_Data->BMS_Min_Output_U;
	sse_0x17_request_bms_config->max_output_current = 4000 - channel->Channel_Common_Data->Max_Output_I;
	sse_0x17_request_bms_config->min_output_current = 4000 - channel->Channel_Common_Data->Min_Output_I;

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x17_request_bms_config, sizeof(sse_0x17_request_bms_config_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_BMS_CONFIG].state = COMMAND_STATE_IDLE;
	return ret;
}

static net_client_command_item_t net_client_command_item_bms_config = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_BMS_CONFIG,
	.request_frame = 0x17,
	.request_callback = request_callback_bms_config,
	.response_frame = 0x18,
	.response_callback = NULL,
};

static int request_callback_bms_statistic(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x19_request_bms_statistic_t *sse_0x19_request_bms_statistic = (sse_0x19_request_bms_statistic_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	//net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	memcpy(sse_0x19_request_bms_statistic->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x19_request_bms_statistic->charge_serial_no));
	set_device_id_bcd(sse_0x19_request_bms_statistic->device_id, 7);
	sse_0x19_request_bms_statistic->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x19_request_bms_statistic->soc = channel->Channel_BMS_Message.pMessage_BSD->SOC;
	sse_0x19_request_bms_statistic->single_min_voltage = channel->Channel_BMS_Message.pMessage_BSD->Min_U;
	sse_0x19_request_bms_statistic->single_max_voltage = channel->Channel_BMS_Message.pMessage_BSD->Max_U;
	sse_0x19_request_bms_statistic->battery_min_temperature = channel->Channel_BMS_Message.pMessage_BSD->Min_Temp;
	sse_0x19_request_bms_statistic->battery_max_temperature = channel->Channel_BMS_Message.pMessage_BSD->Max_Temp;

	sse_0x19_request_bms_statistic->total_charge_time = channel->Modbus_Channel->Display_Info.Charge_Time;
	sse_0x19_request_bms_statistic->total_charge_energy = get_u32_from_u16_01(channel->Modbus_Channel->Display_Info.Charge_Energy_L, channel->Modbus_Channel->Display_Info.Charge_Energy_H) / 10;
	sse_0x19_request_bms_statistic->charger_sn = channel->Channel_Res_Data.BMS_Charger_Num;

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x19_request_bms_statistic, sizeof(sse_0x19_request_bms_statistic_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_BMS_STATISTIC].state = COMMAND_STATE_IDLE;
	return ret;
}

static net_client_command_item_t net_client_command_item_bms_statistic = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_BMS_STATISTIC,
	.request_frame = 0x19,
	.request_callback = request_callback_bms_statistic,
	.response_frame = 0x1a,
	.response_callback = NULL,
};

static int request_callback_bms_error(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x1b_request_bms_error_t *sse_0x1b_request_bms_error = (sse_0x1b_request_bms_error_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	//net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	memcpy(sse_0x1b_request_bms_error->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x1b_request_bms_error->charge_serial_no));
	set_device_id_bcd(sse_0x1b_request_bms_error->device_id, 7);
	sse_0x1b_request_bms_error->channel_id = get_bcd_from_u8(channel_id + 1);

	switch(channel->Modbus_Channel->Status_Info.BMS_Status) {
		case BRM_TIMEOUT:
			sse_0x1b_request_bms_error->cem.u1.s.brm_timeout = 0x01;
			break;

		case BCP_TIMEOUT:
			sse_0x1b_request_bms_error->cem.u2.s.bcp_timeout = 0x01;
			break;

		case BRO_TIMEOUT:
			sse_0x1b_request_bms_error->cem.u2.s.bro_timeout = 0x01;
			break;

		case BRO_ERROR:
			sse_0x1b_request_bms_error->cem.u2.s.bro_timeout = 0x01;
			break;

		case BCS_TIMEOUT:
			sse_0x1b_request_bms_error->cem.u3.s.bcs_timeout = 0x01;
			break;

		case BCL_TIMEOUT:
			sse_0x1b_request_bms_error->cem.u3.s.bcl_timeout = 0x01;
			break;

		case BST_TIMEOUT:
			sse_0x1b_request_bms_error->cem.u3.s.bst_timeout = 0x01;
			break;

		case BSD_TIMEOUT:
			sse_0x1b_request_bms_error->cem.u4.s.bsd_timeout = 0x01;
			break;

		default:
			break;
	}

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x1b_request_bms_error, sizeof(sse_0x1b_request_bms_error_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_BMS_ERROR].state = COMMAND_STATE_IDLE;
	return ret;
}

static net_client_command_item_t net_client_command_item_bms_error = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_BMS_ERROR,
	.request_frame = 0x1b,
	.request_callback = request_callback_bms_error,
	.response_frame = 0x1c,
	.response_callback = NULL,
};

static int request_callback_bms_stop(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x1d_request_bms_stop_t *sse_0x1d_request_bms_stop = (sse_0x1d_request_bms_stop_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	//net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	memcpy(sse_0x1d_request_bms_stop->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x1d_request_bms_stop->charge_serial_no));
	set_device_id_bcd(sse_0x1d_request_bms_stop->device_id, 7);
	sse_0x1d_request_bms_stop->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x1d_request_bms_stop->u2.v = channel->Modbus_Channel->Status_Info.BMS_Stop_Reason;

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x1d_request_bms_stop, sizeof(sse_0x1d_request_bms_stop_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_BMS_STOP].state = COMMAND_STATE_IDLE;
	return ret;
}

static net_client_command_item_t net_client_command_item_bms_stop = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_BMS_STOP,
	.request_frame = 0x1d,
	.request_callback = request_callback_bms_stop,
	.response_frame = 0x1e,
	.response_callback = NULL,
};

static int request_callback_channel_stop(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x21_request_channel_stop_t *sse_0x21_request_channel_stop = (sse_0x21_request_channel_stop_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	//net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	memcpy(sse_0x21_request_channel_stop->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x21_request_channel_stop->charge_serial_no));
	set_device_id_bcd(sse_0x21_request_channel_stop->device_id, 7);
	sse_0x21_request_channel_stop->channel_id = get_bcd_from_u8(channel_id + 1);

	switch(channel->Modbus_Channel->Status_Info.Charger_Stop_Reason) {
		case 0: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_bms_stop = 0x01;
		}
		break;

		case PERSON_STOP: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_manual = 0x01;
		}
		break;

		case U_ERROR_STOP: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_fault = 0x01;
			sse_0x21_request_channel_stop->u3.s.stop_error_reason_voltage = 0x01;
		}
		break;

		case TEMP_ERROR_STOP: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_fault = 0x01;
			sse_0x21_request_channel_stop->u2.s.stop_fault_reason_temperature = 0x01;
		}
		break;

		case BMS_ERROR_STOP: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_fault = 0x01;
			sse_0x21_request_channel_stop->u2.s.stop_fault_reason_other = 0x01;
		}
		break;

		case CONDITION_DURATION_STOP:
		case CONDITION_NO_ENOUGH_MONEY_STOP:
		case CONDITION_MONEY_STOP:
		case CONDITION_ENERGY_STOP:
		case CONDITION_TIME_STOP:
		case CONDITION_SOC_STOP: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_condition = 0x01;
		}
		break;

		case CONNECT_ERROR_STOP: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_fault = 0x01;
			sse_0x21_request_channel_stop->u2.s.stop_fault_reason_connector = 0x01;
		}
		break;

		case E_STOP_STOP: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_manual = 0x01;
			sse_0x21_request_channel_stop->u2.s.stop_fault_reason_emergency = 0x01;
		}
		break;

		default: {
			sse_0x21_request_channel_stop->u1.s.stop_reason_fault = 0x01;
			sse_0x21_request_channel_stop->u2.s.stop_fault_reason_other = 0x01;
		}
		break;
	}

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x21_request_channel_stop, sizeof(sse_0x21_request_channel_stop_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_CHANNEL_STOP].state = COMMAND_STATE_IDLE;
	return ret;
}

static net_client_command_item_t net_client_command_item_channel_stop = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_CHANNEL_STOP,
	.request_frame = 0x21,
	.request_callback = request_callback_channel_stop,
	.response_frame = 0x22,
	.response_callback = NULL,
};

static int request_callback_bms_state(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x23_request_bms_state_t *sse_0x23_request_bms_state = (sse_0x23_request_bms_state_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	//net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	memcpy(sse_0x23_request_bms_state->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x23_request_bms_state->charge_serial_no));
	set_device_id_bcd(sse_0x23_request_bms_state->device_id, 7);
	sse_0x23_request_bms_state->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x23_request_bms_state->require_voltage = channel->Channel_BMS_Message.pMessage_BCL->Battery_Require_U;
	sse_0x23_request_bms_state->require_current = channel->Channel_BMS_Message.pMessage_BCL->Battery_Require_I;
	sse_0x23_request_bms_state->charge_mode = channel->Channel_BMS_Message.pMessage_BCL->Charge_Mode;

	sse_0x23_request_bms_state->charge_voltage = channel->Channel_BMS_Message.pMessage_BCS->Charge_U_Measure;
	sse_0x23_request_bms_state->charge_current = channel->Channel_BMS_Message.pMessage_BCS->Charge_I_Measure;
	sse_0x23_request_bms_state->u1.s.single_battery_max_voltage = channel->Channel_BMS_Message.pMessage_BCS->Max_U_And_Num.Max_U;
	sse_0x23_request_bms_state->u1.s.single_battery_max_group = channel->Channel_BMS_Message.pMessage_BCS->Max_U_And_Num.Max_U;
	sse_0x23_request_bms_state->u1.v = channel->Channel_BMS_Message.pMessage_BCS->Max_U_And_Num.Max_U;
	sse_0x23_request_bms_state->soc = channel->Channel_BMS_Message.pMessage_BCS->Battery_SOC;
	sse_0x23_request_bms_state->remain_min = channel->Channel_BMS_Message.pMessage_BCS->Remaining_Time;

	sse_0x23_request_bms_state->output_voltage = channel->Modbus_Channel->Display_Info.Charge_U;
	sse_0x23_request_bms_state->output_current = 4000 - channel->Modbus_Channel->Display_Info.Charge_I;
	sse_0x23_request_bms_state->total_charge_time = channel->Modbus_Channel->Display_Info.Charge_Time;

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x23_request_bms_state, sizeof(sse_0x23_request_bms_state_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_BMS_STATE].state = COMMAND_STATE_IDLE;
	return ret;
}

static net_client_command_item_t net_client_command_item_channel_bms_state = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_BMS_STATE,
	.periodic = 15 * 1000,
	.request_frame = 0x23,
	.request_callback = request_callback_bms_state,
	.response_frame = 0x24,
	.response_callback = NULL,
};

static int request_callback_charge_state(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x25_request_charge_state_t *sse_0x25_request_charge_state = (sse_0x25_request_charge_state_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	//net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	memcpy(sse_0x25_request_charge_state->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x25_request_charge_state->charge_serial_no));
	set_device_id_bcd(sse_0x25_request_charge_state->device_id, 7);
	sse_0x25_request_charge_state->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x25_request_charge_state->single_max_voltage_group = channel->Channel_BMS_Message.pMessage_BSM->Max_U_Num;
	sse_0x25_request_charge_state->battery_max_temperature = channel->Channel_BMS_Message.pMessage_BSM->Max_Temp;
	sse_0x25_request_charge_state->battery_max_temperature_sn = channel->Channel_BMS_Message.pMessage_BSM->Max_Temp_Num;
	sse_0x25_request_charge_state->battery_min_temperature = channel->Channel_BMS_Message.pMessage_BSM->Min_Temp;
	sse_0x25_request_charge_state->battery_min_temperature_sn = channel->Channel_BMS_Message.pMessage_BSM->Min_Temp_Num;
	sse_0x25_request_charge_state->u1.v = get_u8_l_from_u16(channel->Channel_BMS_Message.pMessage_BSM->Status);
	sse_0x25_request_charge_state->u2.v = get_u8_h_from_u16(channel->Channel_BMS_Message.pMessage_BSM->Status);

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x25_request_charge_state, sizeof(sse_0x25_request_charge_state_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_CHARGE_STATE].state = COMMAND_STATE_IDLE;
	return ret;
}

static net_client_command_item_t net_client_command_item_channel_charge_state = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_CHARGE_STATE,
	.periodic = 15 * 1000,
	.request_frame = 0x25,
	.request_callback = request_callback_charge_state,
	.response_frame = 0x26,
	.response_callback = NULL,
};

static int request_callback_request_start(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x31_request_start_t *sse_0x31_request_start = (sse_0x31_request_start_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	set_device_id_bcd(sse_0x31_request_start->device_id, 7);
	sse_0x31_request_start->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x31_request_start->start_type = channel_data_ctx->start_type;
	sse_0x31_request_start->need_password = channel_data_ctx->need_password;

	if(channel_data_ctx->start_type == KEAN_CHANNEL_START_TYPE_CARD) {
		sse_0x31_request_start->physical_account = Card_Data.Data.ID;

		mbedtls_md5_ret((const unsigned char *)st_CommonData->Tmp_PWD, strlen((char *)st_CommonData->Tmp_PWD), sse_0x31_request_start->password);

		Card_Data.Data.ID = 0;
		memset(st_CommonData->Tmp_PWD, 0x00, sizeof(st_CommonData->Tmp_PWD));
		pModBus_Data->Channel_D.Status_Info.Charger_Start_Reason = START_CARD;
	} else if(channel_data_ctx->start_type == KEAN_CHANNEL_START_TYPE_ACCOUNT) {
		memcpy(&sse_0x31_request_start->physical_account, st_CommonData->TmpUID, sizeof(sse_0x31_request_start->physical_account));//xiaofei
		mbedtls_md5_ret((const unsigned char *)st_CommonData->Tmp_PWD, strlen((char *)st_CommonData->Tmp_PWD), sse_0x31_request_start->password);

		memset(st_CommonData->TmpUID, 0x00, sizeof(st_CommonData->TmpUID));
		memset(st_CommonData->Tmp_PWD, 0x00, sizeof(st_CommonData->Tmp_PWD));
		pModBus_Data->Channel_D.Status_Info.Charger_Start_Reason = START_PASSWORD;
	} else if(channel_data_ctx->start_type == KEAN_CHANNEL_START_TYPE_VIN) {
		memcpy(sse_0x31_request_start->vin, channel->Channel_BMS_Message.pMessage_BRM->VIN, sizeof(sse_0x31_request_start->vin));
		pModBus_Data->Channel_D.Status_Info.Charger_Start_Reason = START_VIN;
	}

	send_frame(net_client_info, net_client_data_ctx->serial++, 0x00, item->request_frame, (uint8_t *)sse_0x31_request_start, sizeof(sse_0x31_request_start_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static char *get_logic_card_id_by_channel_id(uint8_t channel_id)
{
	char *card_id = NULL;

	switch(channel_id) {
		case 0: {
			card_id = (char *)&st_CommonData->ACard;
		}
		break;

		case 1: {
			card_id = (char *)&st_CommonData->BCard;
		}
		break;

		case 2: {
			card_id = (char *)&st_CommonData->CCard;
		}
		break;

		case 3: {
			card_id = (char *)&st_CommonData->DCard;
		}
		break;

		default: {
			app_panic();
		}
		break;
	}

	return card_id;
}

static Modbus_Channel_TypeDef *get_modbus_channel_by_channel_id(uint8_t channel_id)
{
	Modbus_Channel_TypeDef *modbus_channel = NULL;

	switch(channel_id) {
		case 0: {
			modbus_channel = &pModBus_Data->Channel_A;
		}
		break;

		case 1: {
			modbus_channel = &pModBus_Data->Channel_B;
		}
		break;

		case 2: {
			modbus_channel = &pModBus_Data->Channel_C;
		}
		break;

		case 3: {
			modbus_channel = &pModBus_Data->Channel_D;
		}
		break;

		default: {
			app_panic();
		}
		break;
	}

	return modbus_channel;
}

static int response_callback_request_start(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x32_response_start_t *sse_0x32_response_start = (sse_0x32_response_start_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	uint8_t device_id[7];

	//常规过滤,必走
	if(get_u8_from_bcd(sse_0x32_response_start->channel_id) != (channel_id + 1)) {
		ret = -2;
		return ret;
	}

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x32_response_start->device_id, 7) != 0) {
		return ret;
	}

	memcpy(channel->Channel_Record.Serial_Num, sse_0x32_response_start->charge_serial_no, sizeof(sse_0x32_response_start->charge_serial_no));//xiaofei 是否会被清掉
	bcd_to_ascii(get_logic_card_id_by_channel_id(channel_id), 16, sse_0x32_response_start->account, sizeof(sse_0x32_response_start->account));
	channel->Modbus_Channel->Setting_Info.Charge_Money = sse_0x32_response_start->account_balance;
	channel->Channel_Card.Card_Money = sse_0x32_response_start->account_balance;
	channel->Modbus_Channel->Display_Info.Card_Money_L = get_u16_0_from_u32(sse_0x32_response_start->account_balance);
	channel->Modbus_Channel->Display_Info.Card_Money_H = get_u16_1_from_u32(sse_0x32_response_start->account_balance);

	//启动
	if(sse_0x32_response_start->code == 0x01) {
		net_client_data_ctx->channel_data_ctx[channel_id].start_finish = 0;
		net_client_data_ctx->channel_data_ctx[channel_id].start_code = 0x00;
		net_client_data_ctx->channel_data_ctx[channel_id].start_failed_reason = 0x00;

		if(Modbus_Command_Channel_ON_OFF(channel, CHARGER_ON) != SUCCESS) {
			net_client_data_ctx->channel_data_ctx[channel_id].start_finish = 1;
			net_client_data_ctx->channel_data_ctx[channel_id].start_failed_reason = 0x03;
		}
	} else {
		switch(get_u8_from_bcd(sse_0x32_response_start->reason)) {
			case 0x01: {
				channel->Modbus_System->Data_Info.Windows = WINDOWS_TITLE_0136;
			}
			break;

			case 0x02: {
				channel->Modbus_System->Data_Info.Windows = WINDOWS_TITLE_0134;
			}
			break;

			case 0x03: {
				channel->Modbus_System->Data_Info.Windows = WINDOWS_SYS_NO_ENOUGH_MONEY;
			}
			break;

			case 0x04: {
				channel->Modbus_System->Data_Info.Windows = WINDOWS_TITLE_0134;
			}
			break;

			case 0x05: {
				channel->Modbus_System->Data_Info.Windows = WINDOWS_TITLE_0135;
			}
			break;

			case 0x06: {
				channel->Modbus_System->Data_Info.Windows = WINDOWS_TITLE_0135;
			}
			break;

			case 0x07: {
				channel->Modbus_System->Data_Info.Windows = WINDOWS_SYS_PASSWORD_ERROR;
			}
			break;

			case 0x08: {
				channel->Modbus_System->Data_Info.Windows = WINDOWS_TITLE_0134;
			}
			break;

			case 0x09: {
				Modbus_Channel_TypeDef *modbus_channel = get_modbus_channel_by_channel_id(channel_id);
				channel->Modbus_System->Data_Info.Windows = WINDOWS_TITLE_0134;

				if( (modbus_channel->Status_Info.Charger_Start_Reason == START_VIN) &&
				    (channel->Channel_Net.Net_Verify_VIN_Result == 4) ) {
					channel->Channel_Net.Net_Verify_VIN_Result = 3;//通知 VIN验证失败。
				}
			}
			break;

			default: {
				debug("");
			}
			break;
		}
	}

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].state = COMMAND_STATE_IDLE;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_channel_request_start = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_REQUEST_START,
	.request_frame = 0x31,
	.request_callback = request_callback_request_start,
	.response_frame = 0x32,
	.response_callback = response_callback_request_start,
};

static int request_callback_remote_start(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x33_request_remote_start_t *sse_0x33_request_remote_start = (sse_0x33_request_remote_start_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	if(channel_data_ctx->start_finish == 0) {//未启动完成
		return ret;
	}

	memcpy(sse_0x33_request_remote_start->charge_serial_no, channel->Channel_Record.Serial_Num, sizeof(sse_0x33_request_remote_start->charge_serial_no));
	set_device_id_bcd(sse_0x33_request_remote_start->device_id, 7);
	sse_0x33_request_remote_start->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x33_request_remote_start->code = channel_data_ctx->start_code;
	sse_0x33_request_remote_start->reason = channel_data_ctx->start_failed_reason;

	send_frame(net_client_info, net_client_data_ctx->sse_remote_start_serial, 0x00, item->request_frame, (uint8_t *)sse_0x33_request_remote_start, sizeof(sse_0x33_request_remote_start_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REMOTE_START].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_remote_start(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x34_response_remote_start_t *sse_0x34_response_remote_start = (sse_0x34_response_remote_start_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	uint8_t device_id[7];

	//常规过滤,必走
	if(get_u8_from_bcd(sse_0x34_response_remote_start->channel_id) != (channel_id + 1)) {
		ret = -2;
		return ret;
	}

	net_client_data_ctx->sse_remote_start_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x34_response_remote_start->device_id, 7) != 0) {
		return ret;
	}

	memcpy(channel->Channel_Record.Serial_Num, sse_0x34_response_remote_start->charge_serial_no, sizeof(sse_0x34_response_remote_start->charge_serial_no));
	bcd_to_ascii(get_logic_card_id_by_channel_id(channel_id), 16, sse_0x34_response_remote_start->logic_account, sizeof(sse_0x34_response_remote_start->logic_account));
	net_client_data_ctx->channel_data_ctx[channel_id].physical_account = sse_0x34_response_remote_start->physical_account;
	channel->Modbus_Channel->Setting_Info.Charge_Money = sse_0x34_response_remote_start->account_balance;
	channel->Channel_Card.Card_Money = sse_0x34_response_remote_start->account_balance;
	channel->Modbus_Channel->Display_Info.Card_Money_L = get_u16_0_from_u32(sse_0x34_response_remote_start->account_balance);
	channel->Modbus_Channel->Display_Info.Card_Money_H = get_u16_1_from_u32(sse_0x34_response_remote_start->account_balance);

	//启动
	pModBus_Data->Channel_D.Status_Info.Charger_Start_Reason = START_REMOTE;

	if(net_client_data_ctx->channel_data_ctx[channel_id].sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) {
		net_client_data_ctx->channel_data_ctx[channel_id].start_finish = 1;
		net_client_data_ctx->channel_data_ctx[channel_id].start_code = 0x00;
		net_client_data_ctx->channel_data_ctx[channel_id].start_failed_reason = 0x02;
	} else {
		net_client_data_ctx->channel_data_ctx[channel_id].start_finish = 0;
		net_client_data_ctx->channel_data_ctx[channel_id].start_code = 0x00;
		net_client_data_ctx->channel_data_ctx[channel_id].start_failed_reason = 0x00;

		if(Modbus_Command_Channel_ON_OFF(channel, CHARGER_ON) != SUCCESS) {
			net_client_data_ctx->channel_data_ctx[channel_id].start_finish = 1;
			net_client_data_ctx->channel_data_ctx[channel_id].start_failed_reason = 0x03;
		}
	}

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REMOTE_START].state = COMMAND_STATE_REQUEST;

	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_channel_remote_start = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_REMOTE_START,
	.request_frame = 0x33,
	.request_callback = request_callback_remote_start,
	.response_frame = 0x34,
	.response_callback = response_callback_remote_start,
};

static int request_callback_remote_stop(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x35_request_remote_stop_t *sse_0x35_request_remote_stop = (sse_0x35_request_remote_stop_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	set_device_id_bcd(sse_0x35_request_remote_stop->device_id, 7);
	sse_0x35_request_remote_stop->channel_id = get_bcd_from_u8(channel_id + 1);
	sse_0x35_request_remote_stop->code = 0x00;
	sse_0x35_request_remote_stop->reason = (channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) ? 0x00 : 0x02;

	if(channel_data_ctx->sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) {
		//停机
		channel->Modbus_Channel->Status_Info.Charger_Stop_Reason = REMOTE_STOP;
		Modbus_Command_Channel_ON_OFF(channel, CHARGER_OFF);
		sse_0x35_request_remote_stop->code = 0x01;
	} else {
		sse_0x35_request_remote_stop->code = 0x00;
		sse_0x35_request_remote_stop->reason = 0x02;
	}

	send_frame(net_client_info, net_client_data_ctx->sse_remote_stop_serial, 0x00, item->request_frame, (uint8_t *)sse_0x35_request_remote_stop, sizeof(sse_0x35_request_remote_stop_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REMOTE_STOP].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_remote_stop(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x36_response_remote_stop_t *sse_0x36_response_remote_stop = (sse_0x36_response_remote_stop_t *)(sse_frame_header + 1);
	//Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	uint8_t device_id[7];

	//常规过滤,必走
	if(get_u8_from_bcd(sse_0x36_response_remote_stop->channel_id) != (channel_id + 1)) {
		ret = -2;
		return ret;
	}

	net_client_data_ctx->sse_remote_stop_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x36_response_remote_stop->device_id, 7) != 0) {
		return ret;
	}

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REMOTE_STOP].state = COMMAND_STATE_REQUEST;
	ret = 0;

	return ret;
}

static net_client_command_item_t net_client_command_item_channel_remote_stop = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_REMOTE_STOP,
	.request_frame = 0x35,
	.request_callback = request_callback_remote_stop,
	.response_frame = 0x36,
	.response_callback = response_callback_remote_stop,
};

static int request_callback_update_account_balance(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x41_request_update_account_balance_t *sse_0x41_request_update_account_balance = (sse_0x41_request_update_account_balance_t *)(sse_frame_header + 1);
	//Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];

	set_device_id_bcd(sse_0x41_request_update_account_balance->device_id, 7);
	sse_0x41_request_update_account_balance->physical_account = channel_data_ctx->physical_account;
	sse_0x41_request_update_account_balance->code = channel_data_ctx->update_account_balance_code;

	send_frame(net_client_info, channel_data_ctx->sse_update_account_balance_serial, 0x00, item->request_frame, (uint8_t *)sse_0x41_request_update_account_balance, sizeof(sse_0x41_request_update_account_balance_t));

	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_UPDATE_ACCOUNT_BALANCE].state = COMMAND_STATE_IDLE;
	return ret;
}

static int response_callback_update_account_balance(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x42_response_update_account_balance_t *sse_0x42_response_update_account_balance = (sse_0x42_response_update_account_balance_t *)(sse_frame_header + 1);
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);
	net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[channel_id];
	uint8_t device_id[7];

	//常规过滤,必走
	if(get_u8_from_bcd(sse_0x42_response_update_account_balance->channel_id) != (channel_id + 1)) {
		ret = -2;
		return ret;
	}

	channel_data_ctx->sse_update_account_balance_serial = sse_frame_header->serial;

	set_device_id_bcd(device_id, 7);

	if(memcmp(device_id, sse_0x42_response_update_account_balance->device_id, 7) != 0) {
		channel_data_ctx->update_account_balance_code = 0x01;
		goto finish;
	}

	if(sse_0x42_response_update_account_balance->physical_account != channel_data_ctx->physical_account) {
		channel_data_ctx->update_account_balance_code = 0x02;
		goto finish;
	}

	channel->Modbus_Channel->Setting_Info.Charge_Money = sse_0x42_response_update_account_balance->account_balance;
	channel->Channel_Card.Card_Money = sse_0x42_response_update_account_balance->account_balance;
	channel->Modbus_Channel->Display_Info.Card_Money_L = get_u16_0_from_u32(sse_0x42_response_update_account_balance->account_balance);
	channel->Modbus_Channel->Display_Info.Card_Money_H = get_u16_1_from_u32(sse_0x42_response_update_account_balance->account_balance);
	channel_data_ctx->update_account_balance_code = 0x00;
	ret = 0;

finish:
	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_UPDATE_ACCOUNT_BALANCE].state = COMMAND_STATE_REQUEST;

	return ret;
}

static net_client_command_item_t net_client_command_item_channel_update_account_balance = {
	.cmd = NET_CLIENT_COMMAND_CHANNEL_UPDATE_ACCOUNT_BALANCE,
	.request_frame = 0x41,
	.request_callback = request_callback_update_account_balance,
	.response_frame = 0x42,
	.response_callback = response_callback_update_account_balance,
};

static net_client_command_item_t *net_client_command_item_channel_table[] = {
	&net_client_command_item_heartbeat,
	&net_client_command_item_realtime_data,
	&net_client_command_item_offline_data,
	&net_client_command_item_bms_handshake,
	&net_client_command_item_bms_config,
	&net_client_command_item_bms_statistic,
	&net_client_command_item_bms_error,
	&net_client_command_item_bms_stop,
	&net_client_command_item_channel_stop,
	&net_client_command_item_channel_bms_state,
	&net_client_command_item_channel_charge_state,
	&net_client_command_item_channel_request_start,
	&net_client_command_item_channel_remote_start,
	&net_client_command_item_channel_remote_stop,
	&net_client_command_item_channel_update_account_balance,
};

static char *get_net_client_cmd_device_des(net_client_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(NET_CLIENT_COMMAND_DEVICE_LOGIN);

		default: {
		}
		break;
	}

	return des;
}

static char *get_net_client_cmd_channel_des(net_client_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(NET_CLIENT_COMMAND_CHANNEL_HEARTBEAT);

		default: {
		}
		break;
	}

	return des;
}

static void login_callback(void)
{
	int j;
	uint32_t ticks = osKernelSysTick();

	pModBus_Data->System.Data_Info.Net_Status = 1;
	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_MODE].available = 1;
	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_MODE].state = COMMAND_STATE_REQUEST;

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_BILLING_RULES].available = 1;

	net_client_data_ctx->transaction_record_disable_stamps = ticks - TRANSACTION_RECORD_DISABLE_DURATION;

	for(j = 0; j < get_device_gun_number(); j++) {
		net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[j];
		channel_data_ctx->channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_HEARTBEAT].available = 1;
		channel_data_ctx->channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REALTIME_DATA].available = 1;
		//离线变位数据没地方存,做不了刷卡离线监测数据上传
		//channel_data_ctx->channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_OFFLINE_DATA].available = 1;//xiaofei
	}
}

static void logout_callback(void)
{
	int j;
	int i;

	pModBus_Data->System.Data_Info.Net_Status = 0;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];
		net_client_data_ctx->device_cmd_ctx[item->cmd].available = 0;
	}

	for(j = 0; j < get_device_gun_number(); j++) {
		net_client_channel_data_ctx_t *channel_data_ctx = &net_client_data_ctx->channel_data_ctx[j];

		net_client_data_ctx->channel_data_ctx[j].heartbeat_timeout = 0;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];
			channel_data_ctx->channel_cmd_ctx[item->cmd].available = 0;
		}
	}

	//允许登录
	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_LOGIN].available = 1;
	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_COMMAND_DEVICE_LOGIN].state = COMMAND_STATE_REQUEST;
}

static void request_set_lan_led_state(void *ctx, uint32_t state)
{
	if(state == 0) {
		//HAL_GPIO_WritePin(led_lan_GPIO_Port, led_lan_Pin, GPIO_PIN_RESET);
	} else {
		//HAL_GPIO_WritePin(led_lan_GPIO_Port, led_lan_Pin, GPIO_PIN_SET);
	}
}

static void request_init(void *ctx)
{
	int i;

	if(net_client_data_ctx == NULL) {
		uint8_t device_gun_number = DEVICE_GUN_NUMBER_MAX;

		OS_ASSERT(get_device_gun_number() <= device_gun_number);

		net_client_data_ctx = (net_client_data_ctx_t *)os_calloc(1, sizeof(net_client_data_ctx_t));
		OS_ASSERT(net_client_data_ctx != NULL);
		net_client_data_ctx->device_cmd_ctx = (command_status_t *)os_calloc(ARRAY_SIZE(net_client_command_item_device_table), sizeof(command_status_t));
		OS_ASSERT(net_client_data_ctx->device_cmd_ctx != NULL);

		net_client_data_ctx->channel_data_ctx = (net_client_channel_data_ctx_t *)os_calloc(device_gun_number, sizeof(net_client_channel_data_ctx_t));
		OS_ASSERT(net_client_data_ctx->channel_data_ctx != NULL);

		for(i = 0; i < device_gun_number; i++) {
			net_client_channel_data_ctx_t *net_client_channel_data_ctx = net_client_data_ctx->channel_data_ctx + i;
			net_client_channel_data_ctx->channel_cmd_ctx = (command_status_t *)os_calloc(ARRAY_SIZE(net_client_command_item_channel_table), sizeof(command_status_t));
			OS_ASSERT(net_client_channel_data_ctx->channel_cmd_ctx != NULL);
		}
	}
}


static void request_before_create_server_connect(void *ctx)
{
	debug("");
}

static void request_after_create_server_connect(void *ctx)
{
	debug("");
}

static void request_before_close_server_connect(void *ctx)
{
	debug("");

	logout_callback();
}

static void request_after_close_server_connect(void *ctx)
{
	debug("");
}

static void request_parse(void *ctx, char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)buffer;
	size_t frame_size = 0;
	sse_frame_crc_t *sse_frame_crc;

	*prequest = NULL;
	*request_size = 0;

	if(sse_frame_header->start != 0x68) {//无效包
		return;
	}

	frame_size = sse_frame_header->len + KEAN_CONST_FRAME_LEN_EXTRA;

	if(size < frame_size) {//可能有效,还要收
		*prequest = buffer;
		return;
	}

	if(max_request_size < frame_size) {//长度太大,无效包
		return;
	}

	sse_frame_crc = (sse_frame_crc_t *)(buffer + frame_size);
	sse_frame_crc -= 1;

	if(modbus_calc_crc((uint8_t *)&sse_frame_header->serial, sse_frame_header->len) != sse_frame_crc->crc) {//无效包
		return;
	}

	*prequest = buffer;
	*request_size = frame_size;
	return;
}

static void sse_response(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	int i;
	int j;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	net_client_channel_data_ctx_t *channel_data_ctx;
	uint8_t handled = 0;
	command_status_t *device_cmd_ctx;

	OS_ASSERT(net_client_data_ctx != NULL);
	OS_ASSERT(net_client_data_ctx->device_cmd_ctx != NULL);
	OS_ASSERT(net_client_data_ctx->channel_data_ctx != NULL);

	device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	channel_data_ctx = net_client_data_ctx->channel_data_ctx;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].available != 1) {
			continue;
		}

		if(item->response_frame != sse_frame_header->frame) {
			//debug("");
			continue;
		}

		handled = 1;

		net_client_data_ctx->request_retry = 0;

		if(item->response_callback == NULL) {
			debug("");
			continue;
		}

		ret = item->response_callback(net_client_info, item, 0, request, request_size, send_buffer, send_buffer_size);

		if(ret != 0) {
			debug("cmd %d(%s) response error!", item->cmd, get_net_client_cmd_device_des(item->cmd));
		} else {
			debug("cmd:%d(%s) response", item->cmd, get_net_client_cmd_device_des(item->cmd));
		}

		break;
	}

	if(handled == 1) {
		return;
	}

	for(j = 0; j < DEVICE_GUN_NUMBER_MAX; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].available != 1) {
				continue;
			}

			if(item->response_frame != sse_frame_header->frame) {
				//debug("");
				continue;
			}

			net_client_data_ctx->request_retry = 0;

			if(item->response_callback == NULL) {
				debug("");
				continue;
			}

			ret = item->response_callback(net_client_info, item, j, request, request_size, send_buffer, send_buffer_size);

			if(ret != 0) {
				if(ret == -1) {
				} else {
					debug("channel %d cmd %d(%s) response error!", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				}
			} else {
				debug("channel %d cmd:%d(%s) response", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
			}

			break;
		}
	}

	return;
}

static void request_process(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	//_hexdump("request_process", (const char *)request, request_size);

	sse_response(ctx, request, request_size, send_buffer, send_buffer_size);
}

#define RESPONSE_TIMEOUT_DURATOIN (3 * 1000)

static void sse_periodic(net_client_info_t *net_client_info)
{
	int i;
	int j;
	uint32_t ticks = osKernelSysTick();
	command_status_t *device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx;

	OS_ASSERT(net_client_data_ctx != NULL);
	OS_ASSERT(net_client_data_ctx->device_cmd_ctx != NULL);
	OS_ASSERT(net_client_data_ctx->channel_data_ctx != NULL);

	device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	channel_data_ctx = net_client_data_ctx->channel_data_ctx;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].available != 1) {
			continue;
		}

		if(device_cmd_ctx[item->cmd].state == COMMAND_STATE_RESPONSE) {
			if(ticks_duration(ticks, device_cmd_ctx[item->cmd].send_stamp) >= RESPONSE_TIMEOUT_DURATOIN) {
				net_client_data_ctx->request_retry++;
				debug("cmd %d(%s) timeout", item->cmd, get_net_client_cmd_device_des(item->cmd));

				if(item->timeout_callback != NULL) {
					item->timeout_callback(net_client_info, item, 0);
				}

				if(net_client_data_ctx->request_retry < 10) {
					device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
				} else {
					net_client_data_ctx->request_retry = 0;
					debug("reset connect!");
					set_client_state(net_client_info, CLIENT_RESET);
					break;
				}
			}
		}

		if(item->periodic == 0) {
			continue;
		}

		if(ticks_duration(ticks, device_cmd_ctx[item->cmd].stamp) >= item->periodic) {
			debug("cmd %d(%s) start", item->cmd, get_net_client_cmd_device_des(item->cmd));
			device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
			device_cmd_ctx[item->cmd].stamp = ticks;
		}
	}

	for(j = 0; j < DEVICE_GUN_NUMBER_MAX; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].available != 1) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].state == COMMAND_STATE_RESPONSE) {
				if(ticks_duration(ticks, channel_cmd_ctx[item->cmd].send_stamp) >= RESPONSE_TIMEOUT_DURATOIN) {
					net_client_data_ctx->request_retry++;
					debug("channel %d cmd %d(%s) timeout", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));

					if(item->timeout_callback != NULL) {
						item->timeout_callback(net_client_info, item, j);
					}

					if(net_client_data_ctx->request_retry < 10) {
						channel_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
					} else {
						net_client_data_ctx->request_retry = 0;
						debug("reset connect!");
						set_client_state(net_client_info, CLIENT_RESET);
						break;
					}
				}
			}

			if(item->periodic == 0) {
				continue;
			}

			if(ticks_duration(ticks, channel_cmd_ctx[item->cmd].stamp) >= item->periodic) {
				debug("channel %d cmd %d(%s) start", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				channel_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
				channel_cmd_ctx[item->cmd].stamp = ticks;
			}
		}
	}
}

static void request_process_request(net_client_info_t *net_client_info, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int i;
	int j;
	int ret;
	command_status_t *device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx;

	OS_ASSERT(net_client_data_ctx != NULL);
	OS_ASSERT(net_client_data_ctx->device_cmd_ctx != NULL);
	OS_ASSERT(net_client_data_ctx->channel_data_ctx != NULL);

	device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	channel_data_ctx = net_client_data_ctx->channel_data_ctx;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		uint32_t ticks = osKernelSysTick();

		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].state != COMMAND_STATE_REQUEST) {
			continue;
		}

		if(device_cmd_ctx[item->cmd].available != 1) {
			continue;
		}

		device_cmd_ctx[item->cmd].send_stamp = ticks;

		debug("request cmd:%d(%s)", item->cmd, get_net_client_cmd_device_des(item->cmd));

		if(item->request_callback == NULL) {
			debug("");
			continue;
		}

		memset(send_buffer, 0, send_buffer_size);

		ret = item->request_callback(net_client_info, item, 0, send_buffer, send_buffer_size);

		if(ret != 0) {
			debug("send request cmd %d(%s) error!", item->cmd, get_net_client_cmd_device_des(item->cmd));
			continue;
		}
	}

	for(j = 0; j < DEVICE_GUN_NUMBER_MAX; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			uint32_t ticks = osKernelSysTick();

			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].state != COMMAND_STATE_REQUEST) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].available != 1) {
				continue;
			}

			channel_cmd_ctx[item->cmd].send_stamp = ticks;

			debug("channel %d request cmd:%d(%s)", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));

			if(item->request_callback == NULL) {
				debug("");
				continue;
			}

			ret = item->request_callback(net_client_info, item, j, send_buffer, send_buffer_size);

			if(ret != 0) {
				debug("send channel %d request cmd %d(%s) error!", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				continue;
			}
		}
	}
}

static void handle_start_channel_vin(void)
{
	uint8_t channel_id = 0xff;

	if((pModBus_Data->Channel_A.Status_Info.Charger_Start_Reason == START_VIN) && (Channel_A_Charger.Channel_Net.Net_Verify_VIN_Result == 1)) {
		Channel_A_Charger.Channel_Net.Net_Verify_VIN_Result = 4; //防止重入
		channel_id = 0;
	} else if((pModBus_Data->Channel_B.Status_Info.Charger_Start_Reason == START_VIN) && (Channel_B_Charger.Channel_Net.Net_Verify_VIN_Result == 1)) {
		Channel_B_Charger.Channel_Net.Net_Verify_VIN_Result = 4; //防止重入
		channel_id = 1;
	} else if((pModBus_Data->Channel_C.Status_Info.Charger_Start_Reason == START_VIN) && (Channel_C_Charger.Channel_Net.Net_Verify_VIN_Result == 1)) {
		Channel_C_Charger.Channel_Net.Net_Verify_VIN_Result = 4; //防止重入
		channel_id = 2;
	} else if((pModBus_Data->Channel_D.Status_Info.Charger_Start_Reason == START_VIN) && (Channel_D_Charger.Channel_Net.Net_Verify_VIN_Result == 1)) {
		Channel_D_Charger.Channel_Net.Net_Verify_VIN_Result = 4; //防止重入
		channel_id = 3;
	}

	if(channel_id >= get_device_gun_number()) {
		return;
	}

	if(net_client_data_ctx->channel_data_ctx[channel_id].sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) {
		return;
	}

	net_client_data_ctx->channel_data_ctx[channel_id].start_type = KEAN_CHANNEL_START_TYPE_VIN;
	net_client_data_ctx->channel_data_ctx[channel_id].sse_channel_state = KEAN_CHANNEL_STATE_RUNNING;
	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].available = 1;
	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].state = COMMAND_STATE_REQUEST;
}

typedef union {
	uint8_t v[16];
	struct {
		uint8_t device_id[7];//2 桩编码 BCD 码 7 不足 7 位补 0
		uint8_t channel_id;//3 枪号 BCD 码 1
		uint8_t year;
		uint8_t month;
		uint8_t day;
		uint8_t hour;
		uint8_t min;
		uint8_t second;
		uint8_t serial_l;
		uint8_t serial_h;
	} s;
} u_charge_serial_no_t;

static void try_card_start_local(uint8_t channel_id)
{
	card_data_t *card_data_local = net_client_data_ctx->card_data;
	int i;
	uint8_t found = 0;
	u_charge_serial_no_t *u_charge_serial_no;
	struct tm *tm = local_time_to_tm();
	static uint16_t serial = 0;
	Channel_TypeDef *channel = get_charger_channel_by_gun_number(channel_id + 1);

	for(i = 0; i < 32; i++) {
		if(card_data_local[i].physical_account == Card_Data.Data.ID) {
			found = 1;
			break;
		}
	}

	if(found == 0) {
		return;
	}

	//生成订单号
	u_charge_serial_no = (u_charge_serial_no_t *)channel->Channel_Record.Serial_Num;
	set_device_id_bcd(u_charge_serial_no->s.device_id, 7);
	u_charge_serial_no->s.channel_id = get_bcd_from_u8(channel_id + 1);
	u_charge_serial_no->s.year = get_bcd_from_u8(tm->tm_year + 1900 - 2000);
	u_charge_serial_no->s.month = get_bcd_from_u8(tm->tm_mon + 1);
	u_charge_serial_no->s.day = get_bcd_from_u8(tm->tm_mday);
	u_charge_serial_no->s.hour = get_bcd_from_u8(tm->tm_hour);
	u_charge_serial_no->s.min = get_bcd_from_u8(tm->tm_min);
	u_charge_serial_no->s.second = get_bcd_from_u8(tm->tm_sec);
	u_charge_serial_no->s.serial_l = get_u8_l_from_u16(get_bcd_u16_from_u16(serial));
	u_charge_serial_no->s.serial_h = get_u8_h_from_u16(get_bcd_u16_from_u16(serial));

	bcd_to_ascii(get_logic_card_id_by_channel_id(channel_id), 16, card_data_local[i].logic_account, sizeof(card_data_local[i].logic_account));
	channel->Modbus_Channel->Setting_Info.Charge_Money = 0xffff;
	channel->Channel_Card.Card_Money = 0xffff;
	channel->Modbus_Channel->Display_Info.Card_Money_L = get_u16_0_from_u32(0xffff);
	channel->Modbus_Channel->Display_Info.Card_Money_H = get_u16_1_from_u32(0xffff);

	serial++;

	if(serial >= 9999) {
		serial = 0;
	}

	Card_Data.Data.ID = 0;
	memset(st_CommonData->Tmp_PWD, 0x00, sizeof(st_CommonData->Tmp_PWD));
	pModBus_Data->Channel_D.Status_Info.Charger_Start_Reason = START_CARD;

	if(Modbus_Command_Channel_ON_OFF(channel, CHARGER_ON) != SUCCESS) {
	}
}

static void handle_start_channel_card(net_client_info_t *net_client_info)
{
	uint8_t channel_id = 0xff;
	int i;

	for(i  = 0; i < get_device_gun_number(); i++) {
		if(net_client_data_ctx->channel_data_ctx[i].sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) {
			return;
		}
	}

	if(st_CommonData->UI_DownMessageIndex == 1) {
		st_CommonData->UI_DownMessageIndex = 0;

		if(Card_Data.status.password_state == 1) {
			Card_Data.status.password_state = 0;

			Channel_A_Charger.Modbus_System->Data_Info.Windows = WINDOWS_NONE;
			Card_Data.Data.IfCardID = 0;

			switch(Card_Data.Data.GunIndex) {
				case 1: {
					channel_id = 0;
				}
				break;

				case 2: {
					channel_id = 1;
				}
				break;

				case 3: {
					channel_id = 2;
				}
				break;

				case 4: {
					channel_id = 3;
				}
				break;

				default: {
				}
				break;
			}
		}
	}

	if(channel_id >= get_device_gun_number()) {
		return;
	}

	if(net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].state != COMMAND_STATE_IDLE) {
		return;
	}

	net_client_data_ctx->channel_data_ctx[channel_id].start_type = KEAN_CHANNEL_START_TYPE_CARD;
	net_client_data_ctx->channel_data_ctx[channel_id].sse_channel_state = KEAN_CHANNEL_STATE_RUNNING;

	if(get_client_state(net_client_info) == CLIENT_CONNECTED) {
		net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].available = 1;
		net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].state = COMMAND_STATE_REQUEST;
	} else {
		try_card_start_local(channel_id);
	}
}

static void handle_start_channel_account(void)
{
	uint8_t channel_id = 0xff;

	if(st_CommonData->UI_DownMessageIndex == 61) {
		st_CommonData->UI_DownMessageIndex = 0;
		memset(st_CommonData->TmpUID, 0x00, sizeof(st_CommonData->TmpUID));
		memset(st_CommonData->Tmp_PWD, 0x00, sizeof(st_CommonData->Tmp_PWD));
		Card_Data.Data.GunIndex = 1;
		return;
	} else if(st_CommonData->UI_DownMessageIndex == 62) {
		st_CommonData->UI_DownMessageIndex = 0;
		memset(st_CommonData->TmpUID, 0x00, sizeof(st_CommonData->TmpUID));
		memset(st_CommonData->Tmp_PWD, 0x00, sizeof(st_CommonData->Tmp_PWD));
		Card_Data.Data.GunIndex = 2;
		return;
	} else if(st_CommonData->UI_DownMessageIndex == 63) {
		st_CommonData->UI_DownMessageIndex = 0;
		memset(st_CommonData->TmpUID, 0x00, sizeof(st_CommonData->TmpUID));
		memset(st_CommonData->Tmp_PWD, 0x00, sizeof(st_CommonData->Tmp_PWD));
		Card_Data.Data.GunIndex = 3;
		return;
	} else if(st_CommonData->UI_DownMessageIndex == 64) {
		st_CommonData->UI_DownMessageIndex = 0;
		memset(st_CommonData->TmpUID, 0x00, sizeof(st_CommonData->TmpUID));
		memset(st_CommonData->Tmp_PWD, 0x00, sizeof(st_CommonData->Tmp_PWD));
		Card_Data.Data.GunIndex = 4;
		return;
	}

	if(st_CommonData->UI_DownMessageIndex == 65) {
		st_CommonData->UI_DownMessageIndex = 0;

		switch(Card_Data.Data.GunIndex) {
			case 1: {
				channel_id = 0;
			}
			break;

			case 2: {
				channel_id = 1;
			}
			break;

			case 3: {
				channel_id = 2;
			}
			break;

			case 4: {
				channel_id = 3;
			}
			break;

			default: {
			}
			break;
		}
	}

	if(channel_id >= get_device_gun_number()) {
		return;
	}

	if(net_client_data_ctx->channel_data_ctx[channel_id].sse_channel_state == KEAN_CHANNEL_STATE_RUNNING) {
		return;
	}

	net_client_data_ctx->channel_data_ctx[channel_id].start_type = KEAN_CHANNEL_START_TYPE_ACCOUNT;
	net_client_data_ctx->channel_data_ctx[channel_id].sse_channel_state = KEAN_CHANNEL_STATE_RUNNING;
	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].available = 1;
	net_client_data_ctx->channel_data_ctx[channel_id].channel_cmd_ctx[NET_CLIENT_COMMAND_CHANNEL_REQUEST_START].state = COMMAND_STATE_REQUEST;
}

static void handle_start_channel(net_client_info_t *net_client_info)
{
	handle_start_channel_vin();
	handle_start_channel_card(net_client_info);
	handle_start_channel_account();
}

static void update_channel_bms_step(void)
{
	int i;

	for(i = 0; i < get_device_gun_number(); i++) {
		Channel_TypeDef *channel = get_charger_channel_by_gun_number(i + 1);

		if(channel->Channel_Res_Data.BMS_Step != net_client_data_ctx->channel_data_ctx[i].bms_step) {
			channel_callback_event_t event = CHANNEL_CALLBACK_EVENT_NONE;

			switch(channel->Channel_Res_Data.BMS_Step) {
				case COM_CONNECT: {
					event = CHANNEL_CALLBACK_EVENT_BMS_CONNECT;
				}
				break;

				case COM_SHAKE_HAND: {
					event = CHANNEL_CALLBACK_EVENT_BMS_SHAKE_HAND;
				}
				break;

				case COM_CONFIG: {
					event = CHANNEL_CALLBACK_EVENT_BMS_CONFIG;
				}
				break;

				case COM_CHARGE: {
					event = CHANNEL_CALLBACK_EVENT_BMS_CHARGE;
				}
				break;

				case COM_END: {
					event = CHANNEL_CALLBACK_EVENT_BMS_END;
				}
				break;

				case COM_NONE: {
					event = CHANNEL_CALLBACK_EVENT_BMS_NONE;
				}
				break;

				default: {
				}
				break;
			}

			sse_channel_callback(channel, event);
			net_client_data_ctx->channel_data_ctx[i].bms_step = channel->Channel_Res_Data.BMS_Step;
		}
	}
}

static void request_periodic(void *ctx, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	uint32_t ticks = osKernelSysTick();
	static uint32_t send_stamp = 0;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(ticks_duration(ticks, send_stamp) < 100) {
		return;
	}

	send_stamp = ticks;

	handle_start_channel(net_client_info);
	update_channel_bms_step();

	if(get_client_state(net_client_info) != CLIENT_CONNECTED) {
		//debug("");
		return;
	}

	sync_transaction_record();
	sse_periodic(net_client_info);
	request_process_request(net_client_info, send_buffer, send_buffer_size);
}

request_callback_t request_callback_sse = {
	.type = REQUEST_TYPE_DEFAULT_SSE,
	.set_lan_led_state = request_set_lan_led_state,
	.init = request_init,
	.before_connect = request_before_create_server_connect,
	.after_connect = request_after_create_server_connect,
	.before_close = request_before_close_server_connect,
	.after_close = request_after_close_server_connect,
	.parse = request_parse,
	.process = request_process,
	.periodic = request_periodic,
};
