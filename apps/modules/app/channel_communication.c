

/*================================================================
 *
 *
 *   文件名称：channel_communication.c
 *   创 建 者：肖飞
 *   创建日期：2020年04月29日 星期三 12时22分44秒
 *   修改日期：2020年05月22日 星期五 09时46分06秒
 *   描    述：
 *
 *================================================================*/
#include "channel_communication.h"
#include <string.h>
#include "app.h"

#include "auxiliary_function_board.h"
#include "charger.h"

#define LOG_NONE
#include "log.h"

typedef struct {
	uint8_t cmd;
} cmd_common_t;

typedef struct {
	uint8_t gun_state : 1;//有无插枪
	uint8_t battery_available : 1;//车上电池继电器吸合状态，通过电池电压判断---a-f-b
	uint8_t output_state : 1;//输出继电器有没有吸合
	uint8_t adhesion_p : 1;//正继电器粘连---a-f-b
	uint8_t adhesion_n : 1;//负继电器粘连---a-f-b
	uint8_t gun_lock_state : 1;//枪上锁状态
	uint8_t bms_charger_enable : 1;//bms充电允许状态
	uint8_t a_f_b_state : 1;//辅助功能板连接状态
} cmd_1_b1_t;

typedef struct {
	uint8_t cmd;//1
	cmd_1_b1_t b1;
	uint8_t bms_state;//bms阶段
	uint8_t dc_p_temperature;//-20-220 +20偏移
	uint8_t dc_n_temperature;//-20-220 +20偏移
	uint8_t insulation_resistor_value;//0.1M欧每位
	uint8_t ver_h;//版本号h 程序版本
	uint8_t ver_l;//版本号l 程序版本
} cmd_1_t;//心跳

typedef struct {
	uint8_t test_mode : 1;//测试模式
	uint8_t precharge_enable : 1;//允许预充
	uint8_t fault : 1;//充电机故障
	uint8_t charger_power_on : 1;//充电机主板开机状态
	uint8_t manual : 1;//手动模式
	uint8_t adhesion_test : 1;//粘连检测
	uint8_t double_gun_one_car : 1;//双枪充一车
	uint8_t cp_ad : 1;//cp-ad采样
} cmd_101_b3_t;

typedef struct {
	uint8_t cmd;//101
	uint8_t charger_sn;//充电机编号
	uint8_t gb;//标准
	cmd_101_b3_t b3;
	uint8_t charger_output_voltage_l;//充电电压
	uint8_t charger_output_voltage_h;//充电电压
	uint8_t charger_output_current_l;//充电电流
	uint8_t charger_output_current_h;//充电电流
} cmd_101_t;//心跳回复

typedef struct {
	uint8_t cmd;//2
	uint8_t unused[7];
} cmd_2_t;//启动命令回复

typedef struct {
	uint8_t cmd;//102
	uint8_t auxiliary_power_type;//12-24v选择
	uint8_t charger_max_output_voltage_l;//最大输出电压
	uint8_t charger_max_output_voltage_h;//最大输出电压
	uint8_t charger_min_output_voltage_l;//最小输出电压
	uint8_t charger_min_output_voltage_h;//最小输出电压
	uint8_t charger_max_output_current_l;//最大输出电流
	uint8_t charger_max_output_current_h;//最大输出电流
} cmd_102_t;//启动命令

typedef struct {
	uint8_t cmd;//13
	uint8_t unused[7];
} cmd_13_t;//启动命令2回复

typedef struct {
	uint8_t cmd;//113
	uint8_t charger_min_output_current_l;//最小输出电流
	uint8_t charger_min_output_current_h;//最小输出电流
	uint8_t unused[5];
} cmd_113_t;//启动命令2

typedef struct {
	uint8_t door : 1;//门禁报警
	uint8_t stop : 1;//急停
} cmd_3_b4_t;

typedef struct {
	uint8_t cmd;//3
	uint8_t a_f_b_ver_h;//辅助功能板版本号h
	uint8_t a_f_b_ver_l;//辅助功能板版本号l
	uint8_t bms_status;//bms工作状态码
	cmd_3_b4_t b4;
	uint8_t unused[3];
} cmd_3_t;//心跳2

typedef struct {
	uint8_t cmd;//103
	uint8_t module_output_voltage_l;//模块充电电压
	uint8_t module_output_voltage_h;//模块充电电压
	uint8_t charnnel_max_output_power_l;//通道最大输出功率
	uint8_t charnnel_max_output_power_h;//通道最大输出功率
	uint8_t module_output_current_l;//模块充电电流
	uint8_t module_output_current_h;//模块充电电流
	uint8_t unused;
} cmd_103_t;//心跳2回复

typedef struct {
	uint8_t cmd;//4
	uint8_t precharge_voltage_l;//预充电电压
	uint8_t precharge_voltage_h;//预充电电压
	uint8_t precharge_action;//0-停止预充, 1-开始预充, 2-单模块预充
	uint8_t unused[4];
} cmd_4_t;//预充请求

typedef struct {
	uint8_t cmd;//104
	uint8_t unused[7];
} cmd_104_t;//预充请求回复

typedef struct {
	uint8_t cmd;//5
	uint8_t bms_version_h;//bms版本号 brm version_1
	uint8_t bms_version_l;//bms版本号 brm version_0
	uint8_t battery_type;//电池类型
	uint8_t total_battery_rate_capicity_l;//电池容量
	uint8_t total_battery_rate_capicity_h;//电池容量
	uint8_t total_battery_rate_voltage_l;//电池额定总电压
	uint8_t total_battery_rate_voltage_h;//电池额定总电压
} cmd_5_t;//上传bms数据 brm

typedef struct {
	uint8_t cmd;//105
	uint8_t unused[7];
} cmd_105_t;//上传bms数据回复

typedef struct {
	uint8_t cmd;//6
	uint8_t single_battery_max_voltage_l;//单体最高允许电压 max_charge_voltage_single_battery
	uint8_t single_battery_max_voltage_h;//单体最高允许电压 max_charge_voltage_single_battery
	uint8_t max_temperature;//最高允许温度
	uint8_t max_charge_voltage_l;//最高允许充电总电压
	uint8_t max_charge_voltage_h;//最高允许充电总电压
	uint8_t total_voltage_l;//电池总电压
	uint8_t total_voltage_h;//电池总电压
} cmd_6_t;//上传bms数据2 bcp

typedef struct {
	uint8_t cmd;//106
	uint8_t unused[7];
} cmd_106_t;//上传bms数据回复2

typedef struct {
	uint8_t cmd;//7
	uint8_t vin[7];//0-6
} cmd_7_t;//vin

typedef struct {
	uint8_t cmd;//107
	uint8_t unused[7];
} cmd_107_t;//vin回复

typedef struct {
	uint8_t cmd;//8
	uint8_t vin[7];//7-13
} cmd_8_t;//vin2

typedef struct {
	uint8_t cmd;//108
	uint8_t unused[7];
} cmd_108_t;//vin回复2

