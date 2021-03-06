

/*================================================================
 *
 *
 *   文件名称：request_sse.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月27日 星期四 13时09分48秒
 *   修改日期：2021年07月14日 星期三 16时43分02秒
 *   描    述：
 *
 *================================================================*/
#include <string.h>
#include <ctype.h>

#include "net_client.h"
#include "modbus_spec.h"
#include "command_status.h"
#include "channels.h"
#include "channel.h"
#include "charger.h"
#include "channels_power_module.h"
#include "iap.h"
#include "app.h"

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
	uint32_t stop_telemeter_total;//0.01kWh <=V23 0.0001kWh V24
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
	uint16_t stop_hour_min;//BCD 码
	uint32_t price;
} sse_query_price_item_info_t;

typedef struct {
	uint8_t count;
	sse_query_price_item_info_t item[0];
} sse_query_price_info_t;

typedef struct {
	uint16_t start_hour_min;//BCD 码
	uint16_t stop_hour_min;//BCD 码
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
	SSE_QUERY_QR_CODE_ERROR_REASON_FAILED = 0,
	SSE_QUERY_QR_CODE_ERROR_REASON_NO_PARA,
	SSE_QUERY_QR_CODE_ERROR_REASON_DATA_ERROR,
	SSE_QUERY_QR_CODE_ERROR_REASON_ORDER_FAILED,
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
	uint8_t status;//数据库执行状态 1 0 异常 1 正常
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
	uint8_t channel_id;//从1开始
	uint8_t cmd;//0 结束充电 1 锁定 2 解锁 3 开始充电 当预约成功时需要由后台发 送锁定命令。遥控开始充电功能由后台发送，充电机根据自己的条件进行开始充电操作。
	uint8_t mode;//解锁/开始充电时有效,一般是按金额充电模式 0 充满为止 1 按金额充 2 按时间充 3 按电量充 4 VIN 充电
	uint32_t data;//解锁/开始充电时有效,充满为止时为0, 按金额充时为金额(0.01元), 按时间充时高16位BCD码开始时分,低16位BCD码结束时分,按电量充时为电 量(0.01kWh)
} sse_remote_start_stop_t;

typedef struct {
	uint8_t device_id[32];//1 桩编码
	uint8_t on_off;//0 关 1 开
} sse_remote_ad_on_off_t;

typedef struct {
	uint8_t type;//0:start/stop 1:ad on/off
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

typedef enum {
	DEVIE_TYPE_UNKNOW = 0,
	DEVIE_TYPE_DC,
	DEVIE_TYPE_AC,
} devie_type_t;

typedef struct {
	uint8_t start_finish;
	uint8_t start_code;
	uint8_t start_failed_reason;
	uint8_t retry;
	uint16_t serial_event_start;

	command_status_t *channel_cmd_ctx;
} net_client_channel_data_ctx_t;

typedef struct {
	channels_info_t *channels_info;
	uint8_t request_timeout;
	uint16_t serial;
	uint16_t transaction_record_id;
	channel_record_item_t channel_record_item;
	uint16_t channel_record_sync_stamps;
	uint8_t query_device_info_id;
	uint16_t serial_query_device_info;
	uint16_t serial_query_device_message;

	uint32_t sse_report_duration;

	callback_chain_t *card_account_chain;
	callback_item_t card_account_callback_item;
	uint8_t card_id[32];
	uint8_t card_password[32];

	uint16_t serial_settings;
	uint16_t serial_remote_device;
	uint16_t serial_upgrade;
	uint32_t upgrade_offset;
	uint8_t upgrade_status;

	callback_chain_t *card_account_info_chain;
	callback_item_t card_account_info_callback_item;

	command_status_t *device_cmd_ctx;
	net_client_channel_data_ctx_t *channel_data_ctx;
} net_client_data_ctx_t;

typedef enum {
	NET_CLIENT_DEVICE_COMMAND_REPORT = 0,
	NET_CLIENT_DEVICE_COMMAND_EVENT_FAULT,
	NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD,
	NET_CLIENT_DEVICE_COMMAND_QUERY_DEVICE_INFO,
	NET_CLIENT_DEVICE_COMMAND_QUERY_DEVICE_MESSAGE,
	NET_CLIENT_DEVICE_COMMAND_QUERY_CARD_ACCOUNT,
	NET_CLIENT_DEVICE_COMMAND_SETTINGS,
	NET_CLIENT_DEVICE_COMMAND_REMOTE_DEVICE,
	NET_CLIENT_DEVICE_COMMAND_UPGRADE,
} net_client_device_command_t;

typedef enum {
	NET_CLIENT_CHANNEL_COMMAND_EVENT_START = 0,
	NET_CLIENT_CHANNEL_COMMAND_QUERY_QR_CODE,
	NET_CLIENT_CHANNEL_COMMAND_REMOTE_START_STOP,
} net_client_channel_command_t;

typedef int (*net_client_request_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size);
typedef int (*net_client_response_callback_t)(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size);
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

static int send_frame(net_client_info_t *net_client_info, uint16_t serial, uint8_t frame, uint8_t type, uint8_t *message, size_t len)
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

static int filter_channel_record_state(channel_record_item_state_t state)
{
	int ret = -1;

	if(state == CHANNEL_RECORD_ITEM_STATE_FINISH) {
		ret = 0;
	}

	return ret;
}

static void sync_transaction_record(void)
{
	channel_record_task_info_t *channel_record_task_info = get_or_alloc_channel_record_task_info(0);
	channel_record_info_t *channel_record_info = &channel_record_task_info->channel_record_info;
	uint16_t record_id;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, net_client_data_ctx->channel_record_sync_stamps) < 200) {
		return;
	}

	net_client_data_ctx->channel_record_sync_stamps = ticks;

	if(net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD].state != COMMAND_STATE_IDLE) {
		debug("");
		return;
	}

	//find record to upload
	if(get_channel_record_item_by_filter(channel_record_task_info, filter_channel_record_state, channel_record_info->start, channel_record_info->end, &record_id) != 0) {
		debug("");
		return;
	}

	report_transaction_record(record_id);
}

static uint8_t get_telemeter_faults(channels_info_t *channels_info)
{
	int i;
	uint8_t fault = 0;

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(get_fault(channel_info->faults, CHANNEL_FAULT_ENERGYMETER) == 1) {
			fault = 1;
			break;
		}
	}

	return fault;
}

static uint8_t get_channels_faults(channels_info_t *channels_info)
{
	int i;

	if(get_first_fault(channels_info->faults) != -1) {
		return 1;
	}

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(get_first_fault(channel_info->faults) != -1) {
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
	int i;

	if(get_fault(channels_info->faults, CHANNELS_FAULT_POWER_MODULES) == 1) {
		fault = SSE_DEVICE_FAULT_TYPE_MODULE;
		return fault;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_CARD_READER) == 1) {
		fault = SSE_DEVICE_FAULT_TYPE_CARD_READER;
		return fault;
	}

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(get_fault(channel_info->faults, CHANNEL_FAULT_CONNECT) == 1) {
			switch(i) {
				case 1: {
					fault = SSE_DEVICE_FAULT_TYPE_CONTROL_BOARD_B;
					return fault;
				}
				break;

				case 2: {
					fault = SSE_DEVICE_FAULT_TYPE_CONTROL_BOARD_C;
					return fault;
				}
				break;

				case 3: {
					fault = SSE_DEVICE_FAULT_TYPE_CONTROL_BOARD_D;
					return fault;
				}
				break;

				default: {
				}
				break;
			}
		}

		if(get_fault(channel_info->faults, CHANNEL_FAULT_FUNCTION_BOARD_CONNECT) == 1) {
			switch(i) {
				case 0: {
					fault = SSE_DEVICE_FAULT_TYPE_FUNCTION_BOARD_A;
					return fault;
				}
				break;

				case 1: {
					fault = SSE_DEVICE_FAULT_TYPE_FUNCTION_BOARD_B;
					return fault;
				}
				break;

				case 2: {
					fault = SSE_DEVICE_FAULT_TYPE_FUNCTION_BOARD_C;
					return fault;
				}
				break;

				case 3: {
					fault = SSE_DEVICE_FAULT_TYPE_FUNCTION_BOARD_D;
					return fault;
				}
				break;

				default: {
				}
				break;
			}
		}

		if(get_fault(channel_info->faults, CHANNEL_FAULT_ENERGYMETER) == 1) {
			switch(i) {
				case 0: {
					fault = SSE_DEVICE_FAULT_TYPE_TELEMETER_A;
					return fault;
				}
				break;

				case 1: {
					fault = SSE_DEVICE_FAULT_TYPE_TELEMETER_B;
					return fault;
				}
				break;

				case 2: {
					fault = SSE_DEVICE_FAULT_TYPE_TELEMETER_C;
					return fault;
				}
				break;

				case 3: {
					fault = SSE_DEVICE_FAULT_TYPE_TELEMETER_D;
					return fault;
				}
				break;

				default: {
				}
				break;
			}
		}
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_DOOR) == 1) {
		fault = CHANNELS_FAULT_DOOR;
		return fault;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_RELAY_ADHESION) == 1) {
		fault = SSE_DEVICE_FAULT_TYPE_RELAY_ADHESION;
		return fault;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_FORCE_STOP) == 1) {
		fault = SSE_DEVICE_FAULT_TYPE_FORCE_STOP;
		return fault;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_INPUT_OVER_VOLTAGE) == 1) {
		fault = SSE_DEVICE_FAULT_TYPE_INPUT_OVER_VOLTAGE;
		return fault;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_INPUT_LOW_VOLTAGE) == 1) {
		fault = SSE_DEVICE_FAULT_TYPE_INPUT_LOW_VOLTAGE;
		return fault;
	}

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(get_fault(channel_info->faults, CHANNEL_FAULT_OVER_TEMPERATURE) == 1) {
			fault = SSE_DEVICE_FAULT_TYPE_CHANNEL_OVER_TEMPERATURE;
		}
	}

	return fault;
}

