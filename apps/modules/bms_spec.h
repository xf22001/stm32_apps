

/*================================================================
 *
 *
 *   文件名称：bms_spec.h
 *   创 建 者：肖飞
 *   创建日期：2019年11月01日 星期五 09时51分27秒
 *   修改日期：2020年01月15日 星期三 09时53分48秒
 *   描    述：
 *
 *================================================================*/
#ifndef _BMS_SPEC_H
#define _BMS_SPEC_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#define BMS_GENERIC_TIMEOUT (5 * 1000)

#pragma pack(push, 1)

typedef enum {
	CHARGER_ADDR = 0x56,
	BMS_ADDR = 0xf4,
} bms_addr_t;

typedef enum {
	FN_INVALID = 0xff,

	FN_CHM = 0x26,
	FN_CHM_PRIORITY = 6,
	FN_CHM_SEND_PERIOD = 250,

	FN_BHM = 0x27,
	FN_BHM_PRIORITY = 6,
	FN_BHM_SEND_PERIOD = 250,

	FN_CRM = 0x01,
	FN_CRM_PRIORITY = 6,
	FN_CRM_SEND_PERIOD = 250,
	FN_CRM_TIMEOUT = 30 * 1000,

	FN_BRM = 0x02,
	FN_BRM_PRIORITY = 7,
	FN_BRM_SEND_PERIOD = 250,

	FN_BCP = 0x06,
	FN_BCP_PRIORITY = 7,
	FN_BCP_SEND_PERIOD = 500,

	FN_CTS = 0x07,
	FN_CTS_PRIORITY = 6,
	FN_CTS_SEND_PERIOD = 500,

	FN_CML = 0x08,
	FN_CML_PRIORITY = 6,
	FN_CML_SEND_PERIOD = 250,

	FN_BRO = 0x09,
	FN_BRO_PRIORITY = 4,
	FN_BRO_SEND_PERIOD = 250,
	FN_BRO_0xAA_TIMEOUT = 60 * 1000,

	FN_CRO = 0x0a,
	FN_CRO_PRIORITY = 4,
	FN_CRO_SEND_PERIOD = 250,
	FN_CRO_AA_TIMEOUT = 60 * 1000,

	FN_BCL = 0x10,
	FN_BCL_PRIORITY = 6,
	FN_BCL_SEND_PERIOD = 50,
	FN_BCL_TIMEOUT = 1000,

	FN_BCS = 0x11,
	FN_BCS_PRIORITY = 7,
	FN_BCS_SEND_PERIOD = 250,

	FN_CCS = 0x12,
	FN_CCS_PRIORITY = 6,
	FN_CCS_SEND_PERIOD = 50,
	FN_CCS_TIMEOUT = 1000,

	FN_BSM = 0x13,
	FN_BSM_PRIORITY = 6,
	FN_BSM_SEND_PERIOD = 250,

	FN_BMV = 0x15,
	FN_BMV_PRIORITY = 7,
	FN_BMV_SEND_PERIOD = (10 * 1000),

	FN_BMT = 0x16,
	FN_BMT_PRIORITY = 7,
	FN_BMT_SEND_PERIOD = (10 * 1000),

	FN_BSP = 0x17,
	FN_BSP_PRIORITY = 7,
	FN_BSP_SEND_PERIOD = (10 * 1000),

	FN_BST = 0x19,
	FN_BST_PRIORITY = 4,
	FN_BST_SEND_PERIOD = 10,

	FN_CST = 0x1a,
	FN_CST_PRIORITY = 4,
	FN_CST_SEND_PERIOD = 10,

	FN_BSD = 0x1c,
	FN_BSD_PRIORITY = 6,
	FN_BSD_SEND_PERIOD = 250,
	FN_BSD_TIMEOUT = (10 * 1000),

	FN_CSD = 0x1d,
	FN_CSD_PRIORITY = 6,
	FN_CSD_SEND_PERIOD = 250,

	FN_BEM = 0x1e,
	FN_BEM_PRIORITY = 2,
	FN_BEM_SEND_PERIOD = 250,

	FN_CEM = 0x1f,
	FN_CEM_PRIORITY = 2,
	FN_CEM_SEND_PERIOD = 250,

	FN_MULTI_REQUEST_PRIORITY = 7,
	FN_MULTI_REQUEST = 0xec,
	FN_MULTI_REQUEST_TYPE = 0x10,
	FN_MULTI_REQUEST_RESPONSE_TYPE = 0x11,

	FN_MULTI_DATA_PRIORITY = 7,
	FN_MULTI_DATA = 0xeb,
	FN_MULTI_DATA_RESPONSE_TYPE = 0x13,
} bms_fn_t;