typedef struct {
	uint8_t cmd;//9
	uint8_t vin[3];//14-16
	uint8_t unused[4];
} cmd_9_t;//vin3

typedef struct {
	uint8_t cmd;//109
	uint8_t unused[7];
} cmd_109_t;//vin回复3

typedef struct {
	uint8_t cmd;//10
	uint8_t require_voltage_l;//需求电压
	uint8_t require_voltage_h;//需求电压
	uint8_t require_current_l;//需求电流
	uint8_t require_current_h;//需求电流
	uint8_t soc;//soc
	uint8_t single_battery_max_voltage_l;//单体最高电压
	uint8_t single_battery_max_voltage_h;//单体最高电压
} cmd_10_t;//定时上报数据1 bcl

typedef struct {
	uint8_t cmd;//110
	uint8_t output_voltage_l;//输出电压
	uint8_t output_voltage_h;//输出电压
	uint8_t output_current_l;//输出电流
	uint8_t output_current_h;//输出电流
	uint8_t total_charge_time_l;//累计充电时间
	uint8_t total_charge_time_h;//累计充电时间
	uint8_t unused;
} cmd_110_t;//定时上报数据1回复 ccs

typedef struct {
	uint8_t cmd;//11
	uint8_t charge_voltage_l;//充电电压测量值
	uint8_t charge_voltage_h;//充电电压测量值
	uint8_t charge_current_l;//充电电流测量值
	uint8_t charge_current_h;//充电电流测量值
	uint8_t remain_min_l;//剩余充电时间
	uint8_t battery_max_temperature;//电池最高温度
	uint8_t remain_min_h;//剩余充电时间
} cmd_11_t;//定时上报数据2 bcs bsm

typedef struct {
	uint8_t cmd;//111
	uint8_t charger_output_energy_l;//输出总度数
	uint8_t charger_output_energy_h;//输出总度数
	uint8_t unused[5];
} cmd_111_t;//定时上报数据2 bcs bsm

typedef struct {
	uint8_t cmd;//20
	uint8_t unused[7];
} cmd_20_t;//辅板回复

typedef struct {
	uint8_t cmd;//120
	uint8_t unused[7];
} cmd_120_t;//打开辅板输出继电器

typedef struct {
	uint8_t cmd;//21
	uint8_t unused[7];
} cmd_21_t;//辅板回复

typedef struct {
	uint8_t cmd;//121
	uint8_t unused[7];
} cmd_121_t;//锁定辅板电子锁

typedef struct {
	uint8_t cmd;//22
	uint8_t unused[7];
} cmd_22_t;//辅板回复

typedef struct {
	uint8_t cmd;//122
	uint8_t unused[7];
} cmd_122_t;//解除辅板电子锁

typedef struct {
	uint8_t cmd;//25
	uint8_t unused[7];
} cmd_25_t;//通道主动开机命令

typedef struct {
	uint8_t cmd;//125
	uint8_t unused[7];
} cmd_125_t;//回复

typedef struct {
	uint8_t cmd;//30
	uint8_t unused[7];
} cmd_30_t;//通道主动停机命令

typedef struct {
	uint8_t cmd;//130
	uint8_t unused[7];
} cmd_130_t;//回复

typedef struct {
	uint8_t cmd;//50
	uint8_t unused[7];
} cmd_50_t;//辅板回复

typedef struct {
	uint8_t cmd;//150
	uint8_t reason;//停机原因
	uint8_t unused[6];
} cmd_150_t;//发送停机命令

typedef struct {
	uint8_t cmd;//51
	uint8_t unused[7];
} cmd_51_t;//辅板回复

typedef struct {
	uint8_t cmd;//151
	uint8_t unused[7];
} cmd_151_t;//关闭辅板输出继电器

typedef struct {
	uint8_t cmd;//60
	uint8_t brm_data[7];//0
} cmd_60_t;//brm 0

typedef struct {
	uint8_t cmd;//160
	uint8_t unused[7];
} cmd_160_t;//辅板回复

typedef struct {
	uint8_t cmd;//61
	uint8_t brm_data[7];//1
} cmd_61_t;//brm 1

typedef struct {
	uint8_t cmd;//161
	uint8_t unused[7];
} cmd_161_t;//辅板回复

typedef struct {
	uint8_t cmd;//62
	uint8_t brm_data[7];//2
} cmd_62_t;//brm 2

typedef struct {
	uint8_t cmd;//162
	uint8_t unused[7];
} cmd_162_t;//辅板回复

typedef struct {
	uint8_t cmd;//63
	uint8_t brm_data[7];//3
} cmd_63_t;//brm 3

typedef struct {
	uint8_t cmd;//163
	uint8_t unused[7];
} cmd_163_t;//辅板回复

typedef struct {
	uint8_t cmd;//64
	uint8_t brm_data[7];//4
} cmd_64_t;//brm 4

typedef struct {
	uint8_t cmd;//164
	uint8_t unused[7];
} cmd_164_t;//辅板回复

typedef struct {
	uint8_t cmd;//65
	uint8_t brm_data[7];//5
} cmd_65_t;//brm 5

typedef struct {
	uint8_t cmd;//165
	uint8_t unused[7];
} cmd_165_t;//辅板回复

typedef struct {
	uint8_t cmd;//66
	uint8_t brm_data[7];//6
} cmd_66_t;//brm 6

typedef struct {
	uint8_t cmd;//166
	uint8_t unused[7];
} cmd_166_t;//辅板回复

typedef struct {
	uint8_t cmd;//67
	uint8_t bcp_data[7];//0
} cmd_67_t;//bcp 0

typedef struct {
	uint8_t cmd;//167
	uint8_t unused[7];
} cmd_167_t;//辅板回复

typedef struct {
	uint8_t cmd;//68
	uint8_t bcp_data[7];//1
} cmd_68_t;//bcp 1

typedef struct {
	uint8_t cmd;//168
	uint8_t unused[7];
} cmd_168_t;//辅板回复

typedef struct {
	uint8_t cmd;//69
	uint8_t bcs_data[7];//0
} cmd_69_t;//bcs 1

typedef struct {
	uint8_t cmd;//169
	uint8_t unused[7];
} cmd_169_t;//辅板回复

typedef struct {
	uint8_t cmd;//70
	uint8_t bcs_data[2];//1
	uint8_t bcl_data[5];//0
} cmd_70_t;//bcs_bcl 1 0

typedef struct {
	uint8_t cmd;//170
	uint8_t unused[7];
} cmd_170_t;//辅板回复

typedef struct {
	uint8_t cmd;//71
	uint8_t bsm_data[7];//0
} cmd_71_t;//bsm 0

typedef struct {
	uint8_t cmd;//171
	uint8_t unused[7];
} cmd_171_t;//辅板回复

typedef struct {
	uint8_t cmd;//72
	uint8_t bst_data[4];//0
	uint8_t unused[3];
} cmd_72_t;//bst 0

