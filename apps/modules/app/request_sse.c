

/*================================================================
 *
 *
 *   文件名称：request_sse.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月27日 星期四 13时09分48秒
 *   修改日期：2021年06月07日 星期一 14时23分09秒
 *   描    述：
 *
 *================================================================*/
#include <string.h>
#include <ctype.h>

#include "net_client.h"
#include "modbus_spec.h"
#include "command_status.h"
#include "channels.h"
#include "charger.h"
#include "channels_power_module.h"
#include "iap.h"

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
} u_sse_device_state_t;

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
	uint8_t charger_lock_state : 1;
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
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_NONE = 0,
	SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SOC_ARCHIEVED,
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
} sse_report_channel_charge_info_dc_t;

//ac
typedef struct {
	uint16_t output_voltage;
	uint16_t output_current;
	uint32_t telemeter_total;
	uint32_t charge_energy;
	uint32_t charge_amount;
} sse_report_channel_charge_info_ac_t;

typedef struct {
	uint8_t channel_id;//从1开始
	uint8_t channel_stop_reason;//0:充电机未主动停机 1:人工停机 2:电池温度超限停机 3:超过最大允许充电电压停机
	u_sse_channel_device_state_t u_sse_channel_device_state;
	uint8_t channel_work_state;//0:待机 1:枪已插好 2:正在充电 4:枪已上锁
	uint8_t sse_report_channel_charger_bms_state;//sse_report_channel_charger_bms_state_t
	uint8_t sse_report_channel_charger_bms_stop_reason;//sse_report_channel_charger_bms_stop_reason_t
	sse_report_channel_charge_info_dc_t sse_report_channel_charge_info_dc[0];
	sse_report_channel_charge_info_ac_t sse_report_channel_charge_info_ac[0];
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
	uint32_t float_percision;//字节数顺序描述 V24 3:电表3位小数,2:电表2位小数
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
	uint32_t start_telemeter_total;//0.01kWh <=V23 0.0001kWh V24
	uint32_t withholding;//告诉后台此卡的预扣款是多少 0.01 元
	uint8_t stop_reason;//sse_record_stop_reason_t;
	uint8_t stop_dt[8];//bcd 20161223135922ff
	uint32_t end_telemeter_total;//0.01kWh <=V23 0.0001kWh V24
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
	SSE_QUERY_CARD_ERROR_REASON_PASSWORD = 0,
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
	channels_info_t *channels_info;
	uint8_t request_timeout;
	uint16_t serial;
	uint16_t transaction_record_id;

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

typedef int (*net_client_request_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_response_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_timeout_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id);

typedef struct {
	uint8_t cmd;
	uint32_t periodic;
	uint16_t frame;
	net_client_request_callback_t request_callback;
	net_client_response_callback_t response_callback;
	net_client_timeout_callback_t timeout_callback;
} net_client_command_item_t;

static net_client_data_ctx_t *net_client_data_ctx = NULL;

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
	sse_frame_crc->crc = sum_crc8((uint8_t *)sse_frame_header, sse_frame_header->frame_len - SSE_CONST_FRAME_CRC_LEN);

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
	//uint8_t AA = 0;
	//int i;
	//int channel_id;

	if(net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD].state != COMMAND_STATE_IDLE) {
		return;
	}

	//find record to upload
	//report_transaction_record(record_id);
}

static uint8_t get_telemeter_faults(channels_info_t *channels_info)
{
	int i;
	uint8_t fault = 0;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(get_fault(channel_info->faults, CHANNEL_FAULT_TELEMETER) == 1) {
			fault = 1;
			break;
		}
	}

	return fault;
}

static uint8_t get_channels_faults(channels_info_t *channels_info)
{
	int i;

	if(test_fault(channels_info->faults) == -1) {
		return 1;
	}

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(test_fault(channel_info->faults) == -1) {
			return 1;
		}
	}

	return 0;
}