typedef struct {
	uint8_t sa;
	uint8_t ps;
	uint8_t pf;
	uint8_t dp : 1;
	uint8_t r : 1;
	uint8_t p : 3;
	uint8_t reserved : 3;
} pdu_head_t;

typedef union {
	pdu_head_t pdu;
	uint32_t v;
} u_pdu_head_t;

typedef struct {
	uint8_t version_1;//0x01
	uint16_t version_0;//0x01
} chm_data_t;

typedef struct {
	uint16_t max_charge_voltage;//0.1v
} bhm_data_t;

typedef struct {
	uint8_t crm_result;//0x00-不能辨识，0xaa-能辨识
	uint32_t charger_sn;
} crm_data_t;

typedef struct {
	uint8_t version_1;//0x01
	uint16_t version_0;//0x01
	uint8_t battery_type;//0x01 : '铅酸电池', 0x02 : '镍氢电池', 0x03 : '磷酸电池', 0x04 : '锰酸锂电池', 0x05 : '钴酸锂电池', 0x06 : '三元材料电池', 0x07 : '聚合物电池', 0x08 : '钛酸锂电池', 0xff : '其他电池'
	uint16_t total_battery_rate_capicity;//0.1ah
	uint16_t total_battery_rate_voltage;//0.1v
} brm_data_t;

typedef struct {
	uint8_t serial;
	uint8_t day;
	uint8_t month;
	uint16_t year;
	uint8_t reserved[3];//0xffffff
} bms_version_t;

typedef struct {
	brm_data_t brm_data;
	uint8_t battery_vendor[4];
	uint32_t battery_vendor_sn;
	uint8_t battery_year;//1985+
	uint8_t battery_month;//1-12
	uint8_t battery_day;//1-31
	uint8_t battery_charge_times[3];
	uint8_t battery_property;//0-租赁，1-自有
	uint8_t reserved;
	uint8_t vin[17];
	bms_version_t version;
} brm_data_multi_t;

typedef struct {
	uint16_t max_charge_voltage_single_battery;//0.01v
	uint16_t max_charge_current;//0.1a -4000
	uint16_t rate_total_power;//0.1kwh
	uint16_t max_charge_voltage;//0.1v
	uint8_t max_temperature;// -50
	uint16_t soc;//0.1%
	uint16_t total_voltage;//0.1v
	uint8_t transfer_type;//0xff
} bcp_data_multi_t;

typedef struct {
	uint8_t S;
	uint8_t M;
	uint8_t H;
	uint8_t d;
	uint8_t m;
	uint16_t Y;
} cts_data_t;

typedef struct {
	uint16_t max_output_voltage;//0.1v
	uint16_t min_output_voltage;//0.1v
	uint16_t max_output_current;//0.1a -400
	uint16_t min_output_current;//0.1a -400
} cml_data_t;

typedef struct {
	uint8_t bro_result;//0x00-未准备好, 0xaa-准备好, 0xff-无效
} bro_data_t;

typedef struct {
	uint8_t cro_result;//0x00-未准备好, 0xaa-准备好, 0xff-无效
} cro_data_t;

typedef struct {
	uint16_t require_voltage;//0.1v
	uint16_t require_current;//0.1a -400
	uint8_t charge_mode;//0x01-恒压, 0x02-恒流
} bcl_data_t;

typedef struct {
	uint16_t charge_voltage;//0.1v
	uint16_t charge_current;//0.1a -400
	union {
		struct {
			uint16_t single_battery_max_voltage : 12;//0.01v 0-24v
			uint16_t single_battery_max_group : 4;//0-15
		} s;
		uint16_t v;
	} u1;
	uint8_t soc;//0-100%
	uint16_t remain_min;//0-600min
} bcs_data_t;

typedef struct {
	uint16_t output_voltage;//0.1v
	uint16_t output_current;//0.1a -400
	uint16_t total_charge_time;//1min 0-600
	union {
		struct {
			uint8_t charge_enable : 2;//0x00-暂停, 0x01-允许
		} s;
		uint8_t v;
	} u1;
} ccs_data_t;