static uint32_t get_sse_module_state_mask(channels_info_t *channels_info)
{
	uint32_t mask = 0;
	uint8_t *cells = (uint8_t *)&mask;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	channels_power_module_t *channels_power_module = (channels_power_module_t *)channels_info->channels_power_module;
	int i = 0;
	int channels_power_module_number = channels_settings->channels_power_module_settings.channels_power_module_number;

	for(i = 0; i < channels_power_module_number; i++) {
		power_module_item_info_t *power_module_item_info = get_power_module_item_info(channels_power_module, i);
		int j = i / 8;
		int k = i % 8;
		cells[j] = set_u8_bits(cells[j], k, (get_first_fault(power_module_item_info->faults) != -1) ? 1 : 0);
	}

	return mask;
}

static uint32_t get_sse_module_state_value(channels_info_t *channels_info)
{
	uint32_t state = 0;
	uint8_t *cells = (uint8_t *)&state;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	channels_power_module_t *channels_power_module = (channels_power_module_t *)channels_info->channels_power_module;
	channels_power_module_callback_t *channels_power_module_callback = channels_power_module->channels_power_module_callback;
	int i = 0;
	int channels_power_module_number = channels_settings->channels_power_module_settings.channels_power_module_number;

	if(channels_power_module_callback == NULL) {
		return state;
	}

	for(i = 0; i < channels_power_module_number; i++) {
		int j = i / 8;
		int k = i % 8;
		cells[j] = set_u8_bits(cells[j], k, 1);
	}

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

	u_sse_channel_device_state.s.connect = ((charger_info_t *)(channel_info->charger_info))->charger_connect_state;
	u_sse_channel_device_state.s.auxiliary_power_12 = (channel_info->auxiliary_power_type == AUXILIARY_POWER_TYPE_12) ? 1 : 0;
	u_sse_channel_device_state.s.auxiliary_power_24 = (channel_info->auxiliary_power_type == AUXILIARY_POWER_TYPE_24) ? 1 : 0;
	u_sse_channel_device_state.s.vehicle_relay_state = ((charger_info_t *)(channel_info->charger_info))->vehicle_relay_state;
	u_sse_channel_device_state.s.charger_lock_state = channel_info->charger_lock_state;

	return u_sse_channel_device_state.v;
}

static uint8_t get_sse_report_channel_charger_bms_state(channel_info_t *channel_info)
{
	uint8_t state = SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_NONE;

	if(channel_info->channel_settings.channel_type == CHANNEL_TYPE_DC) {
		switch(channel_info->state) {
			case CHANNEL_STATE_NONE:
			case CHANNEL_STATE_IDLE: {
				state = SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_IDLE;
			}
			break;

			case CHANNEL_STATE_STARTING: {
				state = SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_STARTING;
			}
			break;

			case CHANNEL_STATE_CHARGING: {
				state = SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_RUNNING;
			}
			break;

			case CHANNEL_STATE_STOPPING: {
				state = SSE_REPORT_CHANNEL_CHARGER_BMS_STATE_NONE;
			}
			break;

			default: {
			}
			break;
		}
	}

	return state;
}