static uint16_t get_device_state(channels_info_t *channels_info)
{
	u_sse_device_state_t device_state;
	u_uint16_bytes_t *u_uint16_bytes = (u_uint16_bytes_t *)&device_state;
	uint8_t connects = 0;
	int i;
	device_state.v = 0;
	device_state.s.insulation = get_fault(channels_info->faults, CHANNELS_FAULT_INSULATION);
	device_state.s.telemeter = get_telemeter_faults(channels_info);;
	device_state.s.card_reader = get_fault(channels_info->faults, CHANNELS_FAULT_CARD_READER);
	device_state.s.display = get_fault(channels_info->faults, CHANNELS_FAULT_DISPLAY);
	device_state.s.fault = get_channels_faults(channels_info);

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;
		charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;

		connects = set_u8_bits(connects, i, charger_info->connect_state);
	}

	u_uint16_bytes->s.byte1 = connects;

	return device_state.v;
}

static uint16_t get_sse_device_fault_type(channels_info_t *channels_info)
{
	uint16_t fault = SSE_DEVICE_FAULT_TYPE_NONE;

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

static uint32_t get_sse_module_state_value(channels_info_t *channels_info)
{
	uint32_t state = 0;
	//todo
	return state;
}

static uint8_t get_channel_stop_reason(channel_info_t *channel_info)
{
	uint8_t stop_reason = 0;

	switch(channel_info->channel_record_item.state) {
		case CHANNEL_RECORD_ITEM_STATE_FINISH:
		case CHANNEL_RECORD_ITEM_STATE_UPLOAD: {
			switch(channel_info->channel_record_item.stop_reason) {
				case CHANNEL_RECORD_ITEM_STOP_REASON_MANUAL: {
					stop_reason = 1;
				}
				break;

				case CHANNEL_RECORD_ITEM_STOP_REASON_TEMPERATURE_LIMIT: {
					stop_reason = 2;
				}
				break;

				case CHANNEL_RECORD_ITEM_STOP_REASON_VOLTAGE_LIMIT: {
					stop_reason = 3;
				}
				break;

				default: {
				}
				break;
			}
		}
		break;

		default: {
		}
		break;
	}

	return stop_reason;
}

static uint8_t get_channel_device_state(channel_info_t *channel_info)
{
	u_sse_channel_device_state_t u_sse_channel_device_state;
	u_sse_channel_device_state.v = 0;

	u_sse_channel_device_state.s.connect = channel_info->charger_connect_state;
	u_sse_channel_device_state.s.auxiliary_power_12 = (channel_info->channel_settings.auxiliary_power_type == AUXILIARY_POWER_TYPE_12) ? 1 : 0;
	u_sse_channel_device_state.s.auxiliary_power_24 = (channel_info->channel_settings.auxiliary_power_type == AUXILIARY_POWER_TYPE_24) ? 1 : 0;
	u_sse_channel_device_state.s.vehicle_relay_state = channel_info->vehicle_relay_state;
	u_sse_channel_device_state.s.charger_lock_state = channel_info->charger_lock_state;

	return u_sse_channel_device_state.v;
}

static uint8_t get_sse_report_channel_charger_bms_state(channel_info_t *channel_info)
{
	uint8_t state = SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_NONE;
	//todo
	return state;
}

static uint8_t get_sse_report_channel_charger_bms_stop_reason(channel_info_t *channel_info)
{
	uint8_t stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_NONE;
	//todo
	return stop_reason;
}

static void udpate_sse_chennel_charger_report(sse_channel_report_t *sse_channel_report, channel_info_t *channel_info)
{
	if(channel_info->channel_config->channel_type == CHANNEL_TYPE_DC) {
		sse_report_channel_charge_info_dc_t *sse_report_channel_charge_info_dc = sse_channel_report->sse_report_channel_charge_info_dc;
		charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;
		sse_report_channel_charge_info_dc->soc = 
	} else if(channel_info->channel_config->channel_type == CHANNEL_TYPE_AC) {
		sse_report_channel_charge_info_ac_t *sse_report_channel_charge_info_ac = sse_channel_report->sse_report_channel_charge_info_ac;
	}
}

static void udpate_sse_chennel_report(sse_channel_report_t *sse_channel_report, uint8_t channel_id)
{
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	channel_info_t *channel_info = channels_info->channel_info + channel_id;
	sse_channel_report = sse_channel_report + channel_id;

	sse_channel_report->channel_id = channel_id + 1;
	sse_channel_report->channel_stop_reason = get_channel_stop_reason(channel_info);
	sse_channel_report->u_sse_channel_device_state.v = get_channel_device_state(channel_info);
	sse_channel_report->channel_work_state = (channel_info->state == CHANNEL_STATE_STARTING) ? 1 : (channel_info->state == CHANNEL_STATE_CHARGING) ? 2 : 0;
	sse_channel_report->sse_report_channel_charger_bms_state = get_sse_report_channel_charger_bms_state(channel_info);
	sse_channel_report->sse_report_channel_charger_bms_stop_reason = get_sse_report_channel_charger_bms_stop_reason(channel_info);

	udpate_sse_chennel_charger_report(sse_channel_report, channel_info);
}

static int request_callback_report(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x00_request_report_t *sse_0x00_request_report = (sse_0x00_request_report_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	time_t ts = get_time();
	struct tm *tm = localtime(&ts);
	char dt[20];
	int i;
	uint8_t *channel_report_start;

	snprintf((char *)sse_0x00_request_report->device_id, 32, "%s", channels_settings->device_id);
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
	sse_0x00_request_report->float_percision = (channels_info->channels_settings.magnification == 0) ? 2 : 3;
	sse_0x00_request_report->channel_number = channels_info->channel_number;

	channel_report_start = (uint8_t *)sse_0x00_request_report->sse_channel_report;

	for(i = 0; i < channels_info->channel_number; i++) {
		sse_channel_report_t *sse_channel_report = (sse_channel_report_t *)channel_report_start;
		channel_info_t *channel_info = channels_info->channel_info + channel_id;
		udpate_sse_chennel_report(sse_channel_report, i);

		if(channel_info->channel_config->channel_type == CHANNEL_TYPE_DC) {
			channel_report_start += (sizeof(sse_channel_report_t) + sizeof(sse_report_channel_charge_info_dc_t));
		} else if(channel_info->channel_config->channel_type == CHANNEL_TYPE_AC) {
			channel_report_start += (sizeof(sse_channel_report_t) + sizeof(sse_report_channel_charge_info_ac_t));
		} else {
			app_panic();
		}
	}

	send_frame(net_client_info, net_client_data_ctx->serial++, item->frame, 0, (uint8_t *)sse_0x00_request_report, sizeof(sse_0x00_request_report_t) + channels_info->channel_number * sizeof(sse_channel_report_t));

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_REPORT].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_callback_report(net_client_info_t *net_client_info, void *_command_item, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x00_response_report_t *sse_0x00_response_report = (sse_0x00_response_report_t *)(sse_frame_header + 1);

	net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_REPORT].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int timeout_callback_report(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	return ret;
}

static net_client_command_item_t net_client_command_item_report = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_REPORT,
	.frame = 0x00,
	.request_callback = request_callback_report,
	.response_callback = response_callback_report,
	.timeout_callback = timeout_callback_report,
};

static net_client_command_item_t *net_client_command_item_device_table[] = {
	&net_client_command_item_report,
};


static net_client_command_item_t *net_client_command_item_channel_table[] = {
};

static char *get_net_client_cmd_device_des(net_client_device_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(NET_CLIENT_DEVICE_COMMAND_REPORT);

		default: {
		}
		break;
	}

	return des;
}