typedef struct {
	uint8_t cmd;//172
	uint8_t unused[7];
} cmd_172_t;//辅板回复

typedef struct {
	uint8_t cmd;//73
	uint8_t bsd_data[7];//0
} cmd_73_t;//bsd 0

typedef struct {
	uint8_t cmd;//173
	uint8_t unused[7];
} cmd_173_t;//辅板回复

typedef int (*channel_com_request_callback_t)(channel_com_info_t *channel_com_info);
typedef int (*channel_com_response_callback_t)(channel_com_info_t *channel_com_info);

typedef struct {
	channel_com_cmd_t cmd;
	uint32_t request_period;
	uint8_t request_code;
	channel_com_request_callback_t request_callback;
	uint8_t response_code;
	channel_com_response_callback_t response_callback;
} channel_com_command_item_t;


static LIST_HEAD(channel_com_info_list);
static osMutexId channel_com_info_list_mutex = NULL;

static channel_com_info_t *get_channel_com_info(channel_info_config_t *channel_info_config)
{
	channel_com_info_t *channel_com_info = NULL;
	channel_com_info_t *channel_com_info_item = NULL;
	osStatus os_status;

	if(channel_com_info_list_mutex == NULL) {
		return channel_com_info;
	}

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(channel_com_info_item, &channel_com_info_list, channel_com_info_t, list) {
		if(channel_com_info_item->can_info->hcan == channel_info_config->hcan_com) {
			channel_com_info = channel_com_info_item;
			break;
		}
	}

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	return channel_com_info;
}

void free_channel_com_info(channel_com_info_t *channel_com_info)
{
	osStatus os_status;

	if(channel_com_info == NULL) {
		return;
	}

	if(channel_com_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&channel_com_info->list);

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	os_free(channel_com_info);
}

static void charger_info_report_status_cb(void *fn_ctx, void *chain_ctx)
{
	channel_com_info_t *channel_com_info = (channel_com_info_t *)fn_ctx;
	charger_report_status_t *charger_report_status = (charger_report_status_t *)chain_ctx;

	_printf("%s:%s:%d state:%s, status:%d\n",
	        __FILE__, __func__, __LINE__,
	        get_charger_state_des(charger_report_status->state),
	        charger_report_status->status);

	switch(charger_report_status->state) {
		case CHARGER_STATE_IDLE: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_5_105].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_6_106].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_7_107].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_60_160].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_10_110].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_11_111].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_67_167].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_71_171].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_72_172].available = 0;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_73_173].available = 0;
			channel_com_info->bms_status = RETURN_ERROR;
		}
		break;

		case CHARGER_STATE_CHM: {
			channel_com_info->bms_status = BMS_STARTING;
		}
		break;

		case CHARGER_STATE_CRM: {
			channel_com_info->bms_status = BMS_STARTING;
		}
		break;

		case CHARGER_STATE_CTS_CML: {
		}
		break;

		case CHARGER_STATE_CRO: {
		}
		break;

		case CHARGER_STATE_CCS: {
			channel_com_info->bms_status = BMS_SUCCESS;
		}
		break;

		case CHARGER_STATE_CST: {
		}
		break;

		case CHARGER_STATE_CSD_CEM: {
		}
		break;

		default:
			break;
	}

	switch(charger_report_status->status) {
		case CHARGER_INFO_STATUS_NONE: {
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH: {
			channel_com_info->bms_status = BMS_U_UNMATCH;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT: {
			channel_com_info->bms_status = RETURN_DISCHARGE_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT: {
			channel_com_info->bms_status = RETURN_INC_BMS_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT: {
			channel_com_info->bms_status = RETURN_DISCHARGE_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_BRM_TIMEOUT: {
			channel_com_info->bms_status = BRM_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_BCP_TIMEOUT: {
			channel_com_info->bms_status = BCP_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_BRO_TIMEOUT: {
			channel_com_info->bms_status = BRO_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH: {
			channel_com_info->bms_status = BMS_U_UNMATCH;
		}
		break;

		case CHARGER_INFO_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT: {
			channel_com_info->bms_status = PRECHARGE_ERROR;
		}
		break;

		case CHARGER_INFO_STATUS_BCL_TIMEOUT: {
			channel_com_info->bms_status = BCL_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_BCS_TIMEOUT: {
			channel_com_info->bms_status = BCS_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT: {
			channel_com_info->bms_status = RETURN_DISCHARGE_TIMEOUT;
		}
		break;

		case CHARGER_INFO_STATUS_CST: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_30_130].state = CHANNEL_COM_STATE_REQUEST;
		}
		break;

		case CHARGER_INFO_STATUS_BRM_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_5_105].available = 1;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_7_107].available = 1;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_60_160].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCP_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_6_106].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCL_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_10_110].available = 1;
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_67_167].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BSM_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_71_171].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BCS_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_11_111].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BST_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_72_172].available = 1;
		}
		break;

		case CHARGER_INFO_STATUS_BSD_RECEIVED: {
			channel_com_info->cmd_ctx[CHANNEL_COM_CMD_73_173].available = 1;
		}
		break;

		default:
			break;
	}

}

static int channel_com_info_set_channel_config(channel_com_info_t *channel_com_info, channel_info_config_t *channel_info_config)
{
	int ret = -1;
	can_info_t *can_info;
	a_f_b_info_t *a_f_b_info;
	charger_info_t *charger_info;

	can_info = get_or_alloc_can_info(channel_info_config->hcan_com);

	if(can_info == NULL) {
		return ret;
	}

	channel_com_info->can_info = can_info;

	charger_info = get_or_alloc_charger_info(channel_info_config);

	if(charger_info == NULL) {
		return ret;
	}

	channel_com_info->charger_info = charger_info;

	channel_com_info->charger_info_report_status_cb.fn = charger_info_report_status_cb;
	channel_com_info->charger_info_report_status_cb.fn_ctx = channel_com_info;
	add_charger_info_report_status_cb(charger_info, &channel_com_info->charger_info_report_status_cb);

	a_f_b_info = get_or_alloc_a_f_b_info(channel_info_config);

	if(a_f_b_info == NULL) {
		return ret;
	}

	channel_com_info->a_f_b_info = a_f_b_info;

	ret = 0;
	return ret;
}

channel_com_info_t *get_or_alloc_channel_com_info(channel_info_config_t *channel_info_config)
{
	channel_com_info_t *channel_com_info = NULL;
	osStatus os_status;

	channel_com_info = get_channel_com_info(channel_info_config);

	if(channel_com_info != NULL) {
		return channel_com_info;
	}

	if(channel_com_info_list_mutex == NULL) {
		osMutexDef(channel_com_info_list_mutex);
		channel_com_info_list_mutex = osMutexCreate(osMutex(channel_com_info_list_mutex));

		if(channel_com_info_list_mutex == NULL) {
			return channel_com_info;
		}
	}

	channel_com_info = (channel_com_info_t *)os_alloc(sizeof(channel_com_info_t));

	if(channel_com_info == NULL) {
		return channel_com_info;
	}

	memset(channel_com_info, 0, sizeof(channel_com_info_t));

	channel_com_info->channel_info_config = channel_info_config;

	os_status = osMutexWait(channel_com_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&channel_com_info->list, &channel_com_info_list);

	os_status = osMutexRelease(channel_com_info_list_mutex);

	if(os_status != osOK) {
	}

	if(channel_com_info_set_channel_config(channel_com_info, channel_info_config) != 0) {
		goto failed;
	}

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_1_101].available = 1;

	return channel_com_info;