typedef struct {
	uint8_t single_max_voltage_group;//1-256
	uint8_t battery_max_temperature;//1 -50
	uint8_t battery_max_temperature_sn;//1-128
	uint8_t battery_min_temperature;//1 -50
	uint8_t battery_min_temperature_sn;//1-128
	union {
		struct {
			uint8_t single_voltage_state : 2;//0x00-正常, 0x01-过高, 0x10-过低
			uint8_t total_soc_state : 2;//0x00-正常, 0x01-过高, 0x10-过低
			uint8_t battery_current_state : 2;//0x00-正常, 0x01-过流, 0x10-不可信状态
			uint8_t battery_temperature_state : 2;//0x00-正常, 0x01-过高, 0x10-不可信状态
		} s;
		uint8_t v;
	} u1;
	union {
		struct {
			uint8_t battery_insulation_state : 2;//0x00-正常, 0x01-不正常, 0x10-不可信状态
			uint8_t battery_connector_state : 2;//0x00-正常, 0x01-不正常, 0x10-不可信状态
			uint8_t battery_charge_enable : 2;//0x00-禁止, 0x01-允许, 0x10-不可信状态
		} s;
		uint8_t v;
	} u2;
} bsm_data_t;

typedef struct {
	uint16_t single_voltage[256];//单体动力电池电压 0.01v 0-24v
} bmv_data_t;

typedef struct {
	uint8_t single_temperature[128];//单体动力电池温度 -50
} bmt_data_t;

typedef struct {
	uint8_t battery_reserved[16];//单体动力电池温度 -50
} bsp_data_t;

typedef struct {
	union {
		struct {
			uint8_t stop_reason_soc : 2;//0x00-未达soc目标值, 0x01-达到soc目标值, 0x10-不可信状态
			uint8_t stop_reason_voltage : 2;//0x00-未达总电压目标值, 0x01-达到总电压目标值, 0x10-不可信状态
			uint8_t stop_reason_single_voltage : 2;//0x00-未达单体电压目标值, 0x01-达到单体电压目标值, 0x10-不可信状态
			uint8_t stop_reason_charger_stop : 2;//0x00-正常, 0x01-充电机终止, 0x10-不可信状态
		} s;
		uint8_t v;
	} u1;
	union {
		struct {
			uint16_t stop_fault_reason_insulation : 2;//0x00-正常, 0x01-故障, 0x10-不可信状态
			uint16_t stop_fault_reason_connector_temperature : 2;//0x00-正常, 0x01-故障, 0x10-不可信状态
			uint16_t stop_fault_reason_bms_connector_temperature : 2;//0x00-正常, 0x01-故障, 0x10-不可信状态
			uint16_t stop_fault_reason_charger_connector : 2;//0x00-正常, 0x01-故障, 0x10-不可信状态
			uint16_t stop_fault_reason_battery_temperature : 2;//0x00-正常, 0x01-温度过高, 0x10-不可信状态
			uint16_t stop_fault_reason_relay : 2;//0x00-正常, 0x01-故障, 0x10-不可信状态
			uint16_t stop_fault_reason_voltage_check : 2;//0x00-正常, 0x01-故障, 0x10-不可信状态
			uint16_t stop_fault_reason_other : 2;//0x00-正常, 0x01-故障, 0x10-不可信状态
		} s;
		uint16_t v;
	} u2;
	union {
		struct {
			uint8_t stop_error_reason_current : 2;//0x00-正常, 0x01-超过需求值, 0x10-不可信状态
			uint8_t stop_error_reason_voltage : 2;//0x00-正常, 0x01-电压异常, 0x10-不可信状态
		} s;
		uint8_t v;
	} u3;
} bst_data_t;

typedef struct {
	union {
		struct {
			uint8_t stop_reason_condition : 2;//0x00-正常, 0x01-达到设定条件中止, 0x10-不可信状态
			uint8_t stop_reason_manual : 2;//0x00-正常, 0x01-人工中止, 0x10-不可信状态
			uint8_t stop_reason_fault : 2;//0x00-正常, 0x01-故障中止, 0x10-不可信状态
			uint8_t stop_reason_bms_stop : 2;//0x00-正常, 0x01-bms终止, 0x10-不可信状态
		} s;
		uint8_t v;
	} u1;
	union {
		struct {
			uint16_t stop_fault_reason_temperature : 2;//0x00-正常, 0x01-过温, 0x10-不可信状态
			uint16_t stop_fault_reason_connector : 2;//0x00-正常, 0x01-连接器故障, 0x10-不可信状态
			uint16_t stop_fault_reason_inner_temperature : 2;//0x00-正常, 0x01-过温, 0x10-不可信状态
			uint16_t stop_fault_reason_charge : 2;//0x00-正常, 0x01-能量不能传送, 0x10-不可信状态
			uint16_t stop_fault_reason_emergency : 2;//0x00-正常, 0x01-应急故障, 0x10-不可信状态
			uint16_t stop_fault_reason_other : 2;//0x00-正常, 0x01-故障, 0x10-不可信状态
			uint8_t stop_error_reason_current : 2;//0x00-匹配, 0x01-不匹配, 0x10-不可信状态
			uint8_t stop_error_reason_voltage : 2;//0x00-正常, 0x01-电压异常, 0x10-不可信状态
		} s;
		uint16_t v;
	} u2;
} cst_data_t;