static char *get_net_client_cmd_channel_des(net_client_channel_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {

		default: {
		}
		break;
	}

	return des;
}

static void request_init(void *ctx)
{
	int i;

	if(net_client_data_ctx == NULL) {
		net_client_data_ctx = (net_client_data_ctx_t *)os_calloc(1, sizeof(net_client_data_ctx_t));
		OS_ASSERT(net_client_data_ctx != NULL);

		net_client_data_ctx->channels_info = start_channels();
		OS_ASSERT(net_client_data_ctx->channels_info != NULL);

		net_client_data_ctx->device_cmd_ctx = (command_status_t *)os_calloc(ARRAY_SIZE(net_client_command_item_device_table), sizeof(command_status_t));
		OS_ASSERT(net_client_data_ctx->device_cmd_ctx != NULL);

		net_client_data_ctx->channel_data_ctx = (net_client_channel_data_ctx_t *)os_calloc(net_client_data_ctx->channels_info->channel_number, sizeof(net_client_channel_data_ctx_t));
		OS_ASSERT(net_client_data_ctx->channel_data_ctx != NULL);

		for(i = 0; i < net_client_data_ctx->channels_info->channel_number; i++) {
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
}

static void request_after_close_server_connect(void *ctx)
{
	debug("");
}

static void request_parse(void *ctx, char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *request_size)
{
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)buffer;
	sse_frame_crc_t *sse_frame_crc;

	*prequest = NULL;
	*request_size = 0;

	if(sse_frame_header->magic != 0xf5aa) {//无效包
		return;
	}

	if(max_request_size < sse_frame_header->frame_len) {//长度太大,无效包
		return;
	}

	if(size < sse_frame_header->frame_len) {//可能有效,还要收
		*prequest = buffer;
		return;
	}

	sse_frame_crc = (sse_frame_crc_t *)(buffer + sse_frame_header->frame_len);
	sse_frame_crc -= 1;

	if(sum_crc8((uint8_t *)sse_frame_header, sse_frame_header->frame_len - SSE_CONST_FRAME_CRC_LEN) != sse_frame_crc->crc) {//无效包
		return;
	}

	*prequest = buffer;
	*request_size = sse_frame_header->frame_len;
	return;
}

static void sse_response(void *ctx, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	int i;
	int j;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	uint8_t handled = 0;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].available == 1) {
			continue;
		}

		if(item->frame != sse_frame_header->cmd.cmd) {
			continue;
		}

		net_client_data_ctx->request_timeout = 0;

		if(item->response_callback == NULL) {
			debug("");
			continue;
		}

		ret = item->response_callback(net_client_info, item, request, request_size, send_buffer, send_buffer_size);

		if(ret != 0) {
			if(ret == 1) {
			} else {
				debug("device cmd %d(%s) response error!", item->cmd, get_net_client_cmd_channel_des(item->cmd));
			}
		} else {
			debug("device cmd:%d(%s) response", item->cmd, get_net_client_cmd_device_des(item->cmd));
			handled = 1;

		}
	}

	if(handled == 1) {
		return;
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].available == 1) {
				continue;
			}

			if(item->frame != sse_frame_header->cmd.cmd) {
				continue;
			}

			net_client_data_ctx->request_timeout = 0;

			if(item->response_callback == NULL) {
				debug("");
				continue;
			}

			ret = item->response_callback(net_client_info, item, request, request_size, send_buffer, send_buffer_size);

			if(ret != 0) {
				if(ret == 1) {
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
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].state == COMMAND_STATE_RESPONSE) {
			if(ticks_duration(ticks, device_cmd_ctx[item->cmd].send_stamp) >= RESPONSE_TIMEOUT_DURATOIN) {
				net_client_data_ctx->request_timeout++;
				debug("device cmd %d(%s) timeout", item->cmd, get_net_client_cmd_device_des(item->cmd));
				device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;

				if(item->timeout_callback != NULL) {
					item->timeout_callback(net_client_info, item, 0);
				}
			}
		}

		if(item->periodic == 0) {
			continue;
		}

		if(device_cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		if(ticks_duration(ticks, device_cmd_ctx[item->cmd].stamp) >= item->periodic) {
			debug("device cmd %d(%s) start", item->cmd, get_net_client_cmd_device_des(item->cmd));
			device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
			device_cmd_ctx[item->cmd].stamp = ticks;
		}
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].state == COMMAND_STATE_RESPONSE) {
				if(ticks_duration(ticks, channel_cmd_ctx[item->cmd].send_stamp) >= RESPONSE_TIMEOUT_DURATOIN) {
					net_client_data_ctx->request_timeout++;
					debug("channel %d cmd %d(%s) timeout", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
					channel_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;

					if(item->timeout_callback != NULL) {
						item->timeout_callback(net_client_info, item, j);
					}
				}
			}

			if(item->periodic == 0) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].available == 0) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].state != COMMAND_STATE_IDLE) {
				continue;
			}

			if(ticks_duration(ticks, channel_cmd_ctx[item->cmd].stamp) >= item->periodic) {
				debug("channel %d cmd %d(%s) start", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
				channel_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
				channel_cmd_ctx[item->cmd].stamp = ticks;
			}
		}
	}

	if(net_client_data_ctx->request_timeout >= 10) {
		net_client_data_ctx->request_timeout = 0;
		debug("reset connect!");
		set_client_state(net_client_info, CLIENT_RESET);
	}
}