static uint8_t get_sse_report_channel_charger_bms_stop_reason(channel_info_t *channel_info)
{
	uint8_t stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_NONE;

	if(channel_info->channel_settings.channel_type == CHANNEL_TYPE_DC) {
		switch(channel_info->state) {
			case CHANNEL_STATE_STOPPING: {
				switch(channel_info->channel_record_item.stop_reason) {
					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SOC_ARCHIEVED: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SOC_ARCHIEVED;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SOC_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SOC_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_VOLTAGE_ARCHIEVED: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_VOLTAGE_ARCHIEVED;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_VOLTAGE_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_VOLTAGE_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SINGLE_VOLTAGE_ARCHIEVED: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SINGLE_VOLTAGE_ARCHIEVED;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_SINGLE_VOLTAGE_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_SINGLE_VOLTAGE_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_INSULATION_FAULT: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_INSULATION_FAULT;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_INSULATION_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_INSULATION_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_OVER_TEMPERATURE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CONNECTOR_OVER_TEMPERATURE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CONNECTOR_TEMPERATURE_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_CONNECTOR_OVER_TEMPERATURE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BMS_CONNECTOR_OVER_TEMPERATURE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BMS_CONNECTOR_TEMPERATURE_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_FAULT: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CONNECTOR_FAULT;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CONNECTOR_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CONNECTOR_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_OVER_TEMPERATURE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BATTERY_OVER_TEMPERATURE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BATTERY_TEMPERATURE_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BATTERY_TEMPERATURE_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OTHER_FAULT: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_OTHER_FAULT;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OTHER_FAULT_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_OTHER_FAULT_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_OVER_CURRENT: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_OVER_CURRENT;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_CURRENT_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_CURRENT_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_ABNORMAL_VOLTAGE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_ABNORMAL_VOLTAGE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_ABNORMAL_VOLTAGE_NOT_CREDIBLE: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_ABNORMAL_VOLTAGE_NOT_CREDIBLE;
					}
					break;

					case CHANNEL_RECORD_ITEM_STOP_REASON_BMS_BMS_OTHER_FAULT: {
						stop_reason = SSE_REPORT_CHANNEL_CHARGER_BMS_STOP_REASON_BMS_OTHER_FAULT;
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
	}

	return stop_reason;
}

static void udpate_sse_chennel_charger_report(sse_channel_report_t *sse_channel_report, channel_info_t *channel_info)
{
	if(channel_info->channel_settings.channel_type == CHANNEL_TYPE_DC) {
		sse_report_channel_charge_info_dc_t *sse_report_channel_charge_info_dc = sse_channel_report->sse_report_channel_charge_info_dc;
		charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;
		sse_report_channel_charge_info_dc->soc = charger_info->bms_data.bcs_data.soc;
		sse_report_channel_charge_info_dc->bcl_require_voltage = charger_info->bms_data.bcl_data.require_voltage;
		sse_report_channel_charge_info_dc->bcl_require_current = charger_info->bms_data.bcl_data.require_current;
		sse_report_channel_charge_info_dc->output_voltage = channel_info->voltage;
		sse_report_channel_charge_info_dc->output_current = channel_info->current;
		sse_report_channel_charge_info_dc->bcs_charge_voltage = charger_info->bms_data.bcs_data.charge_voltage;
		sse_report_channel_charge_info_dc->bcs_charge_current = charger_info->bms_data.bcs_data.charge_current;
		sse_report_channel_charge_info_dc->telemeter_total = channel_info->total_energy;
		sse_report_channel_charge_info_dc->charge_energy = channel_info->channel_record_item.energy;
		sse_report_channel_charge_info_dc->charge_amount = channel_info->channel_record_item.amount;
		sse_report_channel_charge_info_dc->bcs_single_battery_max_voltage = charger_info->bms_data.bcs_data.u1.s.single_battery_max_voltage;
		sse_report_channel_charge_info_dc->bsm_battery_max_temperature = charger_info->bms_data.bsm_data.battery_max_temperature;
		sse_report_channel_charge_info_dc->bcs_remain_min = charger_info->bms_data.bcs_data.remain_min;
		sse_report_channel_charge_info_dc->dc_p_temperature = charger_info->dc_p_temperature;
		sse_report_channel_charge_info_dc->dc_n_temperature = charger_info->dc_n_temperature;
	} else if(channel_info->channel_settings.channel_type == CHANNEL_TYPE_AC) {
		sse_report_channel_charge_info_ac_t *sse_report_channel_charge_info_ac = sse_channel_report->sse_report_channel_charge_info_ac;
		sse_report_channel_charge_info_ac->output_voltage = channel_info->voltage;
		sse_report_channel_charge_info_ac->output_current = channel_info->current;
		sse_report_channel_charge_info_ac->telemeter_total = channel_info->total_energy;
		sse_report_channel_charge_info_ac->charge_energy = channel_info->channel_record_item.energy;
		sse_report_channel_charge_info_ac->charge_amount = channel_info->channel_record_item.amount;
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

static uint8_t get_device_type(channels_info_t *channels_info)
{
	uint8_t device_type = 0;
	channel_info_t *channel_info = channels_info->channel_info + 0;

	if(channels_info->channels_settings.channel_number == 0) {
		return device_type;
	}

	switch(channel_info->channel_settings.channel_type) {
		case CHANNEL_TYPE_DC: {
			device_type = 1;
		}
		break;

		case CHANNEL_TYPE_AC: {
			device_type = 2;
		}
		break;

		default: {
		}
		break;
	}

	return device_type;
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
	size_t size = 0;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	snprintf((char *)sse_0x00_request_report->device_id, 32, "%s", app_info->mechine_info.device_id);
	sse_0x00_request_report->device_type = get_device_type(channels_info);
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

		if(channel_info->channel_settings.channel_type == CHANNEL_TYPE_DC) {
			channel_report_start += (sizeof(sse_channel_report_t) + sizeof(sse_report_channel_charge_info_dc_t));
		} else if(channel_info->channel_settings.channel_type == CHANNEL_TYPE_AC) {
			channel_report_start += (sizeof(sse_channel_report_t) + sizeof(sse_report_channel_charge_info_ac_t));
		} else {
			app_panic();
		}
	}

	size = channel_report_start - (uint8_t *)sse_0x00_request_report;

	send_frame(net_client_info, net_client_data_ctx->serial++, item->frame, 0, (uint8_t *)sse_0x00_request_report, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_callback_report(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x00_response_report_t *sse_0x00_response_report = (sse_0x00_response_report_t *)(sse_frame_header + 1);

	if(sse_0x00_response_report->status != 0) {
	}

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
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

static uint32_t get_device_fault_type(channels_info_t *channels_info)
{
	u_sse_event_fault_type_t u_sse_event_fault_type;
	int i;

	if(get_fault(channels_info->faults, CHANNELS_FAULT_DISPLAY) == 1) {
		u_sse_event_fault_type.s.display = 1;
	}

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info = channels_info->channel_info + i;

		if(get_fault(channel_info->faults, CHANNEL_FAULT_CONNECT) == 1) {
			switch(i) {
				case 0: {
					u_sse_event_fault_type.s.control_board_1 = 1;
				}
				break;

				case 1: {
					u_sse_event_fault_type.s.control_board_2 = 1;
				}
				break;

				case 2: {
					u_sse_event_fault_type.s.control_board_3 = 1;
				}
				break;

				case 3: {
					u_sse_event_fault_type.s.control_board_4 = 1;
				}
				break;

				case 4: {
					u_sse_event_fault_type.s.control_board_5 = 1;
				}
				break;

				case 5: {
					u_sse_event_fault_type.s.control_board_6 = 1;
				}
				break;

				case 6: {
					u_sse_event_fault_type.s.control_board_7 = 1;
				}
				break;

				case 7: {
					u_sse_event_fault_type.s.control_board_8 = 1;
				}
				break;

				default: {
				}
				break;
			}
		}

		if(get_fault(channel_info->faults, CHANNEL_FAULT_FUNCTION_BOARD_CONNECT) == 1) {
			switch(i) {
				case 0: {
					u_sse_event_fault_type.s.function_board_1 = 1;
				}
				break;

				case 1: {
					u_sse_event_fault_type.s.function_board_2 = 1;
				}
				break;

				case 2: {
					u_sse_event_fault_type.s.function_board_3 = 1;
				}
				break;

				case 3: {
					u_sse_event_fault_type.s.function_board_4 = 1;
				}
				break;

				case 4: {
					u_sse_event_fault_type.s.function_board_5 = 1;
				}
				break;

				case 5: {
					u_sse_event_fault_type.s.function_board_6 = 1;
				}
				break;

				case 6: {
					u_sse_event_fault_type.s.function_board_7 = 1;
				}
				break;

				case 7: {
					u_sse_event_fault_type.s.function_board_8 = 1;
				}
				break;

				default: {
				}
				break;
			}

			if(get_fault(channel_info->faults, CHANNEL_FAULT_ENERGYMETER) == 1) {
				switch(i) {
					case 0: {
						u_sse_event_fault_type.s.telemeter_1 = 1;
					}
					break;

					case 1: {
						u_sse_event_fault_type.s.telemeter_2 = 1;
					}
					break;

					case 2: {
						u_sse_event_fault_type.s.telemeter_3 = 1;
					}
					break;

					case 3: {
						u_sse_event_fault_type.s.telemeter_4 = 1;
					}
					break;

					default: {
					}
					break;
				}
			}
		}
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_CARD_READER) == 1) {
		u_sse_event_fault_type.s.card_reader = 1;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_DISPLAY) == 1) {
		u_sse_event_fault_type.s.display1 = 1;
	}

	u_sse_event_fault_type.v = 0;

	return u_sse_event_fault_type.v;
}

static int request_callback_event_fault(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = 0;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x01_request_event_t *sse_0x01_request_event = (sse_0x01_request_event_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	sse_request_event_fault_t *sse_request_event_fault = (sse_request_event_fault_t *)sse_0x01_request_event->event_info;
	size_t size = (uint8_t *)(sse_request_event_fault + 1) - (uint8_t *)sse_0x01_request_event;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	snprintf((char *)sse_0x01_request_event->device_id, 32, "%s", app_info->mechine_info.device_id);
	sse_0x01_request_event->device_type = get_device_type(channels_info);
	sse_0x01_request_event->float_percision = (channels_info->channels_settings.magnification == 0) ? 2 : 3;
	sse_0x01_request_event->event_type = 0x02;

	sse_request_event_fault->type.v = get_device_fault_type(channels_info);;
	sse_request_event_fault->status.v = get_device_state(channels_info);

	send_frame(net_client_info, net_client_data_ctx->serial++, item->frame, 0, (uint8_t *)sse_0x01_request_event, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;
	return ret;
}

static int response_callback_event_fault(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x01_response_event_t *sse_0x01_response_event = (sse_0x01_response_event_t *)(sse_frame_header + 1);
	//channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &channels_info->channels_settings;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	if(sse_0x01_response_event->event_type != 0x02) {
		ret = 1;
		return ret;
	}

	if(strncmp((const char *)sse_0x01_response_event->device_id, (const char *)app_info->mechine_info.device_id, 32) == 0) {//设备号不对,返回出错
		return ret;
	}

	if(sse_0x01_response_event->status == 0) {
	}

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int timeout_callback_event_fault(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	return ret;
}

static net_client_command_item_t net_client_command_item_event_fault = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_EVENT_FAULT,
	.frame = 0x01,
	.request_callback = request_callback_event_fault,
	.response_callback = response_callback_event_fault,
	.timeout_callback = timeout_callback_event_fault,
};

static int request_callback_event_upload_record(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x01_request_event_t *sse_0x01_request_event = (sse_0x01_request_event_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	sse_request_event_record_t *sse_request_event_record = (sse_request_event_record_t *)sse_0x01_request_event->event_info;
	channel_record_task_info_t *channel_record_task_info = get_or_alloc_channel_record_task_info(0);
	char dt[20];
	struct tm *tm;
	size_t size;
	uint8_t start_seg_index;
	uint8_t stop_seg_index;
	int i;
	sse_record_section_t *sse_record_section = sse_request_event_record->sse_record_section;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	snprintf((char *)sse_0x01_request_event->device_id, 32, "%s", app_info->mechine_info.device_id);
	sse_0x01_request_event->device_type = get_device_type(channels_info);
	sse_0x01_request_event->float_percision = (channels_info->channels_settings.magnification == 0) ? 2 : 3;
	sse_0x01_request_event->event_type = 0x03;

	if(get_channel_record_item_by_id(channel_record_task_info, net_client_data_ctx->transaction_record_id, &net_client_data_ctx->channel_record_item) != 0) {
		net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
		return ret;
	}

	sse_request_event_record->record_id = net_client_data_ctx->channel_record_item.id;
	sse_request_event_record->channel_id = net_client_data_ctx->channel_record_item.channel_id + 1;
	memcpy(sse_request_event_record->vin, net_client_data_ctx->channel_record_item.vin, 17);
	sse_request_event_record->chm_bms_version = get_u16_from_u8_lh(net_client_data_ctx->channel_record_item.chm_version_1, net_client_data_ctx->channel_record_item.chm_version_0);;
	sse_request_event_record->brm_battery_type = net_client_data_ctx->channel_record_item.brm_battery_type;
	sse_request_event_record->bcp_rate_total_power = net_client_data_ctx->channel_record_item.bcp_rate_total_power;
	sse_request_event_record->bcp_total_voltage = net_client_data_ctx->channel_record_item.bcp_total_voltage;
	sse_request_event_record->bcp_max_charge_voltage_single_battery = net_client_data_ctx->channel_record_item.bcp_max_charge_voltage_single_battery;
	sse_request_event_record->bcp_max_temperature = net_client_data_ctx->channel_record_item.bcp_max_temperature;
	sse_request_event_record->bcp_max_charge_voltage = net_client_data_ctx->channel_record_item.bcp_max_charge_voltage;
	snprintf((char *)sse_request_event_record->card_id, 32, "%lu", (uint32_t)net_client_data_ctx->channel_record_item.card_id);
	sse_request_event_record->start_type = net_client_data_ctx->channel_record_item.start_reason;
	memset(sse_request_event_record->start_dt, 0xff, sizeof(sse_request_event_record->start_dt));
	tm = localtime(&net_client_data_ctx->channel_record_item.start_time);
	strftime(dt, sizeof(dt), "%Y%m%d%H%M%S", tm);
	ascii_to_bcd(dt, strlen(dt), sse_request_event_record->start_dt, sizeof(sse_request_event_record->start_dt));
	sse_request_event_record->start_telemeter_total = net_client_data_ctx->channel_record_item.start_total_energy;
	sse_request_event_record->withholding = net_client_data_ctx->channel_record_item.withholding;
	sse_request_event_record->stop_reason = net_client_data_ctx->channel_record_item.stop_reason;
	tm = localtime(&net_client_data_ctx->channel_record_item.stop_time);
	strftime(dt, sizeof(dt), "%Y%m%d%H%M%S", tm);
	ascii_to_bcd(dt, strlen(dt), sse_request_event_record->stop_dt, sizeof(sse_request_event_record->stop_dt));
	sse_request_event_record->stop_telemeter_total = net_client_data_ctx->channel_record_item.stop_total_energy;
	sse_request_event_record->charge_amount = net_client_data_ctx->channel_record_item.amount;
	sse_request_event_record->charge_energy = net_client_data_ctx->channel_record_item.energy;
	sse_request_event_record->start_soc = net_client_data_ctx->channel_record_item.start_soc;
	sse_request_event_record->stop_soc = net_client_data_ctx->channel_record_item.stop_soc;

	start_seg_index = get_seg_index_by_ts(net_client_data_ctx->channel_record_item.start_time);
	stop_seg_index = get_seg_index_by_ts(net_client_data_ctx->channel_record_item.stop_time);

	if(start_seg_index > stop_seg_index) {
		stop_seg_index += PRICE_SEGMENT_SIZE;
	}

	sse_request_event_record->section_number = (stop_seg_index + 1) - start_seg_index;

	for(i = start_seg_index; i <= stop_seg_index; i++) {
		sse_record_section->section_id = i % PRICE_SEGMENT_SIZE;
		sse_record_section->energy = net_client_data_ctx->channel_record_item.energy_seg[sse_record_section->section_id];
		sse_record_section += 1;
	}

	size = (uint8_t *)sse_record_section - (uint8_t *)sse_0x01_request_event;

	send_frame(net_client_info, net_client_data_ctx->serial++, item->frame, 0, (uint8_t *)sse_0x01_request_event, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_callback_event_upload_record(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x01_response_event_t *sse_0x01_response_event = (sse_0x01_response_event_t *)(sse_frame_header + 1);
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	uint8_t upload = 1;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	if(sse_0x01_response_event->event_type != 0x03) {
		ret = 1;
		return ret;
	}

	if(strncmp((const char *)sse_0x01_response_event->device_id, (const char *)app_info->mechine_info.device_id, 32) == 0) {//设备号不对,返回出错
		return ret;
	}

	if(sse_0x01_response_event->status == 0) {
		upload = 0;
	}

	if(sse_0x01_response_event->record_id != net_client_data_ctx->transaction_record_id) {
		upload = 0;
	}

	if(upload == 1) {
		channel_record_task_info_t *channel_record_task_info = get_or_alloc_channel_record_task_info(0);

		net_client_data_ctx->channel_record_item.state = CHANNEL_RECORD_ITEM_STATE_UPLOAD;
		channel_record_update(channel_record_task_info, &net_client_data_ctx->channel_record_item);
	}

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int timeout_callback_event_upload_record(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	return ret;
}

static net_client_command_item_t net_client_command_item_event_upload_record = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD,
	.frame = 0x01,
	.request_callback = request_callback_event_upload_record,
	.response_callback = response_callback_event_upload_record,
	.timeout_callback = timeout_callback_event_upload_record,
};

static void update_sse_query_price_item_info(uint8_t i, uint8_t start_seg, uint8_t stop_seg, uint32_t price, void *_sse_query_price_item_info)
{
	sse_query_price_item_info_t *sse_query_price_item_info = (sse_query_price_item_info_t *)_sse_query_price_item_info;
	time_t start;
	time_t stop;
	struct tm *tm;

	if(i >= 12) {
		return;
	}

	start = get_ts_by_seg_index(start_seg);
	stop = get_ts_by_seg_index(stop_seg);

	sse_query_price_item_info += i;

	tm = localtime(&start);
	sse_query_price_item_info->start_hour_min = get_u16_from_u8_lh(
	            get_bcd_from_u8(tm->tm_min),
	            get_bcd_from_u8(tm->tm_hour));
	tm = localtime(&stop);
	sse_query_price_item_info->stop_hour_min = get_u16_from_u8_lh(
	            get_bcd_from_u8(tm->tm_min),
	            get_bcd_from_u8((stop == 0) ? 24 : tm->tm_hour));

	sse_query_price_item_info->price = price;
}

static int request_callback_query_device_info(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x02_request_query_t *sse_0x02_request_query = (sse_0x02_request_query_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	uint8_t *data = sse_0x02_request_query->query;
	size_t size;

	sse_0x02_request_query->type = 0;
	sse_0x02_request_query->id = net_client_data_ctx->query_device_info_id;

	if((sse_0x02_request_query->id == 0) || (sse_0x02_request_query->id == 0xff)) {
		struct in_addr sin_addr = {0};
		struct sockaddr_in *sockaddr_in = (struct sockaddr_in *)&net_client_info->net_client_addr_info.socket_addr_info->addr;
		uint32_t *ip = (uint32_t *)data;

		if(sockaddr_in != NULL) {
			sin_addr = sockaddr_in->sin_addr;
		}

		*ip = sin_addr.s_addr;
		data += sizeof(uint32_t);
	}

	if((sse_0x02_request_query->id == 1) || (sse_0x02_request_query->id == 0xff)) {
		in_port_t sin_port = 0;
		struct sockaddr_in *sockaddr_in = (struct sockaddr_in *)&net_client_info->net_client_addr_info.socket_addr_info->addr;
		uint32_t *port = (uint32_t *)data;

		if(sockaddr_in != NULL) {
			sin_port = sockaddr_in->sin_port;
		}

		*port = ntohs(sin_port);
		data += sizeof(uint32_t);
	}

	if((sse_0x02_request_query->id == 2) || (sse_0x02_request_query->id == 0xff)) {
		uint32_t *report_duration = (uint32_t *)data;
		*report_duration = net_client_data_ctx->sse_report_duration;//todo
		data += sizeof(uint32_t);
	}

	if((sse_0x02_request_query->id == 3) || (sse_0x02_request_query->id == 0xff)) {
		uint32_t *service_price = (uint32_t *)data;
		*service_price = channels_settings->price_info.service_price;
		data += sizeof(uint32_t);
	}

	if((sse_0x02_request_query->id == 4) || (sse_0x02_request_query->id == 0xff)) {
		sse_query_price_info_t *sse_query_price_info = (sse_query_price_info_t *)data;
		sse_query_price_item_info_t *sse_query_price_item_info = (sse_query_price_item_info_t *)sse_query_price_info->item;

		sse_query_price_info->count = parse_price_info(&channels_settings->price_info, update_sse_query_price_item_info, sse_query_price_item_info);

		data += sizeof(sse_query_price_info_t) + sse_query_price_info->count * sizeof(sse_query_price_item_info_t);
	}

	if((sse_0x02_request_query->id == 5) || (sse_0x02_request_query->id == 0xff)) {
		sse_query_ad_info_t *sse_query_ad_info = (sse_query_ad_info_t *)data;
		sse_query_ad_info->count = 0;

		data += sizeof(sse_query_ad_info_t) + sse_query_ad_info->count * sizeof(sse_query_ad_item_info_t);
	}

	if((sse_0x02_request_query->id == 6) || (sse_0x02_request_query->id == 0xff)) {
		uint32_t *devie_type = (uint32_t *)data;

		*devie_type = DEVIE_TYPE_DC;

		data += sizeof(uint32_t);
	}

	if((sse_0x02_request_query->id == 7) || (sse_0x02_request_query->id == 0xff)) {
		uint32_t *channel_number = (uint32_t *)data;

		*channel_number = channels_info->channel_number;

		data += sizeof(uint32_t);
	}

	if((sse_0x02_request_query->id == 8) || (sse_0x02_request_query->id == 0xff)) {
		uint32_t *fw_version = (uint32_t *)data;

		*fw_version = get_u32_from_bcd_b0123(VER_MAJOR, VER_MINOR, VER_REV, VER_BUILD);

		data += sizeof(uint32_t);
	}

	size = data - (uint8_t *)sse_0x02_request_query;

	send_frame(net_client_info, net_client_data_ctx->serial_query_device_info, item->frame, 1, (uint8_t *)sse_0x02_request_query, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int response_callback_query_device_info(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x02_response_query_t *sse_0x02_response_query = (sse_0x02_response_query_t *)(sse_frame_header + 1);
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;

	if(sse_frame_header->cmd.type == 0) {//非回复,忽略
		ret = 1;
		return ret;
	}

	if(sse_0x02_response_query->type != 0) {
		ret = 1;
		return ret;
	}

	net_client_data_ctx->query_device_info_id = sse_0x02_response_query->id;
	net_client_data_ctx->serial_query_device_info = sse_frame_header->serial;

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
	ret = 0;
	return ret;
}

static int timeout_callback_query_device_info(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	return ret;
}

static net_client_command_item_t net_client_command_item_query_device_info = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_QUERY_DEVICE_INFO,
	.frame = 0x02,
	.request_callback = request_callback_query_device_info,
	.response_callback = response_callback_query_device_info,
	.timeout_callback = timeout_callback_query_device_info,
};

static int request_callback_query_device_message(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x02_request_query_t *sse_0x02_request_query = (sse_0x02_request_query_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	//channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	uint8_t *data = sse_0x02_request_query->query;
	size_t size;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	sse_0x02_request_query->type = 1;
	sse_0x02_request_query->id = net_client_data_ctx->query_device_info_id;

	if((sse_0x02_request_query->id == 0) || (sse_0x02_request_query->id == 0xff)) {
		uint8_t *device_id = (uint8_t *)data;
		snprintf((char *)device_id, 32, "%s", app_info->mechine_info.device_id);

		data += 32;
	}

	if((sse_0x02_request_query->id == 1) || (sse_0x02_request_query->id == 0xff)) {
		time_t ts = get_time();
		struct tm *tm = localtime(&ts);
		uint8_t *date_time = (uint8_t *)data;
		char dt[20];

		memset(date_time, 0xff, 8);
		strftime(dt, sizeof(dt), "%Y%m%d%H%M%S", tm);
		ascii_to_bcd(dt, strlen(dt), date_time, 8);

		data += 8;
	}

	if((sse_0x02_request_query->id == 2) || (sse_0x02_request_query->id == 0xff)) {
		data += 16;
	}

	if((sse_0x02_request_query->id == 3) || (sse_0x02_request_query->id == 0xff)) {
		data += 16;
	}

	if((sse_0x02_request_query->id == 4) || (sse_0x02_request_query->id == 0xff)) {
		data += 256;
	}

	if((sse_0x02_request_query->id == 5) || (sse_0x02_request_query->id == 0xff)) {
		data += 256;
	}

	size = data - (uint8_t *)sse_0x02_request_query;

	send_frame(net_client_info, net_client_data_ctx->serial_query_device_message, item->frame, 1, (uint8_t *)sse_0x02_request_query, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int response_callback_query_device_message(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x02_response_query_t *sse_0x02_response_query = (sse_0x02_response_query_t *)(sse_frame_header + 1);
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;

	if(sse_frame_header->cmd.type == 0) {//非回复,忽略
		ret = 1;
		return ret;
	}

	if(sse_0x02_response_query->type != 1) {
		ret = 1;
		return ret;
	}

	net_client_data_ctx->query_device_info_id = sse_0x02_response_query->id;
	net_client_data_ctx->serial_query_device_message = sse_frame_header->serial;

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
	ret = 0;
	return ret;
}

static int timeout_callback_query_device_message(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	return ret;
}

static net_client_command_item_t net_client_command_item_query_device_message = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_QUERY_DEVICE_MESSAGE,
	.frame = 0x02,
	.request_callback = request_callback_query_device_message,
	.response_callback = response_callback_query_device_message,
	.timeout_callback = timeout_callback_query_device_message,
};

static void sse_query_account(net_client_info_t *net_client_info, account_request_info_t *account_request_info)
{
	account_response_info_t account_response_info;

	if(get_client_state(net_client_info) != CLIENT_CONNECTED) {
		if(account_request_info->fn != NULL) {
			account_response_info.code = ACCOUNT_STATE_CODE_OFFLINE;
			account_request_info->fn(account_request_info->fn_ctx, &account_response_info);
		}

		return;
	}

	switch(account_request_info->account_type) {
		case ACCOUNT_TYPE_CARD: {
			if(net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_QUERY_CARD_ACCOUNT].state != COMMAND_STATE_IDLE) {
				if(account_request_info->fn != NULL) {
					account_response_info.code = ACCOUNT_STATE_CODE_BUSY;
					account_request_info->fn(account_request_info->fn_ctx, &account_response_info);
				}

				return;
			}

			snprintf((char *)net_client_data_ctx->card_id, 32, "%lu", (uint32_t)account_request_info->card_id);
			snprintf((char *)net_client_data_ctx->card_password, 32, "%s", (char *)account_request_info->password);

			remove_callback(net_client_data_ctx->card_account_info_chain, &net_client_data_ctx->card_account_info_callback_item);
			net_client_data_ctx->card_account_info_callback_item.fn = account_request_info->fn;
			net_client_data_ctx->card_account_info_callback_item.fn_ctx = account_request_info->fn_ctx;
			OS_ASSERT(register_callback(net_client_data_ctx->card_account_info_chain, &net_client_data_ctx->card_account_info_callback_item) == 0);
			net_client_data_ctx->device_cmd_ctx[NET_CLIENT_DEVICE_COMMAND_QUERY_CARD_ACCOUNT].state = COMMAND_STATE_REQUEST;
		}
		break;

		default: {
		}
		break;
	}
}

static int request_callback_query_card_account(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x02_request_query_t *sse_0x02_request_query = (sse_0x02_request_query_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	sse_query_card_account_info_t *sse_query_card_account_info = (sse_query_card_account_info_t *)sse_0x02_request_query->query;
	size_t size;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	sse_0x02_request_query->type = 2;
	sse_0x02_request_query->id = 0;

	snprintf((char *)sse_query_card_account_info->device_id, 32, "%s", (char *)app_info->mechine_info.device_id);
	snprintf((char *)sse_query_card_account_info->card_id, 32, "%s", (char *)net_client_data_ctx->card_id);
	snprintf((char *)sse_query_card_account_info->card_password, 32, "%s", (char *)net_client_data_ctx->card_password);
	sse_query_card_account_info->withholding = channels_settings->withholding;

	size = (uint8_t *)(sse_query_card_account_info + 1) - (uint8_t *)sse_0x02_request_query;

	send_frame(net_client_info, net_client_data_ctx->serial++, item->frame, 0, (uint8_t *)sse_0x02_request_query, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_callback_query_card_account(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x02_response_query_t *sse_0x02_response_query = (sse_0x02_response_query_t *)(sse_frame_header + 1);
	sse_query_card_account_confirm_t *sse_query_card_account_confirm = (sse_query_card_account_confirm_t *)sse_0x02_response_query->query;
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	account_response_info_t account_response_info;

	account_response_info.code = ACCOUNT_STATE_CODE_UNKNOW;

	if(sse_frame_header->cmd.type == 0) {//非回复,忽略
		ret = 1;
		return ret;
	}

	if(sse_0x02_response_query->type != 2) {
		ret = 1;
		return ret;
	}

	if(sse_0x02_response_query->id != 0) {
		ret = 1;
		return ret;
	}

	if(sse_query_card_account_confirm->valid == 1) {
		account_response_info.code = ACCOUNT_STATE_CODE_OK;
		account_response_info.balance = sse_query_card_account_confirm->card_balance;
	} else {
		switch(sse_query_card_account_confirm->reason) {
			case SSE_QUERY_CARD_ERROR_REASON_PASSWORD: {
				account_response_info.code = ACCOUNT_STATE_CODE_AUTH;
			}
			break;

			case SSE_QUERY_CARD_ERROR_REASON_AMOUNT: {
				account_response_info.code = ACCOUNT_STATE_CODE_AMOUNT;
			}
			break;

			case SSE_QUERY_CARD_ERROR_REASON_STOP: {
				account_response_info.code = ACCOUNT_STATE_CODE_STOP;
			}
			break;

			case SSE_QUERY_CARD_ERROR_REASON_UNUSED: {
				account_response_info.code = ACCOUNT_STATE_CODE_UNUSED;
			}
			break;

			case SSE_QUERY_CARD_ERROR_REASON_NOT_RECOGNIZE: {
				account_response_info.code = ACCOUNT_STATE_CODE_UNKNOW;
			}
			break;

			default: {
			}
			break;
		}
	}

	do_callback_chain(net_client_data_ctx->card_account_info_chain, &account_response_info);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int timeout_callback_query_card_account(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	account_response_info_t account_response_info;
	account_response_info.code = ACCOUNT_STATE_CODE_TIMEOUT;

	do_callback_chain(net_client_data_ctx->card_account_info_chain, &account_response_info);
	return ret;
}

static net_client_command_item_t net_client_command_item_query_card_account = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_QUERY_CARD_ACCOUNT,
	.frame = 0x02,
	.request_callback = request_callback_query_card_account,
	.response_callback = response_callback_query_card_account,
	.timeout_callback = timeout_callback_query_card_account,
};

static int request_callback_settings(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x03_request_settings_t *sse_0x03_request_settings = (sse_0x03_request_settings_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	//channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &channels_info->channels_settings;
	size_t size;

	sse_0x03_request_settings->status = 1;
	size = sizeof(sse_0x03_request_settings_t);

	send_frame(net_client_info, net_client_data_ctx->serial_settings, item->frame, 1, (uint8_t *)sse_0x03_request_settings, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static void price_seg_to_price_info(price_info_t *price_info, sse_query_price_item_info_t *sse_query_price_item_info, uint8_t max_price_seg)
{
	int i;
	int j;

	for(i = 0; i < max_price_seg; i++) {
		sse_query_price_item_info_t *item = sse_query_price_item_info + i;
		time_t start = get_u8_from_bcd(get_u8_h_from_u16(item->start_hour_min)) * 3600 + get_u8_from_bcd(get_u8_l_from_u16(item->start_hour_min)) * 60;
		time_t stop = get_u8_from_bcd(get_u8_h_from_u16(item->stop_hour_min)) * 3600 + get_u8_from_bcd(get_u8_l_from_u16(item->stop_hour_min)) * 60;
		start = get_seg_index_by_ts(start);
		stop = get_seg_index_by_ts(stop);

		if(stop == 0) {
			stop = PRICE_SEGMENT_SIZE;
		}

		for(j = start; j < stop ; j++) {
			price_info->price[j] = item->price;
		}
	}
}

static int response_callback_settings(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	sse_0x03_response_settings_t *sse_0x03_response_settings = (sse_0x03_response_settings_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	uint8_t settings_type = sse_0x03_response_settings->type;
	uint8_t settings_id = sse_0x03_response_settings->id;
	uint8_t *data = sse_0x03_response_settings->query;

	if(net_client_data_ctx->device_cmd_ctx[item->cmd].state != COMMAND_STATE_IDLE) {
		ret = 1;
		return ret;
	}

	if(sse_frame_header->cmd.type == 1) {
		ret = 1;
		return ret;
	}

	ret = 0;

	switch(settings_type) {
		case 0: {
			if((settings_id == 0) && (settings_id == 0xff)) {
				//uint32_t *ip = (uint32_t *)data;
				data += sizeof(uint32_t);
			}

			if((settings_id == 1) || (settings_id == 0xff)) {
				//uint32_t *port = (uint32_t *)data;
				data += sizeof(uint32_t);
			}

			if((settings_id == 2) || (settings_id == 0xff)) {
				uint32_t *report_duration = (uint32_t *)data;
				*report_duration = net_client_data_ctx->sse_report_duration;//todo
				data += sizeof(uint32_t);
			}

			if((settings_id == 3) || (settings_id == 0xff)) {
				uint32_t *service_price = (uint32_t *)data;
				channels_settings->price_info.service_price = *service_price;
				data += sizeof(uint32_t);
			}

			if((settings_id == 4) || (settings_id == 0xff)) {
				sse_query_price_info_t *sse_query_price_info = (sse_query_price_info_t *)data;
				sse_query_price_item_info_t *sse_query_price_item_info = (sse_query_price_item_info_t *)sse_query_price_info->item;

				price_seg_to_price_info(&channels_settings->price_info, sse_query_price_item_info, sse_query_price_info->count);

				data += sizeof(sse_query_price_info_t) + sse_query_price_info->count * sizeof(sse_query_price_item_info_t);
			}

			if((settings_id == 5) || (settings_id == 0xff)) {
				sse_query_ad_info_t *sse_query_ad_info = (sse_query_ad_info_t *)data;

				data += sizeof(sse_query_ad_info_t) + sse_query_ad_info->count * sizeof(sse_query_ad_item_info_t);
			}
		}
		break;

		case 1: {
			if((settings_id == 0) || (settings_id == 0xff)) {
				uint8_t *date_time = (uint8_t *)data;
				char dt[20];
				struct tm tm;
				int catched;

				bcd_to_ascii(dt, sizeof(dt), date_time, 8);

				if(sscanf(dt, "%04d%02d%02d%02d%02d%02d%n",
				          &tm.tm_year,
				          &tm.tm_mon,
				          &tm.tm_mday,
				          &tm.tm_hour,
				          &tm.tm_min,
				          &tm.tm_sec,
				          &catched) == 6) {

					tm.tm_year -= 1900;
					tm.tm_mon -= 1;

					set_time(mktime(&tm));
				}

				data += 8;
			}

			if((settings_id == 1) || (settings_id == 0xff)) {
				data += 16;
			}

			if((settings_id == 2) || (settings_id == 0xff)) {
				data += 16;
			}

			if((settings_id == 3) || (settings_id == 0xff)) {
				data += 256;
			}

			if((settings_id == 4) || (settings_id == 0xff)) {
				data += 256;
			}

			if((settings_id == 5) || (settings_id == 0xff)) {
				data += 256;
			}
		}
		break;

		case 2: {
		}
		break;

		case 3: {
			if(settings_id == 0) {
				uint16_t *power_threshold = (uint16_t *)data;
				channels_settings->power_threshold = *power_threshold;
				data += sizeof(uint16_t);
			}
		}
		break;

		default: {
			ret = -1;
		}
		break;
	}

	net_client_data_ctx->serial_settings = sse_frame_header->serial;

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
	return ret;
}

static int timeout_callback_settings(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	return ret;
}

static net_client_command_item_t net_client_command_item_settings = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_SETTINGS,
	.frame = 0x03,
	.request_callback = request_callback_settings,
	.response_callback = response_callback_settings,
	.timeout_callback = timeout_callback_settings,
};

static int request_callback_remote_device(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x04_request_remote_t *sse_0x04_request_remote = (sse_0x04_request_remote_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	//channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &channels_info->channels_settings;
	size_t size;

	sse_0x04_request_remote->status = 1;
	sse_0x04_request_remote->type = 1;
	sse_0x04_request_remote->result = 1;
	size = sizeof(sse_0x04_request_remote_t);

	send_frame(net_client_info, net_client_data_ctx->serial_remote_device, item->frame, 1, (uint8_t *)sse_0x04_request_remote, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int response_callback_remote_device(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	sse_0x04_response_remote_t *sse_0x04_response_remote = (sse_0x04_response_remote_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;
	sse_remote_ad_on_off_t *sse_remote_ad_on_off = (sse_remote_ad_on_off_t *)sse_0x04_response_remote->remote;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	if(sse_frame_header->cmd.type == 1) {
		ret = 1;
		return ret;
	}

	if(sse_0x04_response_remote->type != 1) {
		ret = 1;
		return ret;
	}

	if(strncmp((const char *)sse_remote_ad_on_off->device_id, (const char *)app_info->mechine_info.device_id, 32) == 0) {//设备号不对,返回出错
		return ret;
	}

	//sse_remote_ad_on_off->on_off;

	ret = 0;

	net_client_data_ctx->serial_remote_device = sse_frame_header->serial;

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
	return ret;
}

static int timeout_callback_remote_device(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	return ret;
}

static net_client_command_item_t net_client_command_item_remote_device = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_REMOTE_DEVICE,
	.frame = 0x04,
	.request_callback = request_callback_remote_device,
	.response_callback = response_callback_remote_device,
	.timeout_callback = timeout_callback_remote_device,
};

static int request_callback_upgrade(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x05_request_upgrade_t *sse_0x05_request_upgrade = (sse_0x05_request_upgrade_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	//channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &channels_info->channels_settings;
	size_t size;

	sse_0x05_request_upgrade->status = net_client_data_ctx->upgrade_status;
	sse_0x05_request_upgrade->offset = net_client_data_ctx->upgrade_offset;
	size = sizeof(sse_0x05_request_upgrade_t);

	send_frame(net_client_info, net_client_data_ctx->serial_upgrade, item->frame, 1, (uint8_t *)sse_0x05_request_upgrade, size);

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;
	ret = 0;
	return ret;
}

static int do_upgrade(size_t total, size_t offset, uint8_t *data, size_t len)
{
	return 0;
}

static int response_callback_upgrade(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	sse_0x05_response_upgrade_t *sse_0x05_response_upgrade = (sse_0x05_response_upgrade_t *)(sse_frame_header + 1);
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	//channels_settings_t *channels_settings = &net_client_data_ctx->channels_info->channels_settings;

	if(sse_frame_header->cmd.type == 1) {
		ret = 1;
		return ret;
	}

	ret = do_upgrade(sse_0x05_response_upgrade->file_len, sse_0x05_response_upgrade->offset, sse_0x05_response_upgrade->data, 256);

	if(ret != 0) {
		net_client_data_ctx->upgrade_status = 0;
	} else {
		net_client_data_ctx->upgrade_status = 1;
	}

	net_client_data_ctx->serial_upgrade = sse_frame_header->serial;
	net_client_data_ctx->upgrade_offset = sse_0x05_response_upgrade->offset;

	net_client_data_ctx->device_cmd_ctx[item->cmd].state = COMMAND_STATE_REQUEST;
	return ret;
}

static int timeout_callback_upgrade(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	return ret;
}

static net_client_command_item_t net_client_command_item_upgrade = {
	.cmd = NET_CLIENT_DEVICE_COMMAND_UPGRADE,
	.frame = 0x05,
	.request_callback = request_callback_upgrade,
	.response_callback = response_callback_upgrade,
	.timeout_callback = timeout_callback_upgrade,
};

static net_client_command_item_t *net_client_command_item_device_table[] = {
	&net_client_command_item_report,
	&net_client_command_item_event_fault,
	&net_client_command_item_event_upload_record,
	&net_client_command_item_query_device_info,
	&net_client_command_item_query_device_message,
	&net_client_command_item_query_card_account,
	&net_client_command_item_settings,
	&net_client_command_item_remote_device,
	&net_client_command_item_upgrade,
};

static int request_callback_event_start(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x01_request_event_t *sse_0x01_request_event = (sse_0x01_request_event_t *)(sse_frame_header + 1);
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &channels_info->channels_settings;
	channel_info_t *channel_info = channels_info->channel_info + channel_id;
	charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_request_event_start_charge_t *sse_request_event_start_charge = (sse_request_event_start_charge_t *)sse_0x01_request_event->event_info;
	char dt[20];
	struct tm *tm;
	size_t size = (uint8_t *)(sse_request_event_start_charge + 1) - (uint8_t *)sse_0x01_request_event;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	snprintf((char *)sse_0x01_request_event->device_id, 32, "%s", app_info->mechine_info.device_id);
	sse_0x01_request_event->device_type = get_device_type(channels_info);
	sse_0x01_request_event->float_percision = (channels_info->channels_settings.magnification == 0) ? 2 : 3;
	sse_0x01_request_event->event_type = 0x00;

	sse_request_event_start_charge->channel_id = channel_id + 1;
	memcpy(sse_request_event_start_charge->vin, charger_info->bms_data.brm_data.vin, 17);
	sse_request_event_start_charge->chm_bms_version = get_u16_from_u8_lh(charger_info->bms_data.chm_data.version_1, charger_info->bms_data.chm_data.version_0);
	sse_request_event_start_charge->brm_battery_type = charger_info->bms_data.brm_data.brm_data.battery_type;
	sse_request_event_start_charge->bcp_rate_total_power = charger_info->bms_data.bcp_data.rate_total_power;
	sse_request_event_start_charge->bcp_total_voltage = charger_info->bms_data.bcp_data.total_voltage;
	sse_request_event_start_charge->bcp_max_charge_voltage_single_battery = charger_info->bms_data.bcp_data.max_charge_voltage_single_battery;
	sse_request_event_start_charge->bcp_max_temperature = charger_info->bms_data.bcp_data.max_temperature;
	sse_request_event_start_charge->bcp_max_charge_voltage = charger_info->bms_data.bcp_data.max_charge_voltage;
	snprintf((char *)sse_request_event_start_charge->card_id, 32, "%lu", (uint32_t)channel_info->channel_record_item.card_id);
	sse_request_event_start_charge->start_type = channel_info->channel_record_item.start_reason;
	memset(sse_request_event_start_charge->start_dt, 0xff, sizeof(sse_request_event_start_charge->start_dt));
	tm = localtime(&channel_info->channel_record_item.start_time);
	strftime(dt, sizeof(dt), "%Y%m%d%H%M%S", tm);
	ascii_to_bcd(dt, strlen(dt), sse_request_event_start_charge->start_dt, sizeof(sse_request_event_start_charge->start_dt));
	sse_request_event_start_charge->telemeter_total = channel_info->channel_record_item.start_total_energy;
	sse_request_event_start_charge->withholding = channel_info->channel_record_item.withholding;
	sse_request_event_start_charge->soc = charger_info->bms_data.bcp_data.soc / 10;
	channel_data_ctx->serial_event_start = net_client_data_ctx->serial++;

	send_frame(net_client_info, channel_data_ctx->serial_event_start, item->frame, 0, (uint8_t *)sse_0x01_request_event, size);

	channel_data_ctx->channel_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_event_start(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x01_response_event_t *sse_0x01_response_event = (sse_0x01_response_event_t *)(sse_frame_header + 1);
	//channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &channels_info->channels_settings;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	if(sse_frame_header->cmd.type == 0) {//非回复,忽略
		ret = 1;
		return ret;
	}

	if(sse_0x01_response_event->event_type != 0x00) {//非启动事件回复,忽略
		ret = 1;
		return ret;
	}

	if(channel_data_ctx->serial_event_start != sse_frame_header->serial) {
		ret = 1;
		return ret;
	}

	if(sse_0x01_response_event->status == 0) {
	}

	if(strncmp((const char *)sse_0x01_response_event->device_id, (const char *)app_info->mechine_info.device_id, 32) == 0) {//设备号不对,返回出错
		return ret;
	}

	channel_data_ctx->channel_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int timeout_callback_event_start(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;
	command_status_t *channel_cmd_ctx = channel_data_ctx->channel_cmd_ctx;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;

	if(channel_data_ctx->retry < 3) {
		channel_data_ctx->retry++;
		channel_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	} else {
		debug("channel %d start event failed!!!", channel_id);
	}

	return ret;
}

static net_client_command_item_t net_client_command_item_event_start = {
	.cmd = NET_CLIENT_CHANNEL_COMMAND_EVENT_START,
	.frame = 0x01,
	.request_callback = request_callback_event_start,
	.response_callback = response_callback_event_start,
	.timeout_callback = timeout_callback_event_start,
};

static int request_callback_query_qr_code(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x02_request_query_t *sse_0x02_request_query = (sse_0x02_request_query_t *)(sse_frame_header + 1);
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	//channel_info_t *channel_info = channels_info->channel_info + channel_id;
	//charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_query_qr_code_info_t *sse_query_qr_code_info = (sse_query_qr_code_info_t *)sse_0x02_request_query->query;
	size_t size = (uint8_t *)(sse_query_qr_code_info + 1) - (uint8_t *)sse_0x02_request_query;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	sse_0x02_request_query->type = 3;
	sse_0x02_request_query->id = 0;

	snprintf((char *)sse_query_qr_code_info->device_id, 32, "%s", app_info->mechine_info.device_id);
	sse_query_qr_code_info->channel_id = channel_id + 1;
	sse_query_qr_code_info->withholding = channels_settings->withholding;

	send_frame(net_client_info, net_client_data_ctx->serial++, item->frame, 0, (uint8_t *)sse_0x02_request_query, size);

	channel_data_ctx->channel_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_query_qr_code(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x02_request_query_t *sse_0x02_request_query = (sse_0x02_request_query_t *)(sse_frame_header + 1);
	//channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &channels_info->channels_settings;
	sse_query_qr_code_confirm_t *sse_query_qr_code_confirm = (sse_query_qr_code_confirm_t *)sse_0x02_request_query->query;
	uint8_t _channel_id = sse_query_qr_code_confirm->channel_id - 1;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;

	if(sse_frame_header->cmd.type == 0) {//非回复,忽略
		ret = 1;
		return ret;
	}

	if(sse_0x02_request_query->type != 3) {
		ret = 1;
		return ret;
	}

	if(sse_0x02_request_query->id != 0) {
		ret = 1;
		return ret;
	}

	if(_channel_id != channel_id) {
		ret = 1;
		return ret;
	}

	channel_data_ctx->channel_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int timeout_callback_query_qr_code(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	//net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;
	//command_status_t *channel_cmd_ctx = channel_data_ctx->channel_cmd_ctx;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;

	return ret;
}

static net_client_command_item_t net_client_command_item_query_qr_code = {
	.cmd = NET_CLIENT_CHANNEL_COMMAND_QUERY_QR_CODE,
	.frame = 0x02,
	.request_callback = request_callback_query_qr_code,
	.response_callback = response_callback_query_qr_code,
	.timeout_callback = timeout_callback_query_qr_code,
};

static int request_callback_remote_start_stop(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)send_buffer;
	sse_0x02_request_query_t *sse_0x02_request_query = (sse_0x02_request_query_t *)(sse_frame_header + 1);
	channels_info_t *channels_info = net_client_data_ctx->channels_info;
	channels_settings_t *channels_settings = &channels_info->channels_settings;
	//channel_info_t *channel_info = channels_info->channel_info + channel_id;
	//charger_info_t *charger_info = (charger_info_t *)channel_info->charger_info;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_query_qr_code_info_t *sse_query_qr_code_info = (sse_query_qr_code_info_t *)sse_0x02_request_query->query;
	size_t size = (uint8_t *)(sse_query_qr_code_info + 1) - (uint8_t *)sse_0x02_request_query;
	app_info_t *app_info = get_app_info();

	OS_ASSERT(app_info != NULL);

	sse_0x02_request_query->type = 3;
	sse_0x02_request_query->id = 0;

	snprintf((char *)sse_query_qr_code_info->device_id, 32, "%s", app_info->mechine_info.device_id);
	sse_query_qr_code_info->channel_id = channel_id + 1;
	sse_query_qr_code_info->withholding = channels_settings->withholding;

	send_frame(net_client_info, net_client_data_ctx->serial++, item->frame, 0, (uint8_t *)sse_0x02_request_query, size);

	channel_data_ctx->channel_cmd_ctx[item->cmd].state = COMMAND_STATE_RESPONSE;

	return ret;
}

static int response_callback_remote_start_stop(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id, uint8_t *request, uint16_t request_size, uint8_t *send_buffer, uint16_t send_buffer_size)
{
	int ret = -1;
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)request;
	net_client_command_item_t *item = (net_client_command_item_t *)_command_item;
	sse_0x04_response_remote_t *sse_0x04_response_remote = (sse_0x04_response_remote_t *)(sse_frame_header + 1);
	//channels_info_t *channels_info = net_client_data_ctx->channels_info;
	//channels_settings_t *channels_settings = &channels_info->channels_settings;
	sse_remote_start_stop_t *sse_remote_start_stop = (sse_remote_start_stop_t *)sse_0x04_response_remote->remote;
	uint8_t _channel_id = sse_remote_start_stop->channel_id - 1;
	net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;

	if(sse_frame_header->cmd.type == 1) {
		ret = 1;
		return ret;
	}

	if(sse_0x04_response_remote->type != 0) {
		ret = 1;
		return ret;
	}

	if(_channel_id != channel_id) {
		ret = 1;
		return ret;
	}

	switch(sse_remote_start_stop->cmd) {
		case 0: {//stop
		}
		break;

		case 1: {//lock
		}
		break;

		case 2: {//unlock
		}
		break;

		case 3: {//start
			switch(sse_remote_start_stop->mode) {
				case 0: {//充满为止
				}
				break;

				case 1: {//按金额充
				}
				break;

				case 2: {//按时间充
				}
				break;

				case 3: {//按电量充 0.01kwh
				}
				break;

				case 4: {//vin
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

	channel_data_ctx->channel_cmd_ctx[item->cmd].state = COMMAND_STATE_IDLE;
	ret = 0;
	return ret;
}

static int timeout_callback_remote_start_stop(net_client_info_t *net_client_info, void *_command_item, uint8_t channel_id)
{
	int ret = 0;
	//net_client_channel_data_ctx_t *channel_data_ctx = net_client_data_ctx->channel_data_ctx + channel_id;
	//command_status_t *channel_cmd_ctx = channel_data_ctx->channel_cmd_ctx;
	//net_client_command_item_t *item = (net_client_command_item_t *)_command_item;

	return ret;
}

static net_client_command_item_t net_client_command_item_remote_start_stop = {
	.cmd = NET_CLIENT_CHANNEL_COMMAND_REMOTE_START_STOP,
	.frame = 0x04,
	.request_callback = request_callback_remote_start_stop,
	.response_callback = response_callback_remote_start_stop,
	.timeout_callback = timeout_callback_remote_start_stop,
};

static net_client_command_item_t *net_client_command_item_channel_table[] = {
	&net_client_command_item_event_start,
	&net_client_command_item_query_qr_code,
	&net_client_command_item_remote_start_stop,
};

static char *get_net_client_cmd_device_des(net_client_device_command_t cmd)
{
	char *des = "unknow";

	switch(cmd) {
			add_des_case(NET_CLIENT_DEVICE_COMMAND_REPORT);
			add_des_case(NET_CLIENT_DEVICE_COMMAND_EVENT_FAULT);
			add_des_case(NET_CLIENT_DEVICE_COMMAND_EVENT_UPLOAD_RECORD);
			add_des_case(NET_CLIENT_DEVICE_COMMAND_QUERY_DEVICE_INFO);
			add_des_case(NET_CLIENT_DEVICE_COMMAND_QUERY_DEVICE_MESSAGE);
			add_des_case(NET_CLIENT_DEVICE_COMMAND_QUERY_CARD_ACCOUNT);
			add_des_case(NET_CLIENT_DEVICE_COMMAND_SETTINGS);
			add_des_case(NET_CLIENT_DEVICE_COMMAND_REMOTE_DEVICE);
			add_des_case(NET_CLIENT_DEVICE_COMMAND_UPGRADE);

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

			add_des_case(NET_CLIENT_CHANNEL_COMMAND_EVENT_START);
			add_des_case(NET_CLIENT_CHANNEL_COMMAND_QUERY_QR_CODE);
			add_des_case(NET_CLIENT_CHANNEL_COMMAND_REMOTE_START_STOP);

		default: {
		}
		break;
	}

	return des;
}

static void sse_ctrl_cmd(void *_net_client_info, void *_ctrl_cmd_info)
{
	net_client_info_t *net_client_info = (net_client_info_t *)_net_client_info;
	net_client_ctrl_cmd_info_t *ctrl_cmd_info = (net_client_ctrl_cmd_info_t *)_ctrl_cmd_info;

	switch(ctrl_cmd_info->cmd) {
		case NET_CLIENT_CTRL_CMD_QUERY_ACCOUNT: {
			account_request_info_t *account_request_info = (account_request_info_t *)ctrl_cmd_info->args;
			OS_ASSERT(account_request_info != NULL);
			sse_query_account(net_client_info, account_request_info);
		}
		break;

		default: {
		}
		break;
	}
}

static void request_init(void *ctx)
{
	int i;
	net_client_info_t *net_client_info = (net_client_info_t *)ctx;

	if(net_client_data_ctx == NULL) {
		net_client_data_ctx = (net_client_data_ctx_t *)os_calloc(1, sizeof(net_client_data_ctx_t));
		OS_ASSERT(net_client_data_ctx != NULL);

		net_client_data_ctx->card_account_info_chain = alloc_callback_chain();
		OS_ASSERT(net_client_data_ctx->card_account_info_chain != NULL);

		net_client_data_ctx->channels_info = get_channels();
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

	remove_callback(net_client_info->net_client_ctrl_cmd_chain, &net_client_info->net_client_ctrl_cmd_callback_item);
	net_client_info->net_client_ctrl_cmd_callback_item.fn = sse_ctrl_cmd;
	net_client_info->net_client_ctrl_cmd_callback_item.fn_ctx = net_client_info;
	OS_ASSERT(register_callback(net_client_info->net_client_ctrl_cmd_chain, &net_client_info->net_client_ctrl_cmd_callback_item) == 0);
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

static void request_parse(void *ctx, char *buffer, size_t size, size_t max_request_size, char **prequest, size_t *prequest_size)
{
	sse_frame_header_t *sse_frame_header = (sse_frame_header_t *)buffer;
	sse_frame_crc_t *sse_frame_crc;

	*prequest = NULL;
	*prequest_size = 0;

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
	*prequest_size = sse_frame_header->frame_len;
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

	do {
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

			ret = item->response_callback(net_client_info, item, 0, request, request_size, send_buffer, send_buffer_size);

			if(ret != 0) {
				if(ret == 1) {//ignore
				} else {
					debug("device cmd %d(%s) response error!", item->cmd, get_net_client_cmd_device_des(item->cmd));
					handled = 1;
				}
			} else {
				debug("device cmd:%d(%s) response", item->cmd, get_net_client_cmd_device_des(item->cmd));
				handled = 1;
			}
		}

		if(handled == 1) {
			break;
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

				ret = item->response_callback(net_client_info, item, j, request, request_size, send_buffer, send_buffer_size);

				if(ret != 0) {
					if(ret == 1) {
					} else {
						debug("channel %d cmd %d(%s) response error!", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
					}
				} else {
					debug("channel %d cmd:%d(%s) response", j, item->cmd, get_net_client_cmd_channel_des(item->cmd));
					handled = 1;
					break;
				}
			}

			if(handled == 1) {
				break;
			}
		}
	} while(0);

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

	sync_transaction_record();
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