failed:

	free_channel_com_info(channel_com_info);

	channel_com_info = NULL;

	return channel_com_info;
}

typedef enum {
	CHANNEL_COM_BMS_STATE_IDLE = 0,
	CHANNEL_COM_BMS_STATE_CONNECT,
	CHANNEL_COM_BMS_STATE_SHAKE_HAND,
	CHANNEL_COM_BMS_STATE_CONFIG,
	CHANNEL_COM_BMS_STATE_CHARGE,
	CHANNEL_COM_BMS_STATE_END,
	CHANNEL_COM_BMS_STATE_NONE,
	CHANNEL_COM_BMS_STATE_CTRL_TEST,
} channel_com_bms_state_t;

static uint8_t channel_com_get_charger_state(charger_info_t *charger_info)
{
	channel_com_bms_state_t channel_com_bms_state = CHANNEL_COM_BMS_STATE_IDLE;

	if(charger_info->manual == 1) {
		channel_com_bms_state = CHANNEL_COM_BMS_STATE_CTRL_TEST;
	} else {
		charger_state_t state = get_charger_state(charger_info);

		switch(state) {
			case CHARGER_STATE_IDLE: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_IDLE;
			}
			break;

			case CHARGER_STATE_CHM:
			case CHARGER_STATE_CRM: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_SHAKE_HAND;
			}
			break;

			case CHARGER_STATE_CTS_CML:
			case CHARGER_STATE_CRO: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_CONFIG;
			}
			break;

			case CHARGER_STATE_CCS: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_CHARGE;
			}
			break;

			case CHARGER_STATE_CST:
			case CHARGER_STATE_CSD_CEM: {
				channel_com_bms_state = CHANNEL_COM_BMS_STATE_END;
			}
			break;

			default:
				break;
		}
	}

	return channel_com_bms_state;
}