static void request_process_request(net_client_info_t *net_client_info, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int i;
	int j;
	int ret;
	command_status_t *device_cmd_ctx = net_client_data_ctx->device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;

	for(i = 0; i < ARRAY_SIZE(net_client_command_item_device_table); i++) {
		uint32_t ticks = osKernelSysTick();

		net_client_command_item_t *item = net_client_command_item_device_table[i];

		if(device_cmd_ctx[item->cmd].state != COMMAND_STATE_REQUEST) {
			continue;
		}

		if(device_cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		device_cmd_ctx[item->cmd].send_stamp = ticks;

		debug("request device cmd:%d(%s)", item->cmd, get_net_client_cmd_device_des(item->cmd));

		if(item->request_callback == NULL) {
			debug("");
			continue;
		}

		memset(send_buffer, 0, send_buffer_size);

		ret = item->request_callback(net_client_info, item, 0, send_buffer, send_buffer_size);

		if(ret != 0) {
			debug("send device request cmd %d(%s) error!", item->cmd, get_net_client_cmd_device_des(item->cmd));
			continue;
		}
	}

	for(j = 0; j < channels_info->channel_number; j++) {
		command_status_t *channel_cmd_ctx = channel_data_ctx[j].channel_cmd_ctx;

		for(i = 0; i < ARRAY_SIZE(net_client_command_item_channel_table); i++) {
			uint32_t ticks = osKernelSysTick();

			net_client_command_item_t *item = net_client_command_item_channel_table[i];

			if(channel_cmd_ctx[item->cmd].state != COMMAND_STATE_REQUEST) {
				continue;
			}

			if(channel_cmd_ctx[item->cmd].available == 0) {
				continue;
			}

			channel_cmd_ctx[item->cmd].send_stamp = ticks;

			debug("request channel %d cmd:%d(%s)", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));

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

static void request_periodic(void *ctx, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	uint32_t ticks = osKernelSysTick();
	static uint32_t send_stamp = 0;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(ticks_duration(ticks, send_stamp) < 100) {
		return;
	}

	send_stamp = ticks;

	if(get_client_state(net_client_info) != CLIENT_CONNECTED) {
		//debug("");
		return;
	}

	sse_periodic(net_client_info);
	request_process_request(net_client_info, send_buffer, send_buffer_size);
}

request_callback_t request_callback_sse = {
	.type = REQUEST_TYPE_SSE,
	.init = request_init,
	.before_connect = request_before_create_server_connect,
	.after_connect = request_after_create_server_connect,
	.before_close = request_before_close_server_connect,
	.after_close = request_after_close_server_connect,
	.parse = request_parse,
	.process = request_process,
	.periodic = request_periodic,
};