typedef struct {
	uint8_t soc;//0-100%
	uint16_t single_min_voltage;//0.01v 0-24v
	uint16_t single_max_voltage;//0.01v 0-24v
	uint8_t battery_min_temperature;// -50
	uint8_t battery_max_temperature;// -50
} bsd_data_t;

typedef struct {
	uint16_t total_charge_time;//1min 0-600
	uint16_t total_charge_energy;//0.1kwh 0-1000
	uint32_t battery_min_temperature;// -50
} csd_data_t;

typedef struct {
	union {
		struct {
			uint8_t crm_00_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t crm_aa_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t reserved_0 : 4;
		} s;
		uint8_t v;
	} u1;
	union {
		struct {
			uint8_t cts_cml_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t cro_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t reserved_1 : 4;
		} s;
		uint8_t v;
	} u2;
	union {
		struct {
			uint8_t ccs_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t cst_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t reserved_2 : 4;
		} s;
		uint8_t v;
	} u3;
	union {
		struct {
			uint8_t csd_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t other : 6;
		} s;
		uint8_t v;
	} u4;
} bem_data_t;

typedef struct {
	union {
		struct {
			uint8_t brm_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t reserved_0 : 6;
		} s;
		uint8_t v;
	} u1;
	union {
		struct {
			uint8_t bcp_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t bro_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t reserved_1 : 4;
		} s;
		uint8_t v;
	} u2;
	union {
		struct {
			uint8_t bcs_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t bcl_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t bst_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t reserved_2 : 2;
		} s;
		uint8_t v;
	} u3;
	union {
		struct {
			uint8_t bsd_timeout : 2;//0x00-正常, 0x01-超时, 0x10-不可信状态
			uint8_t other : 6;
		} s;
		uint8_t v;
	} u4;
} cem_data_t;

typedef struct {
	uint8_t type;//0x10-bms请求, 0x11-bms请求响应, 0x13-bms数据响应
} bms_multi_request_head_t;

typedef struct {
	bms_multi_request_head_t head;
	uint16_t bytes;
	uint8_t packets;
	uint8_t reserved_0;//0xff
	uint8_t reserved_1;//0x00
	uint8_t fn;
	uint8_t reserved_2;//0x00
} bms_multi_request_t;

typedef struct {
	bms_multi_request_head_t head;
	uint8_t packets;
	uint8_t packet_index;
	uint8_t reserved_0;//0xff
	uint8_t reserved_1;//0xff
	uint8_t reserved_2;//0x00
	uint8_t fn;
	uint8_t reserved_3;//0x00
} bms_multi_request_response_t;

typedef struct {
	bms_multi_request_head_t head;
	uint16_t bytes;
	uint8_t packets;
	uint8_t reserved_0;//0xff
	uint8_t reserved_1;//0x00
	uint8_t fn;
	uint8_t reserved_2;//0x00
} bms_multi_data_response_t;

typedef struct {
	uint8_t index;
	uint8_t data[7];
} bms_multi_data_t;

typedef struct {
	uint16_t common_max_charge_voltage;
	uint16_t common_soc;
} bms_data_common_t;

typedef struct {
	chm_data_t chm_data;
	bhm_data_t bhm_data;

	crm_data_t crm_data;
	brm_data_multi_t brm_data;

	bcp_data_multi_t bcp_data;
	cts_data_t cts_data;
	cml_data_t cml_data;

	bro_data_t bro_data;
	cro_data_t cro_data;

	bcl_data_t bcl_data;
	bcs_data_t bcs_data;
	ccs_data_t ccs_data;
	bsm_data_t bsm_data;

	bst_data_t bst_data;
	cst_data_t cst_data;

	bsd_data_t bsd_data;
	csd_data_t csd_data;
	bem_data_t bem_data;
	cem_data_t cem_data;

	bms_data_common_t bms_data_common;
} bms_data_settings_t;

#pragma pack(pop)

static inline uint32_t get_pdu_id(uint8_t priority, uint8_t fn, uint8_t dst, uint8_t src)
{
	u_pdu_head_t head;

	head.v = 0;
	head.pdu.p = priority;
	head.pdu.pf = fn;
	head.pdu.ps = dst;
	head.pdu.sa = src;

	return head.v;
}

#endif //_BMS_SPEC_H