static int request_1_101(channel_com_info_t *channel_com_info)//500ms
{
	int ret = -1;
	cmd_1_t *cmd_1 = (cmd_1_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)channel_com_info->a_f_b_info;

	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(a_f_b_info);

	cmd_1->b1.gun_state = charger_info->gun_connect_state;
	cmd_1->b1.battery_available = get_battery_available_state(channel_com_info->a_f_b_info);
	cmd_1->b1.output_state = charger_info->power_output_state;
	cmd_1->b1.adhesion_p = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->running_state.adhesion_p : 0;
	cmd_1->b1.adhesion_n = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->running_state.adhesion_n : 0;
	cmd_1->b1.gun_lock_state = charger_info->gun_lock_state;
	cmd_1->b1.bms_charger_enable = charger_info->settings->ccs_data.u1.s.charge_enable;
	cmd_1->b1.a_f_b_state = get_a_f_b_connect_state(channel_com_info->a_f_b_info);

	cmd_1->bms_state = channel_com_get_charger_state(charger_info);
	cmd_1->dc_p_temperature = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->dc_p_temperature : 0;
	cmd_1->dc_n_temperature = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->dc_n_temperature : 0;
	cmd_1->insulation_resistor_value = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->insulation_resistor_value : 0;
	cmd_1->ver_h = VER_MAJOR;
	cmd_1->ver_h = VER_MINOR;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_1_101].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_1_101(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_101_t *cmd_101 = (cmd_101_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->settings->crm_data.charger_sn = cmd_101->charger_sn;
	charger_info->gb = cmd_101->gb;
	charger_info->test_mode = cmd_101->b3.test_mode;
	charger_info->precharge_enable = cmd_101->b3.precharge_enable;
	charger_info->fault = cmd_101->b3.fault;
	charger_info->charger_power_on = cmd_101->b3.charger_power_on;
	charger_info->manual = cmd_101->b3.manual;
	charger_info->adhesion_test = cmd_101->b3.adhesion_test;
	charger_info->double_gun_one_car = cmd_101->b3.double_gun_one_car;
	charger_info->cp_ad = cmd_101->b3.cp_ad;
	charger_info->charger_output_voltage = get_u16_from_u8_lh(cmd_101->charger_output_voltage_l, cmd_101->charger_output_voltage_h);
	charger_info->charger_output_current = get_u16_from_u8_lh(cmd_101->charger_output_current_l, cmd_101->charger_output_current_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_1_101].state = CHANNEL_COM_STATE_IDLE;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_3_103].state = CHANNEL_COM_STATE_REQUEST;


	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_1_101 = {
	.cmd = CHANNEL_COM_CMD_1_101,
	.request_period = 500,
	.request_code = 1,
	.request_callback = request_1_101,
	.response_code = 101,
	.response_callback = response_1_101,
};

static int request_2_102(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	set_charger_control_state(charger_info, BMS_CONTROL_STATE_START);
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_2_102].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_2_102(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_102_t *cmd_102 = (cmd_102_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->auxiliary_power_type = cmd_102->auxiliary_power_type;

	charger_info->settings->cml_data.max_output_voltage =
	    get_u16_from_u8_lh(cmd_102->charger_max_output_voltage_l, cmd_102->charger_max_output_voltage_h);

	charger_info->settings->cml_data.min_output_voltage =
	    get_u16_from_u8_lh(cmd_102->charger_min_output_voltage_l, cmd_102->charger_min_output_voltage_h);

	charger_info->settings->cml_data.max_output_current =
	    get_u16_from_u8_lh(cmd_102->charger_max_output_current_l, cmd_102->charger_max_output_current_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_2_102].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_2_102 = {
	.cmd = CHANNEL_COM_CMD_2_102,
	.request_period = 0,
	.request_code = 2,
	.request_callback = request_2_102,
	.response_code = 102,
	.response_callback = response_2_102,
};

static int request_13_113(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = 0;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_13_113].state = CHANNEL_COM_STATE_IDLE;

	return ret;
}

static int response_13_113(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_113_t *cmd_113 = (cmd_113_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->settings->cml_data.min_output_current =
	    get_u16_from_u8_lh(cmd_113->charger_min_output_current_l, cmd_113->charger_min_output_current_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_13_113].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_13_113 = {
	.cmd = CHANNEL_COM_CMD_13_113,
	.request_period = 0,
	.request_code = 13,
	.request_callback = request_13_113,
	.response_code = 113,
	.response_callback = response_13_113,
};

static int request_3_103(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_3_t *cmd_3 = (cmd_3_t *)channel_com_info->can_tx_msg.Data;
	a_f_b_info_t *a_f_b_info = (a_f_b_info_t *)channel_com_info->a_f_b_info;
	a_f_b_reponse_91_data_t *a_f_b_reponse_91_data = get_a_f_b_status_data(a_f_b_info);
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_3->a_f_b_ver_h = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->version.b1 : 0;
	cmd_3->a_f_b_ver_l = (a_f_b_reponse_91_data != NULL) ? a_f_b_reponse_91_data->version.b0 : 0;
	cmd_3->bms_status = channel_com_info->bms_status;
	cmd_3->b4.door = charger_info->door_state;
	cmd_3->b4.stop = charger_info->error_stop_state;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_3_103].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_3_103(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_103_t *cmd_103 = (cmd_103_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->module_output_voltage = get_u16_from_u8_lh(cmd_103->module_output_voltage_l, cmd_103->module_output_voltage_h);
	charger_info->charnnel_max_output_power = get_u16_from_u8_lh(cmd_103->charnnel_max_output_power_l, cmd_103->charnnel_max_output_power_h);
	charger_info->module_output_current = get_u16_from_u8_lh(cmd_103->module_output_current_l, cmd_103->module_output_current_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_3_103].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_3_103 = {
	.cmd = CHANNEL_COM_CMD_3_103,
	.request_period = 0,
	.request_code = 3,
	.request_callback = request_3_103,
	.response_code = 103,
	.response_callback = response_3_103,
};

void request_precharge(channel_com_info_t *channel_com_info)
{
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_4_104].state = CHANNEL_COM_STATE_REQUEST;
}

static int request_4_104(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_4_t *cmd_4 = (cmd_4_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_4->precharge_voltage_l = get_u8_l_from_u16(charger_info->precharge_voltage);
	cmd_4->precharge_voltage_h = get_u8_h_from_u16(charger_info->precharge_voltage);
	cmd_4->precharge_action = charger_info->precharge_action;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_4_104].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_4_104(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_4_104].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_4_104 = {
	.cmd = CHANNEL_COM_CMD_4_104,
	.request_period = 0,
	.request_code = 4,
	.request_callback = request_4_104,
	.response_code = 104,
	.response_callback = response_4_104,
};

static int request_5_105(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;
	cmd_5_t *cmd_5 = (cmd_5_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_5->bms_version_l = charger_info->settings->brm_data.brm_data.version_1;
	cmd_5->bms_version_h = get_u8_l_from_u16(charger_info->settings->brm_data.brm_data.version_0);
	cmd_5->battery_type = charger_info->settings->brm_data.brm_data.battery_type;
	cmd_5->total_battery_rate_capicity_l = get_u8_l_from_u16(charger_info->settings->brm_data.brm_data.total_battery_rate_capicity);
	cmd_5->total_battery_rate_capicity_h = get_u8_h_from_u16(charger_info->settings->brm_data.brm_data.total_battery_rate_capicity);
	cmd_5->total_battery_rate_voltage_l = get_u8_l_from_u16(charger_info->settings->brm_data.brm_data.total_battery_rate_voltage);
	cmd_5->total_battery_rate_voltage_h = get_u8_h_from_u16(charger_info->settings->brm_data.brm_data.total_battery_rate_voltage);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_5_105].state = CHANNEL_COM_STATE_RESPONSE;
	ret = 0;

	return ret;
}

static int response_5_105(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_5_105].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_5_105 = {
	.cmd = CHANNEL_COM_CMD_5_105,
	.request_period = 200,
	.request_code = 5,
	.request_callback = request_5_105,
	.response_code = 105,
	.response_callback = response_5_105,
};

static int request_6_106(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BCP_RECEIVED
{
	int ret = -1;

	cmd_6_t *cmd_6 = (cmd_6_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_6->single_battery_max_voltage_l = get_u8_l_from_u16(charger_info->settings->bcp_data.max_charge_voltage_single_battery);
	cmd_6->single_battery_max_voltage_h = get_u8_h_from_u16(charger_info->settings->bcp_data.max_charge_voltage_single_battery);
	cmd_6->max_temperature = charger_info->settings->bcp_data.max_temperature;
	cmd_6->max_charge_voltage_l = get_u8_l_from_u16(charger_info->settings->bcp_data.max_charge_voltage);
	cmd_6->max_charge_voltage_h = get_u8_h_from_u16(charger_info->settings->bcp_data.max_charge_voltage);
	cmd_6->total_voltage_l = get_u8_l_from_u16(charger_info->settings->bcp_data.total_voltage);
	cmd_6->total_voltage_h = get_u8_h_from_u16(charger_info->settings->bcp_data.total_voltage);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_6_106].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_6_106(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_6_106].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_6_106 = {
	.cmd = CHANNEL_COM_CMD_6_106,
	.request_period = 200,
	.request_code = 6,
	.request_callback = request_6_106,
	.response_code = 106,
	.response_callback = response_6_106,
};

static int request_7_107(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;

	cmd_7_t *cmd_7 = (cmd_7_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	memcpy(cmd_7->vin, charger_info->settings->brm_data.vin + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_7_107].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_7_107(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_7_107].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_8_108].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_7_107 = {
	.cmd = CHANNEL_COM_CMD_7_107,
	.request_period = 200,
	.request_code = 7,
	.request_callback = request_7_107,
	.response_code = 107,
	.response_callback = response_7_107,
};

static int request_8_108(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_8_t *cmd_8 = (cmd_8_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	memcpy(cmd_8->vin, charger_info->settings->brm_data.vin + 7, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_8_108].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_8_108(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_8_108].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_9_109].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_8_108 = {
	.cmd = CHANNEL_COM_CMD_8_108,
	.request_period = 0,
	.request_code = 8,
	.request_callback = request_8_108,
	.response_code = 108,
	.response_callback = response_8_108,
};

static int request_9_109(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_9_t *cmd_9 = (cmd_9_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	memcpy(cmd_9->vin, charger_info->settings->brm_data.vin + 14, 3);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_9_109].state = CHANNEL_COM_STATE_RESPONSE;


	ret = 0;

	return ret;
}

static int response_9_109(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_9_109].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_9_109 = {
	.cmd = CHANNEL_COM_CMD_9_109,
	.request_period = 0,
	.request_code = 9,
	.request_callback = request_9_109,
	.response_code = 109,
	.response_callback = response_9_109,
};

static int request_10_110(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BCL_RECEIVED
{
	int ret = -1;

	cmd_10_t *cmd_10 = (cmd_10_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_10->require_voltage_l = get_u8_l_from_u16(charger_info->settings->bcl_data.require_voltage);
	cmd_10->require_voltage_h = get_u8_h_from_u16(charger_info->settings->bcl_data.require_voltage);
	cmd_10->require_current_l = get_u8_l_from_u16(charger_info->settings->bcl_data.require_current);
	cmd_10->require_current_h = get_u8_h_from_u16(charger_info->settings->bcl_data.require_current);
	cmd_10->soc = charger_info->settings->bcs_data.soc;
	cmd_10->single_battery_max_voltage_l = get_u8_l_from_u16(charger_info->settings->bcs_data.u1.s.single_battery_max_voltage);
	cmd_10->single_battery_max_voltage_h = get_u8_h_from_u16(charger_info->settings->bcs_data.u1.s.single_battery_max_voltage);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_10_110].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_10_110(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_110_t *cmd_110 = (cmd_110_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->settings->ccs_data.output_voltage =
	    get_u16_from_u8_lh(cmd_110->output_voltage_l, cmd_110->output_voltage_h);

	charger_info->settings->ccs_data.output_current =
	    get_u16_from_u8_lh(cmd_110->output_current_l, cmd_110->output_current_h);

	charger_info->settings->ccs_data.total_charge_time =
	    get_u16_from_u8_lh(cmd_110->total_charge_time_l, cmd_110->total_charge_time_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_10_110].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_10_110 = {
	.cmd = CHANNEL_COM_CMD_10_110,
	.request_period = 200,
	.request_code = 10,
	.request_callback = request_10_110,
	.response_code = 110,
	.response_callback = response_10_110,
};

static int request_11_111(channel_com_info_t *channel_com_info)//500ms CHARGER_INFO_STATUS_BCS_RECEIVED
{
	int ret = -1;

	cmd_11_t *cmd_11 = (cmd_11_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	cmd_11->charge_voltage_l = get_u8_l_from_u16(charger_info->settings->bcs_data.charge_voltage);
	cmd_11->charge_voltage_h = get_u8_h_from_u16(charger_info->settings->bcs_data.charge_voltage);
	cmd_11->charge_current_l = get_u8_l_from_u16(charger_info->settings->bcs_data.charge_current);
	cmd_11->charge_current_h = get_u8_h_from_u16(charger_info->settings->bcs_data.charge_current);
	cmd_11->remain_min_l = get_u8_l_from_u16(charger_info->settings->bcs_data.remain_min);
	cmd_11->remain_min_h = get_u8_h_from_u16(charger_info->settings->bcs_data.remain_min);
	cmd_11->battery_max_temperature = charger_info->settings->bsm_data.battery_max_temperature;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_11_111].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_11_111(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_111_t *cmd_111 = (cmd_111_t *)channel_com_info->can_rx_msg->Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	charger_info->settings->csd_data.total_charge_energy =
	    get_u16_from_u8_lh(cmd_111->charger_output_energy_l, cmd_111->charger_output_energy_h);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_11_111].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_11_111 = {
	.cmd = CHANNEL_COM_CMD_11_111,
	.request_period = 500,
	.request_code = 11,
	.request_callback = request_11_111,
	.response_code = 111,
	.response_callback = response_11_111,
};

static int request_20_120(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_20_120].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_20_120(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	//打开辅板输出继电器
	set_power_output_enable(charger_info, 1);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_20_120].state = CHANNEL_COM_STATE_REQUEST;

	if(charger_info->manual == 1) {
		channel_com_info->cmd_ctx[CHANNEL_COM_CMD_25_125].state = CHANNEL_COM_STATE_REQUEST;
	}

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_20_120 = {
	.cmd = CHANNEL_COM_CMD_20_120,
	.request_period = 0,
	.request_code = 20,
	.request_callback = request_20_120,
	.response_code = 120,
	.response_callback = response_20_120,
};

static int request_21_121(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	int op_ret = 0;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	op_ret = set_gun_lock_state(charger_info, 1, &charger_info->charger_op_ctx_gun_lock);

	if(op_ret == 0) {
		channel_com_info->cmd_ctx[CHANNEL_COM_CMD_21_121].state = CHANNEL_COM_STATE_IDLE;
		ret = 0;
	}

	return ret;
}

static int response_21_121(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	int op_ret = 0;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	//锁定辅板电子锁

	charger_info->charger_op_ctx_gun_lock.state = 0;
	op_ret = set_gun_lock_state(charger_info, 1, &charger_info->charger_op_ctx_gun_lock);

	if(op_ret == 1) {
		channel_com_info->cmd_ctx[CHANNEL_COM_CMD_21_121].state = CHANNEL_COM_STATE_REQUEST;
	}

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_21_121 = {
	.cmd = CHANNEL_COM_CMD_21_121,
	.request_period = 0,
	.request_code = 21,
	.request_callback = request_21_121,
	.response_code = 121,
	.response_callback = response_21_121,
};

static int request_22_122(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	int op_ret = 0;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	op_ret = set_gun_lock_state(charger_info, 0, &charger_info->charger_op_ctx_gun_lock);

	if(op_ret == 0) {
		channel_com_info->cmd_ctx[CHANNEL_COM_CMD_22_122].state = CHANNEL_COM_STATE_IDLE;
		ret = 0;
	}


	return ret;
}

static int response_22_122(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	int op_ret = 0;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	//解除辅板电子锁

	charger_info->charger_op_ctx_gun_lock.state = 0;
	op_ret = set_gun_lock_state(charger_info, 0, &charger_info->charger_op_ctx_gun_lock);

	if(op_ret == 1) {
		channel_com_info->cmd_ctx[CHANNEL_COM_CMD_22_122].state = CHANNEL_COM_STATE_REQUEST;
	}

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_22_122 = {
	.cmd = CHANNEL_COM_CMD_22_122,
	.request_period = 0,
	.request_code = 22,
	.request_callback = request_22_122,
	.response_code = 122,
	.response_callback = response_22_122,
};

static int request_25_125(channel_com_info_t *channel_com_info)//测试开机
{
	int ret = -1;

	//通道主动开机命令

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_25_125].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_25_125(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_25_125].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_25_125 = {
	.cmd = CHANNEL_COM_CMD_25_125,
	.request_period = 0,
	.request_code = 25,
	.request_callback = request_25_125,
	.response_code = 125,
	.response_callback = response_25_125,
};

static int request_30_130(channel_com_info_t *channel_com_info)//bsm状态错误;暂停充电超过10分钟;bms超时错误
{
	int ret = -1;

	//通道主动停机命令

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_30_130].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_30_130(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_30_130].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_30_130 = {
	.cmd = CHANNEL_COM_CMD_30_130,
	.request_period = 0,
	.request_code = 30,
	.request_callback = request_30_130,
	.response_code = 130,
	.response_callback = response_30_130,
};

static int request_50_150(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_50_150].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;

	return ret;
}

static int response_50_150(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	//发送停机命令
	set_charger_control_state(charger_info, BMS_CONTROL_STATE_STOP);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_50_150].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_50_150 = {
	.cmd = CHANNEL_COM_CMD_50_150,
	.request_period = 0,
	.request_code = 50,
	.request_callback = request_50_150,
	.response_code = 150,
	.response_callback = response_50_150,
};

static int request_51_151(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	ret = 0;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_51_151].state = CHANNEL_COM_STATE_IDLE;

	return ret;
}

static int response_51_151(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;

	//关闭辅板输出继电器
	set_power_output_enable(charger_info, 0);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_51_151].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_51_151 = {
	.cmd = CHANNEL_COM_CMD_51_151,
	.request_period = 0,
	.request_code = 51,
	.request_callback = request_51_151,
	.response_code = 151,
	.response_callback = response_51_151,
};

static int request_60_160(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BRM_RECEIVED
{
	int ret = -1;

	cmd_60_t *cmd_60 = (cmd_60_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_60->brm_data, brm_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_60_160].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_60_160(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_60_160].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_61_161].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_60_160 = {
	.cmd = CHANNEL_COM_CMD_60_160,
	.request_period = 200,
	.request_code = 60,
	.request_callback = request_60_160,
	.response_code = 160,
	.response_callback = response_60_160,
};

static int request_61_161(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	cmd_61_t *cmd_61 = (cmd_61_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_61->brm_data, brm_data + 7, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_61_161].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_61_161(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_61_161].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_62_162].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_61_161 = {
	.cmd = CHANNEL_COM_CMD_61_161,
	.request_period = 0,
	.request_code = 61,
	.request_callback = request_61_161,
	.response_code = 161,
	.response_callback = response_61_161,
};

static int request_62_162(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_62_t *cmd_62 = (cmd_62_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_62->brm_data, brm_data + 14, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_62_162].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_62_162(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_62_162].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_63_163].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_62_162 = {
	.cmd = CHANNEL_COM_CMD_62_162,
	.request_period = 0,
	.request_code = 62,
	.request_callback = request_62_162,
	.response_code = 162,
	.response_callback = response_62_162,
};

static int request_63_163(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_63_t *cmd_63 = (cmd_63_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_63->brm_data, brm_data + 21, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_63_163].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_63_163(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_63_163].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_64_164].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_63_163 = {
	.cmd = CHANNEL_COM_CMD_63_163,
	.request_period = 0,
	.request_code = 63,
	.request_callback = request_63_163,
	.response_code = 163,
	.response_callback = response_63_163,
};

static int request_64_164(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_64_t *cmd_64 = (cmd_64_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_64->brm_data, brm_data + 28, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_64_164].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_64_164(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_64_164].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_65_165].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_64_164 = {
	.cmd = CHANNEL_COM_CMD_64_164,
	.request_period = 0,
	.request_code = 64,
	.request_callback = request_64_164,
	.response_code = 164,
	.response_callback = response_64_164,
};

static int request_65_165(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_65_t *cmd_65 = (cmd_65_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_65->brm_data, brm_data + 35, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_65_165].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_65_165(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_65_165].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_66_166].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_65_165 = {
	.cmd = CHANNEL_COM_CMD_65_165,
	.request_period = 0,
	.request_code = 65,
	.request_callback = request_65_165,
	.response_code = 165,
	.response_callback = response_65_165,
};

static int request_66_166(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_66_t *cmd_66 = (cmd_66_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *brm_data = (uint8_t *)&charger_info->settings->brm_data;

	memcpy(cmd_66->brm_data, brm_data + 42, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_66_166].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_66_166(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_66_166].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_66_166 = {
	.cmd = CHANNEL_COM_CMD_66_166,
	.request_period = 0,
	.request_code = 66,
	.request_callback = request_66_166,
	.response_code = 166,
	.response_callback = response_66_166,
};

static int request_67_167(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BCL_RECEIVED
{
	int ret = -1;
	cmd_67_t *cmd_67 = (cmd_67_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bcp_data = (uint8_t *)&charger_info->settings->bcp_data;

	memcpy(cmd_67->bcp_data, bcp_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_67_167].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_67_167(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_67_167].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_68_168].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_67_167 = {
	.cmd = CHANNEL_COM_CMD_67_167,
	.request_period = 200,
	.request_code = 67,
	.request_callback = request_67_167,
	.response_code = 167,
	.response_callback = response_67_167,
};

static int request_68_168(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_68_t *cmd_68 = (cmd_68_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bcp_data = (uint8_t *)&charger_info->settings->bcp_data;

	memcpy(cmd_68->bcp_data, bcp_data + 7, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_68_168].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_68_168(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_68_168].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_69_169].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_68_168 = {
	.cmd = CHANNEL_COM_CMD_68_168,
	.request_period = 0,
	.request_code = 68,
	.request_callback = request_68_168,
	.response_code = 168,
	.response_callback = response_68_168,
};

static int request_69_169(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_69_t *cmd_69 = (cmd_69_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bcs_data = (uint8_t *)&charger_info->settings->bcs_data;

	memcpy(cmd_69->bcs_data, bcs_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_69_169].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_69_169(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_69_169].state = CHANNEL_COM_STATE_IDLE;
	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_70_170].state = CHANNEL_COM_STATE_REQUEST;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_69_169 = {
	.cmd = CHANNEL_COM_CMD_69_169,
	.request_period = 0,
	.request_code = 69,
	.request_callback = request_69_169,
	.response_code = 169,
	.response_callback = response_69_169,
};

static int request_70_170(channel_com_info_t *channel_com_info)
{
	int ret = -1;
	cmd_70_t *cmd_70 = (cmd_70_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bcs_data = (uint8_t *)&charger_info->settings->bcs_data;
	uint8_t *bcl_data = (uint8_t *)&charger_info->settings->bcl_data;

	memcpy(cmd_70->bcs_data, bcs_data + 7, 2);
	memcpy(cmd_70->bcl_data, bcl_data + 0, 5);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_70_170].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_70_170(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_70_170].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_70_170 = {
	.cmd = CHANNEL_COM_CMD_70_170,
	.request_period = 0,
	.request_code = 69,
	.request_code = 70,
	.request_callback = request_70_170,
	.response_code = 170,
	.response_callback = response_70_170,
};

static int request_71_171(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BSM_RECEIVED
{
	int ret = -1;
	cmd_71_t *cmd_71 = (cmd_71_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bsm_data = (uint8_t *)&charger_info->settings->bsm_data;

	memcpy(cmd_71->bsm_data, bsm_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_71_171].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_71_171(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_71_171].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_71_171 = {
	.cmd = CHANNEL_COM_CMD_71_171,
	.request_period = 200,
	.request_code = 69,
	.request_code = 71,
	.request_callback = request_71_171,
	.response_code = 171,
	.response_callback = response_71_171,
};

static int request_72_172(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BST_RECEIVED
{
	int ret = -1;
	cmd_72_t *cmd_72 = (cmd_72_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bst_data = (uint8_t *)&charger_info->settings->bst_data;

	memcpy(cmd_72->bst_data, bst_data + 0, 4);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_72_172].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_72_172(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_72_172].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_72_172 = {
	.cmd = CHANNEL_COM_CMD_72_172,
	.request_period = 200,
	.request_code = 72,
	.request_callback = request_72_172,
	.response_code = 172,
	.response_callback = response_72_172,
};

static int request_73_173(channel_com_info_t *channel_com_info)//200ms CHARGER_INFO_STATUS_BSD_RECEIVED
{
	int ret = -1;
	cmd_73_t *cmd_73 = (cmd_73_t *)channel_com_info->can_tx_msg.Data;
	charger_info_t *charger_info = (charger_info_t *)channel_com_info->charger_info;
	uint8_t *bsd_data = (uint8_t *)&charger_info->settings->bsd_data;

	memcpy(cmd_73->bsd_data, bsd_data + 0, 7);

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_73_173].state = CHANNEL_COM_STATE_RESPONSE;

	ret = 0;

	return ret;
}

static int response_73_173(channel_com_info_t *channel_com_info)
{
	int ret = -1;

	channel_com_info->cmd_ctx[CHANNEL_COM_CMD_73_173].state = CHANNEL_COM_STATE_IDLE;

	ret = 0;
	return ret;
}

static channel_com_command_item_t channel_com_command_item_73_173 = {
	.cmd = CHANNEL_COM_CMD_73_173,
	.request_period = 200,
	.request_code = 73,
	.request_callback = request_73_173,
	.response_code = 173,
	.response_callback = response_73_173,
};

static channel_com_command_item_t *channel_com_command_table[] = {
	&channel_com_command_item_1_101,
	&channel_com_command_item_2_102,
	&channel_com_command_item_13_113,
	&channel_com_command_item_3_103,
	&channel_com_command_item_4_104,
	&channel_com_command_item_5_105,
	&channel_com_command_item_6_106,
	&channel_com_command_item_7_107,
	&channel_com_command_item_8_108,
	&channel_com_command_item_9_109,
	&channel_com_command_item_10_110,
	&channel_com_command_item_11_111,
	&channel_com_command_item_20_120,
	&channel_com_command_item_21_121,
	&channel_com_command_item_22_122,
	&channel_com_command_item_25_125,
	&channel_com_command_item_30_130,
	&channel_com_command_item_50_150,
	&channel_com_command_item_51_151,
	&channel_com_command_item_60_160,
	&channel_com_command_item_61_161,
	&channel_com_command_item_62_162,
	&channel_com_command_item_63_163,
	&channel_com_command_item_64_164,
	&channel_com_command_item_65_165,
	&channel_com_command_item_66_166,
	&channel_com_command_item_67_167,
	&channel_com_command_item_68_168,
	&channel_com_command_item_69_169,
	&channel_com_command_item_70_170,
	&channel_com_command_item_71_171,
	&channel_com_command_item_72_172,
	&channel_com_command_item_73_173,
};

static void update_connect_state(channel_com_info_t *channel_com_info, uint8_t state)
{
	channel_com_info->connect_state[channel_com_info->connect_state_index] = state;
	channel_com_info->connect_state_index++;

	if(channel_com_info->connect_state_index >= CHANNEL_COM_CONNECT_STATE_SIZE) {
		channel_com_info->connect_state_index = 0;
	}
}

uint8_t channel_com_get_connect_state(channel_com_info_t *channel_com_info)
{
	uint8_t count = 0;
	int i;

	for(i = 0; i < CHANNEL_COM_CONNECT_STATE_SIZE; i++) {
		if(channel_com_info->connect_state[i] != 0) {
			count++;
		}
	}

	return count;
}

static void channel_com_request_periodic(channel_com_info_t *channel_com_info)
{
	int i;
	uint32_t ticks = osKernelSysTick();

	for(i = 0; i < sizeof(channel_com_command_table) / sizeof(channel_com_command_item_t *); i++) {
		channel_com_command_item_t *item = channel_com_command_table[i];

		if(channel_com_info->cmd_ctx[item->cmd].state == CHANNEL_COM_STATE_RESPONSE) {
			if(ticks - channel_com_info->cmd_ctx[item->cmd].send_stamp >= 100) {
				update_connect_state(channel_com_info, 0);
				channel_com_info->cmd_ctx[item->cmd].state = CHANNEL_COM_STATE_ERROR;
			}
		}

		if(item->request_period == 0) {
			continue;
		}

		if(channel_com_info->cmd_ctx[item->cmd].available == 0) {
			continue;
		}

		if(ticks - channel_com_info->cmd_ctx[item->cmd].stamp >= item->request_period) {
			channel_com_info->cmd_ctx[item->cmd].stamp = ticks;
			channel_com_info->cmd_ctx[item->cmd].state = CHANNEL_COM_STATE_REQUEST;
		}
	}

}

void task_channel_com_request(void const *argument)
{
	int ret = 0;
	int i;

	channel_com_info_t *channel_com_info = (channel_com_info_t *)argument;

	if(channel_com_info == NULL) {
		app_panic();
	}

	while(1) {
		for(i = 0; i < sizeof(channel_com_command_table) / sizeof(channel_com_command_item_t *); i++) {
			uint32_t ticks = osKernelSysTick();
			channel_com_command_item_t *item = channel_com_command_table[i];
			cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_tx_msg.Data;

			if(channel_com_info->cmd_ctx[item->cmd].state != CHANNEL_COM_STATE_REQUEST) {
				continue;
			}

			channel_com_info->can_tx_msg.DLC = 8;

			_printf("%s:%s:%d request cmd %d\n",
			        __FILE__, __func__, __LINE__,
			        item->request_code);

			memset(channel_com_info->can_tx_msg.Data, 0, 8);

			cmd_common->cmd = item->request_code;

			ret = item->request_callback(channel_com_info);

			if(ret != 0) {
				continue;
			}

			channel_com_info->cmd_ctx[item->cmd].send_stamp = ticks;
			ret = can_tx_data(channel_com_info->can_info, &channel_com_info->can_tx_msg, 10);

			if(ret != 0) {
				update_connect_state(channel_com_info, 0);
				channel_com_info->cmd_ctx[item->cmd].state = CHANNEL_COM_STATE_ERROR;
			}
		}

		channel_com_request_periodic(channel_com_info);
		osDelay(10);
	}
}

void task_channel_com_response(void const *argument)
{
	int ret = 0;
	int i;

	channel_com_info_t *channel_com_info = (channel_com_info_t *)argument;

	if(channel_com_info == NULL) {
		app_panic();
	}

	while(1) {
		ret = can_rx_data(channel_com_info->can_info, 1000);

		if(ret != 0) {
			continue;
		}

		channel_com_info->can_rx_msg = can_get_msg(channel_com_info->can_info);

		for(i = 0; i < sizeof(channel_com_command_table) / sizeof(channel_com_command_item_t *); i++) {
			channel_com_command_item_t *item = channel_com_command_table[i];
			cmd_common_t *cmd_common = (cmd_common_t *)channel_com_info->can_rx_msg->Data;

			if(cmd_common->cmd == item->response_code) {
				ret = item->response_callback(channel_com_info);

				if(ret == 0) {
					update_connect_state(channel_com_info, 1);
				}

				break;
			}

		}
	}
}
