

/*================================================================
 *
 *
 *   文件名称：charger_handler.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 14时18分42秒
 *   修改日期：2021年02月21日 星期日 19时36分47秒
 *   描    述：
 *
 *================================================================*/
#include <string.h>
#include "charger_handler.h"
#include "charger.h"
#include "bms_spec.h"
#include "bms_multi_data.h"

#include "log.h"

static int handle_common_bst_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BST: {
			bst_data_t *data = (bst_data_t *)rx_msg->Data;

			charger_info->settings->bst_data = *data;

			if(charger_info->bst_received == 0) {
				charger_info->bst_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BST_RECEIVED);
			}

			set_charger_state(charger_info, CHARGER_STATE_CST);

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

static int handle_common_response(charger_info_t *charger_info)
{
	int ret = -1;

	ret = handle_multi_data_response(charger_info->can_info, &charger_info->multi_packets_info, charger_info->settings);

	if(ret == 0) {
		return ret;
	}

	ret = handle_common_bst_response(charger_info);

	return ret;
}


static int prepare_state_idle(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->stamp = ticks;

	charger_info->idle_op_state = IDLE_OP_STATE_NONE;

	charger_info->charger_request_state = CHARGER_REQUEST_STATE_NONE;

	return ret;
}

static int handle_state_idle_request(charger_info_t *charger_info)
{
	int ret = 0;
	int op_ret = 0;

	switch(charger_info->idle_op_state) {
		case IDLE_OP_STATE_NONE: {
			charger_info->bms_connect_retry = 0;
			set_auxiliary_power_state(charger_info, 0);

			charger_info->charger_op_ctx_gun_lock.state = 0;
			charger_info->idle_op_state = IDLE_OP_STATE_GUN_UNLOCK;
			debug("IDLE_OP_STATE_NONE done!\n");
		}
		break;

		case IDLE_OP_STATE_GUN_UNLOCK: {
			op_ret = set_gun_lock_state(charger_info, 0, &charger_info->charger_op_ctx_gun_lock);

			if(op_ret == 0) {
				charger_info->idle_op_state = IDLE_OP_STATE_IDLE;
				debug("IDLE_OP_STATE_GUN_UNLOCK done!\n");
			}
		}
		break;

		case IDLE_OP_STATE_IDLE: {
			if((charger_info->gun_connect_state == 1) &&
			   (charger_info->charger_request_state == CHARGER_REQUEST_STATE_START)) {
				charger_info->charger_request_state = CHARGER_REQUEST_STATE_NONE;
				charger_info->charger_op_ctx_gun_lock.state = 0;
				charger_info->idle_op_state = IDLE_OP_STATE_GUN_LOCK;
				debug("IDLE_OP_STATE_IDLE done!\n");
			} else {
				//debug("IDLE_OP_STATE_IDLE!\n");
			}
		}
		break;

		case IDLE_OP_STATE_GUN_LOCK: {
			op_ret = set_gun_lock_state(charger_info, 1, &charger_info->charger_op_ctx_gun_lock);

			if(op_ret == 0) {
				charger_info->idle_op_state = IDLE_OP_STATE_AUXILLIARY_POWER_ON;
				debug("IDLE_OP_STATE_GUN_LOCK done!\n");
			}
		}
		break;

		case IDLE_OP_STATE_AUXILLIARY_POWER_ON: {
			set_auxiliary_power_state(charger_info, 1);
			set_charger_state(charger_info, CHARGER_STATE_CHM);
			debug("IDLE_OP_STATE_AUXILLIARY_POWER_ON done!\n");
		}
		break;

		default: {
			debug("%s:%s:%d!\n", __FILE__, __func__, __LINE__);
		}
		break;
	}

	return ret;
}

static int handle_state_idle_response(charger_info_t *charger_info)
{
	int ret = 0;
	return ret;
}

static int prepare_state_chm(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->send_stamp = ticks - FN_CHM_SEND_PERIOD;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

	charger_info->bhm_received = 0;
	charger_info->bst_received = 0;

	charger_info->chm_op_state = CHM_OP_STATE_NONE;

	return ret;
}

static int send_chm(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	chm_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CHM_PRIORITY, FN_CHM, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(chm_data_t);

	data = (chm_data_t *)tx_msg.Data;

	data->version_0 = charger_info->settings->chm_data.version_0;
	data->version_1 = charger_info->settings->chm_data.version_1;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int handle_state_chm_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();
	int op_ret = 0;

	if(ticks_duration(ticks, charger_info->stamp) >= BMS_GENERIC_TIMEOUT) {
		set_charger_state(charger_info, CHARGER_STATE_CRM);
	}

	if(charger_info->charger_request_state == CHARGER_REQUEST_STATE_STOP) {
		charger_info->charger_request_state = CHARGER_REQUEST_STATE_NONE;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	}

	if(ticks_duration(ticks, charger_info->send_stamp) >= FN_CHM_SEND_PERIOD) {
		send_chm(charger_info);
		charger_info->send_stamp = ticks;
	}

	switch(charger_info->chm_op_state) {
		case CHM_OP_STATE_DISCHARGE: {//放电
			op_ret = discharge(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK;

				debug("CHM_OP_STATE_DISCHARGE done!\n");
			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CHM_OP_STATE_DISCHARGE_TIMEOUT);

				set_charger_state(charger_info, CHARGER_STATE_IDLE);

				debug("CHM_OP_STATE_DISCHARGE timeout\n");
			}
		}
		break;

		case CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK: {//接触器外侧电压检查
			op_ret = relay_endpoint_overvoltage_status(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 0) {
				if(charger_info->settings->cml_data.max_output_voltage > charger_info->settings->bhm_data.max_charge_voltage) {
					if(charger_info->settings->bhm_data.max_charge_voltage != 0) {
						charger_info->precharge_voltage = charger_info->settings->bhm_data.max_charge_voltage;
					} else {
						charger_info->precharge_voltage = charger_info->settings->cml_data.max_output_voltage;
					}
				} else {
					charger_info->precharge_voltage = charger_info->settings->cml_data.max_output_voltage;
					charger_info->precharge_action = PRECHARGE_ACTION_START;
				}

				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_INSULATION_CHECK_PRECHARGE;

				debug("CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK done!\n");

			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK_TIMEOUT);

				set_charger_state(charger_info, CHARGER_STATE_IDLE);

				debug("CHM_OP_STATE_RELAY_ENDPOINT_OVERVOLTAGE_CHECK timeout!\n");
			}
		}
		break;

		case CHM_OP_STATE_INSULATION_CHECK_PRECHARGE: {//绝缘检查预充
			op_ret = precharge(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				set_power_output_enable(charger_info, 1);//打开输出

				charger_info->stamp_1 = ticks;

				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_INSULATION_CHECK_DELAY_1;

				debug("CHM_OP_STATE_INSULATION_CHECK_PRECHARGE done!\n");
			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_PRECHARGE_TIMEOUT);
				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_ABORT_DISCHARGE;

				debug("CHM_OP_STATE_INSULATION_CHECK_PRECHARGE timeout!\n");
			}
		}
		break;

		case CHM_OP_STATE_INSULATION_CHECK_DELAY_1: {//绝缘检查预充结束延时
			if(ticks_duration(ticks, charger_info->stamp_1) >= 1000) {

				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE;

				debug("CHM_OP_STATE_INSULATION_CHECK_DELAY_1 done!\n");
			}
		}
		break;

		case CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE: {//绝缘检查停止预充
			charger_info->precharge_voltage = 0;
			charger_info->precharge_action = PRECHARGE_ACTION_STOP;
			op_ret = precharge(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				charger_info->stamp_1 = ticks;

				charger_info->chm_op_state = CHM_OP_STATE_INSULATION_CHECK_DELAY_2;

				debug("CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE done!\n");
			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE_TIMEOUT);
				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_ABORT_DISCHARGE;

				debug("CHM_OP_STATE_INSULATION_CHECK_STOP_PRECHARGE timeout!\n");
			}
		}
		break;

		case CHM_OP_STATE_INSULATION_CHECK_DELAY_2: {//绝缘检查停止预充延时
			if(ticks_duration(ticks, charger_info->stamp_1) >= 500) {

				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_INSULATION_CHECK_DISCHARGE;

				debug("CHM_OP_STATE_INSULATION_CHECK_DELAY_2 done!\n");
			}
		}
		break;

		case CHM_OP_STATE_INSULATION_CHECK_DISCHARGE: {//绝缘检查放电
			op_ret = discharge(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_INSULATION_CHECK;

				debug("CHM_OP_STATE_INSULATION_CHECK_DISCHARGE done!\n");

			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_DISCHARGE_TIMEOUT);
				set_charger_state(charger_info, CHARGER_STATE_IDLE);

				debug("CHM_OP_STATE_INSULATION_CHECK_DISCHARGE timeout\n");
			}
		}
		break;

		case CHM_OP_STATE_INSULATION_CHECK: {//绝缘检查
			op_ret = insulation_check(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				set_power_output_enable(charger_info, 0);//关闭输出

				charger_info->charger_op_ctx.state = 0;

				charger_info->chm_op_state = CHM_OP_STATE_NONE;

				set_charger_state(charger_info, CHARGER_STATE_CRM);

				debug("CHM_OP_STATE_INSULATION_CHECK done!\n");

			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CHM_OP_STATE_INSULATION_CHECK_TIMEOUT);

				set_charger_state(charger_info, CHARGER_STATE_IDLE);

				debug("CHM_OP_STATE_INSULATION_CHECK timeout!\n");
			}
		}
		break;

		case CHM_OP_STATE_ABORT_DISCHARGE: {//chm中止放电
			op_ret = discharge(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				set_power_output_enable(charger_info, 0);//关闭输出
				set_charger_state(charger_info, CHARGER_STATE_IDLE);

				debug("CHM_OP_STATE_ABORT_DISCHARGE done!\n");
			} else if(op_ret == -1) {
				set_power_output_enable(charger_info, 0);//关闭输出
				set_charger_state(charger_info, CHARGER_STATE_IDLE);

				debug("CHM_OP_STATE_ABORT_DISCHARGE timeout\n");
			}
		}
		break;

		default:
			break;
	}

	return ret;
}

static int handle_state_chm_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(charger_info);

	if(ret == 0) {
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BHM: {
			bhm_data_t *data = (bhm_data_t *)rx_msg->Data;
			uint32_t ticks = osKernelSysTick();

			charger_info->settings->bhm_data.max_charge_voltage = data->max_charge_voltage;

			charger_info->stamp = ticks;

			if(charger_info->settings->cml_data.min_output_voltage > charger_info->settings->bhm_data.max_charge_voltage) {//超过充电机输出能力
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CHM_OUTPUT_VOLTAGE_UNMATCH);
				set_charger_state(charger_info, CHARGER_STATE_IDLE);
			} else {
				if(charger_info->bhm_received == 0) {
					charger_info->bhm_received = 1;
					charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BHM_RECEIVED);

					charger_info->charger_op_ctx.state = 0;

					charger_info->chm_op_state = CHM_OP_STATE_DISCHARGE;
				}
			}

			ret = 0;

		}
		break;

		default:
			break;
	}

	return ret;
}

static int prepare_state_crm(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->send_stamp = ticks - FN_CRM_SEND_PERIOD;
	charger_info->stamp = ticks;

	charger_info->brm_received = 0;
	charger_info->bcp_received = 0;
	return ret;
}

static int send_crm(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	crm_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CRM_PRIORITY, FN_CRM, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(crm_data_t);

	data = (crm_data_t *)tx_msg.Data;
	data->crm_result = charger_info->settings->crm_data.crm_result;
	data->charger_sn = charger_info->settings->crm_data.charger_sn;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int handle_state_crm_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, charger_info->stamp) >= BMS_GENERIC_TIMEOUT) {
		if(charger_info->brm_received == 0) {
			charger_info->settings->cem_data.u1.s.brm_timeout = 0x01;
			charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BRM_TIMEOUT);
		} else {
			charger_info->settings->cem_data.u2.s.bcp_timeout = 0x01;
			charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BCP_TIMEOUT);
		}

		set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
	}

	if(charger_info->charger_request_state == CHARGER_REQUEST_STATE_STOP) {
		charger_info->charger_request_state = CHARGER_REQUEST_STATE_NONE;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	}

	if(ticks_duration(ticks, charger_info->send_stamp) >= FN_CRM_SEND_PERIOD) {
		send_crm(charger_info);
		charger_info->send_stamp = ticks;
	}

	return ret;
}

static int handle_state_crm_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(charger_info);

	if(ret == 0) {
		if(is_bms_data_multi_received(charger_info->can_info, &charger_info->multi_packets_info, FN_BRM)) {
			if(charger_info->brm_received == 0) {
				charger_info->settings->crm_data.crm_result = 0xaa;
				charger_info->brm_received = 1;
				charger_info->stamp = osKernelSysTick();
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BRM_RECEIVED);
			}
		} else if(is_bms_data_multi_received(charger_info->can_info, &charger_info->multi_packets_info, FN_BCP)) {//bcp timeout
			if(charger_info->bcp_received == 0) {
				charger_info->bcp_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BCP_RECEIVED);
			}

			set_charger_state(charger_info, CHARGER_STATE_CTS_CML);
		}
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BRM: {
			brm_data_t *data = (brm_data_t *)rx_msg->Data;

			charger_info->settings->brm_data.brm_data = *data;

			if(charger_info->brm_received == 0) {
				charger_info->settings->crm_data.crm_result = 0xaa;
				charger_info->brm_received = 1;
				charger_info->stamp = osKernelSysTick();
			}

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

static int prepare_state_cts_cml(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->send_stamp = ticks - FN_CTS_SEND_PERIOD;
	charger_info->send_stamp_1 = ticks - FN_CML_SEND_PERIOD;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

	charger_info->bro_received = 0;

	return ret;
}

static int send_cts(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	cts_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CTS_PRIORITY, FN_CTS, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(cts_data_t);

	data = (cts_data_t *)tx_msg.Data;
	*data = charger_info->settings->cts_data;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int send_cml(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	cml_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CML_PRIORITY, FN_CML, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(cml_data_t);

	data = (cml_data_t *)tx_msg.Data;
	*data = charger_info->settings->cml_data;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int handle_state_cts_cml_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, charger_info->stamp) >= BMS_GENERIC_TIMEOUT) {//定时发送
		charger_info->settings->cem_data.u2.s.bro_timeout = 0x01;
		charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BRO_TIMEOUT);
		set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
	} else if(ticks_duration(ticks, charger_info->stamp_1) >= FN_BRO_0xAA_TIMEOUT) {
		charger_info->settings->cem_data.u2.s.bro_timeout = 0x01;
		charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BRO_TIMEOUT);
		set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
	}

	if(charger_info->charger_request_state == CHARGER_REQUEST_STATE_STOP) {
		charger_info->charger_request_state = CHARGER_REQUEST_STATE_NONE;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	}

	if(ticks_duration(ticks, charger_info->send_stamp) >= FN_CTS_SEND_PERIOD) {
		send_cts(charger_info);
		charger_info->send_stamp = ticks;
	}

	if(ticks_duration(ticks, charger_info->send_stamp_1) >= FN_CML_SEND_PERIOD) {
		send_cml(charger_info);
		charger_info->send_stamp_1 = ticks;
	}

	return ret;
}

static int handle_state_cts_cml_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(charger_info);

	if(ret == 0) {
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BRO: {
			bro_data_t *data = (bro_data_t *)rx_msg->Data;

			charger_info->settings->bro_data = *data;

			charger_info->stamp = osKernelSysTick();

			if(data->bro_result == 0xaa) {
				set_charger_state(charger_info, CHARGER_STATE_CRO);
			}

			if(charger_info->bro_received == 0) {
				charger_info->bro_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BRO_RECEIVED);
			}

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

static int prepare_state_cro(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->send_stamp = ticks - FN_CRO_SEND_PERIOD;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

	charger_info->stamp_2 = ticks;

	charger_info->settings->cro_data.cro_result = 0x00;
	charger_info->bcl_received = 0;
	charger_info->bcs_received = 0;

	charger_info->cro_op_state = CRO_OP_STATE_START_PRECHARGE;

	return ret;
}

static int send_cro(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	cro_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CRO_PRIORITY, FN_CRO, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(cro_data_t);

	data = (cro_data_t *)tx_msg.Data;
	*data = charger_info->settings->cro_data;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int handle_state_cro_request(charger_info_t *charger_info)
{
	int ret = 0;
	int op_ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(charger_info->settings->cro_data.cro_result == 0xaa) {
		if(ticks_duration(ticks, charger_info->stamp) >= FN_BCL_TIMEOUT) {//定时发送
			charger_info->settings->cem_data.u3.s.bcl_timeout = 0x01;
			charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BCL_TIMEOUT);
			set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
		} else if(ticks_duration(ticks, charger_info->stamp_1) >= BMS_GENERIC_TIMEOUT) {//定时发送
			charger_info->settings->cem_data.u3.s.bcs_timeout = 0x01;
			charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BCS_TIMEOUT);
			set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
		}
	}

	if(charger_info->charger_request_state == CHARGER_REQUEST_STATE_STOP) {
		charger_info->charger_request_state = CHARGER_REQUEST_STATE_NONE;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	}

	if(ticks_duration(ticks, charger_info->send_stamp) >= FN_CRO_SEND_PERIOD) {
		send_cro(charger_info);
		charger_info->send_stamp = ticks;
	}

	switch(charger_info->cro_op_state) {
		case CRO_OP_STATE_START_PRECHARGE: {//预充前充电机输出能力判断
			if(charger_info->settings->cml_data.min_output_voltage > charger_info->settings->bcp_data.total_voltage) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH);
				set_charger_state(charger_info, CHARGER_STATE_CST);
			} else if(charger_info->settings->cml_data.max_output_voltage < charger_info->settings->bcp_data.total_voltage) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CRO_OUTPUT_VOLTAGE_UNMATCH);
				set_charger_state(charger_info, CHARGER_STATE_CST);
			} else {
				charger_info->charger_op_ctx.state = 0;

				charger_info->cro_op_state = CRO_OP_STATE_GET_BATTERY_STATUS;
			}

			debug("CRO_OP_STATE_START_PRECHARGE done!\n");
		}
		break;

		case CRO_OP_STATE_GET_BATTERY_STATUS: {//获取电池电压
			op_ret = battery_voltage_status(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				charger_info->precharge_voltage = charger_info->settings->bcp_data.total_voltage - 50;
				charger_info->precharge_action = PRECHARGE_ACTION_START;

				charger_info->charger_op_ctx.state = 0;

				charger_info->cro_op_state = CRO_OP_STATE_PRECHARGE;

				debug("CRO_OP_STATE_GET_BATTERY_STATUS done\n");
			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CRO_OP_STATE_GET_BATTERY_STATUS_TIMEOUT);

				set_charger_state(charger_info, CHARGER_STATE_CST);

				debug("CRO_OP_STATE_GET_BATTERY_STATUS timeout\n");
			}
		}
		break;

		case CRO_OP_STATE_PRECHARGE: {//预充
			op_ret = precharge(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				charger_info->stamp_2 = ticks;

				charger_info->cro_op_state = CRO_OP_STATE_PRECHARGE_DELAY_1;

				debug("CRO_OP_STATE_PRECHARGE done\n");
			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CRO_OP_STATE_PRECHARGE_TIMEOUT);

				set_charger_state(charger_info, CHARGER_STATE_CST);

				debug("CRO_OP_STATE_PRECHARGE timeout\n");
			}
		}
		break;

		case CRO_OP_STATE_PRECHARGE_DELAY_1: {//预充后等待2s,打开输出继电器
			if(ticks_duration(ticks, charger_info->stamp_2) >= (2 * 1000)) {
				set_power_output_enable(charger_info, 1);//打开输出

				charger_info->precharge_action = PRECHARGE_ACTION_STOP;//打开继电器后，停止预充

				charger_info->stamp_2 = ticks;

				charger_info->cro_op_state = CRO_OP_STATE_PRECHARGE_DELAY_2;

				debug("CRO_OP_STATE_PRECHARGE_DELAY_1 done!\n");
			}
		}
		break;

		case CRO_OP_STATE_PRECHARGE_DELAY_2: {//等待1s
			if(ticks_duration(ticks, charger_info->stamp_2) >= (1 * 1000)) {
				charger_info->stamp = ticks;
				charger_info->stamp_1 = ticks;

				charger_info->settings->cro_data.cro_result = 0xaa;

				charger_info->stamp_2 = ticks;

				charger_info->cro_op_state = CRO_OP_STATE_NONE;

				debug("CRO_OP_STATE_PRECHARGE_DELAY_2 done!\n");
			}
		}
		break;

		default:
			break;
	}

	return ret;
}

static int handle_state_cro_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(charger_info);

	if(ret == 0) {
		if(is_bms_data_multi_received(charger_info->can_info, &charger_info->multi_packets_info, FN_BCS)) {
			charger_info->stamp_1 = osKernelSysTick();

			if(charger_info->bcs_received == 0) {
				charger_info->bcs_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BCS_RECEIVED);
			}
		}
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BCL: {
			bcl_data_t *data = (bcl_data_t *)rx_msg->Data;

			charger_info->settings->bcl_data = *data;

			charger_info->stamp = osKernelSysTick();

			if(charger_info->bcl_received == 0) {
				charger_info->bcl_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BCL_RECEIVED);
			}

			ret = 0;
		}
		break;

		default:
			break;
	}

	if((charger_info->bcl_received == 1) && (charger_info->bcs_received == 1)) {
		set_charger_state(charger_info, CHARGER_STATE_CCS);
	}

	return ret;
}

static int prepare_state_ccs(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->send_stamp = ticks - FN_CCS_SEND_PERIOD;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;
	charger_info->stamp_2 = ticks;//battery_charge_enable

	charger_info->bsm_received = 0;

	return ret;
}

static int send_ccs(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	ccs_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CCS_PRIORITY, FN_CCS, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(ccs_data_t);

	data = (ccs_data_t *)tx_msg.Data;

	if(charger_info->test_mode == 0) {
		charger_info->settings->ccs_data.output_voltage = charger_info->charger_output_voltage;
		charger_info->settings->ccs_data.output_current = charger_info->charger_output_current;
	}

	*data = charger_info->settings->ccs_data;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int handle_state_ccs_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, charger_info->stamp) >= FN_BCL_TIMEOUT) {//bcl timeout
		charger_info->settings->cem_data.u3.s.bcl_timeout = 0x01;
		charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BCL_TIMEOUT);
		set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
	} else if(ticks_duration(ticks, charger_info->stamp_1) >= BMS_GENERIC_TIMEOUT) {//bcs timeout
		charger_info->settings->cem_data.u3.s.bcs_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
		charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BCS_TIMEOUT);
	}

	if(charger_info->charger_request_state == CHARGER_REQUEST_STATE_STOP) {
		charger_info->charger_request_state = CHARGER_REQUEST_STATE_NONE;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	}

	if(ticks_duration(ticks, charger_info->send_stamp) >= FN_CCS_SEND_PERIOD) {
		send_ccs(charger_info);
		charger_info->send_stamp = ticks;
	}

	if(charger_info->bsm_received == 1) {
		if(charger_info->settings->bsm_data.u2.s.battery_charge_enable == 1) {
			charger_info->stamp_2 = ticks;
		}

		if(ticks_duration(ticks, charger_info->stamp_2) >= (10 * 60 * 1000)) {//车端暂停时间大于10分钟，结束充电
			set_charger_state(charger_info, CHARGER_STATE_CST);
		}
	}

	//if(charger_info->charger_power_on == 0) {//主板复位
	//	set_charger_state(charger_info, CHARGER_STATE_CST);
	//}

	return ret;
}

static int handle_state_ccs_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(charger_info);

	if(ret == 0) {
		if(is_bms_data_multi_received(charger_info->can_info, &charger_info->multi_packets_info, FN_BCS)) {
			charger_info->stamp_1 = osKernelSysTick();
		}
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BCL: {
			bcl_data_t *data = (bcl_data_t *)rx_msg->Data;

			charger_info->settings->bcl_data = *data;

			charger_info->stamp = osKernelSysTick();

			ret = 0;
		}

		case FN_BSM: {
			bsm_data_t *data = (bsm_data_t *)rx_msg->Data;

			charger_info->settings->bsm_data = *data;

			if(charger_info->bsm_received == 0) {
				charger_info->bsm_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BSM_RECEIVED);
			}

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

static int prepare_state_cst(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->send_stamp = ticks - FN_CST_SEND_PERIOD;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

	charger_info->start_send_cst_stamp = ticks;

	charger_info->bsd_received = 0;

	charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CST);

	return ret;
}

static int send_cst(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	cst_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CST_PRIORITY, FN_CST, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(cst_data_t);

	data = (cst_data_t *)tx_msg.Data;
	*data = charger_info->settings->cst_data;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int handle_state_cst_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, charger_info->stamp) >= BMS_GENERIC_TIMEOUT) {//超时
		charger_info->settings->cem_data.u3.s.bst_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
	}

	if(ticks_duration(ticks, charger_info->send_stamp) >= FN_CST_SEND_PERIOD) {
		send_cst(charger_info);
		charger_info->send_stamp = ticks;
	}

	return ret;
}

static int handle_state_cst_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;
	uint32_t ticks = osKernelSysTick();

	if(rx_msg == NULL) {
		return ret;
	}

	//ret = handle_common_response(charger_info);

	//if(ret == 0) {
	//}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BST: {
			bst_data_t *data = (bst_data_t *)rx_msg->Data;

			charger_info->settings->bst_data = *data;

			charger_info->stamp = ticks;

			if(charger_info->bst_received == 0) {
				charger_info->bst_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BST_RECEIVED);
			}

			ret = 0;
		}
		break;

		case FN_BSD: {
			bsd_data_t *data = (bsd_data_t *)rx_msg->Data;

			charger_info->settings->bsd_data = *data;

			if(charger_info->bsd_received == 0) {
				charger_info->bsd_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BSD_RECEIVED);
			}

			charger_info->stamp_1 = ticks;

			set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

static int prepare_state_csd_cem(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->send_stamp = ticks - FN_CSD_SEND_PERIOD;
	charger_info->send_stamp_1 = ticks - FN_CEM_SEND_PERIOD;

	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

	charger_info->bem_received = 0;

	charger_info->csd_cem_op_state = CSD_CEM_OP_STATE_NONE;

	return ret;
}

static int send_csd(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	csd_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CSD_PRIORITY, FN_CSD, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(csd_data_t);

	data = (csd_data_t *)tx_msg.Data;
	*data = charger_info->settings->csd_data;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static int send_cem(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	cem_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_CEM_PRIORITY, FN_CEM, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(cem_data_t);

	data = (cem_data_t *)tx_msg.Data;
	*data = charger_info->settings->cem_data;

	ret = can_tx_data(can_info, &tx_msg, 10);

	return ret;
}

static uint8_t is_cem_valid(charger_info_t *charger_info)
{
	uint8_t ret = 0;
	cem_data_t *cem_data = &charger_info->settings->cem_data;

	if(cem_data->u1.s.brm_timeout != 0x00) {
		ret = 1;
	}

	if(cem_data->u2.s.bcp_timeout != 0x00) {
		ret = 1;
	}

	if(cem_data->u2.s.bro_timeout != 0x00) {
		ret = 1;
	}

	if(cem_data->u3.s.bcs_timeout != 0x00) {
		ret = 1;
	}

	if(cem_data->u3.s.bcl_timeout != 0x00) {
		ret = 1;
	}

	if(cem_data->u3.s.bst_timeout != 0x00) {
		ret = 1;
	}

	if(cem_data->u4.s.bsd_timeout != 0x00) {
		ret = 1;
	}

	if(cem_data->u4.s.other != 0x00) {
		ret = 1;
	}

	return ret;
}

static int handle_state_csd_cem_request(charger_info_t *charger_info)
{
	int ret = 0;
	int op_ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks_duration(ticks, charger_info->start_send_cst_stamp) >= FN_BSD_TIMEOUT) {//累计10s没收到bsd,充电时序结束
		charger_info->settings->cem_data.u4.s.bsd_timeout = 0x01;

		if(charger_info->csd_cem_op_state == CSD_CEM_OP_STATE_NONE) {
			charger_info->charger_op_ctx.state = 0;

			charger_info->csd_cem_op_state = CSD_CEM_OP_STATE_WAIT_NO_CURRENT;
		}
	}

	if(charger_info->bsd_received == 1) {
		if(ticks_duration(ticks, charger_info->send_stamp) >= FN_CSD_SEND_PERIOD) {
			send_csd(charger_info);
			charger_info->send_stamp = ticks;
		}
	}

	if(is_cem_valid(charger_info) == 1) {
		if(ticks_duration(ticks, charger_info->send_stamp_1) >= FN_CEM_SEND_PERIOD) {
			send_cem(charger_info);
			charger_info->send_stamp_1 = ticks;
		}
	}

	switch(charger_info->csd_cem_op_state) {
		//等待电流小于5A
		case CSD_CEM_OP_STATE_WAIT_NO_CURRENT: {
			op_ret = wait_no_current(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				charger_info->stamp_1 = ticks;

				charger_info->csd_cem_op_state = CSD_CEM_OP_STATE_DISABLE_OUTPUT_DELAY;

				debug("CSD_CEM_OP_STATE_WAIT_NO_CURRENT done!\n");

			} else if(op_ret == -1) {
				charger_info->stamp_1 = ticks;

				charger_info->csd_cem_op_state = CSD_CEM_OP_STATE_DISABLE_OUTPUT_DELAY;

				debug("CSD_CEM_OP_STATE_WAIT_NO_CURRENT timeout\n");
			}
		}
		break;

		case CSD_CEM_OP_STATE_DISABLE_OUTPUT_DELAY: {
			if(ticks_duration(ticks, charger_info->stamp_1) >= 500) {
				set_power_output_enable(charger_info, 0);//关闭输出

				charger_info->charger_op_ctx.state = 0;

				charger_info->csd_cem_op_state = CSD_CEM_OP_STATE_DISCHARGE;

				debug("CSD_CEM_OP_STATE_DISABLE_OUTPUT_DELAY done!\n");
			}
		}
		break;

		case CSD_CEM_OP_STATE_DISCHARGE: {//放电启动
			op_ret = discharge(charger_info, &charger_info->charger_op_ctx);

			if(op_ret == 1) {
			} else if(op_ret == 0) {
				charger_info->csd_cem_op_state = CSD_CEM_OP_STATE_NONE;

				set_charger_state(charger_info, CHARGER_STATE_IDLE);

				debug("CSD_CEM_OP_STATE_DISCHARGE done!\n");
			} else if(op_ret == -1) {
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_CSD_CEM_OP_STATE_DISCHARGE_TIMEOUT);

				charger_info->csd_cem_op_state = CSD_CEM_OP_STATE_NONE;

				set_charger_state(charger_info, CHARGER_STATE_IDLE);

				debug("CSD_CEM_OP_STATE_DISCHARGE timeout\n");

			}
		}
		break;

		default:
			break;
	}

	return ret;
}

static int handle_state_csd_cem_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	uint32_t ticks = osKernelSysTick();
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BSD: {
			bsd_data_t *data = (bsd_data_t *)rx_msg->Data;

			charger_info->settings->bsd_data = *data;

			if(charger_info->bsd_received == 0) {
				charger_info->bsd_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BSD_RECEIVED);
			}

			charger_info->stamp = ticks;

			ret = 0;
		}
		break;

		case FN_BEM: {
			bem_data_t *data = (bem_data_t *)rx_msg->Data;

			charger_info->settings->bem_data = *data;

			if(charger_info->bem_received == 0) {
				charger_info->bem_received = 1;
				charger_info_report_status(charger_info, charger_info->state, CHARGER_INFO_STATUS_BEM_RECEIVED);
			}

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

static charger_state_handler_t state_handler_sz[] = {
	{
		.state = CHARGER_STATE_IDLE,
		.prepare = prepare_state_idle,
		.handle_request = handle_state_idle_request,
		.handle_response = handle_state_idle_response,
	},
	{
		.state = CHARGER_STATE_CHM,
		.prepare = prepare_state_chm,
		.handle_request = handle_state_chm_request,
		.handle_response = handle_state_chm_response,
	},
	{
		.state = CHARGER_STATE_CRM,
		.prepare = prepare_state_crm,
		.handle_request = handle_state_crm_request,
		.handle_response = handle_state_crm_response,
	},
	{
		.state = CHARGER_STATE_CTS_CML,
		.prepare = prepare_state_cts_cml,
		.handle_request = handle_state_cts_cml_request,
		.handle_response = handle_state_cts_cml_response,
	},
	{
		.state = CHARGER_STATE_CRO,
		.prepare = prepare_state_cro,
		.handle_request = handle_state_cro_request,
		.handle_response = handle_state_cro_response,
	},
	{
		.state = CHARGER_STATE_CCS,
		.prepare = prepare_state_ccs,
		.handle_request = handle_state_ccs_request,
		.handle_response = handle_state_ccs_response,
	},
	{
		.state = CHARGER_STATE_CST,
		.prepare = prepare_state_cst,
		.handle_request = handle_state_cst_request,
		.handle_response = handle_state_cst_response,
	},
	{
		.state = CHARGER_STATE_CSD_CEM,
		.prepare = prepare_state_csd_cem,
		.handle_request = handle_state_csd_cem_request,
		.handle_response = handle_state_csd_cem_response,
	},
};

charger_state_handler_t *charger_get_state_handler(charger_state_t state)
{
	charger_state_handler_t *handler = NULL;
	int i;

	for(i = 0; i < ARRAY_SIZE(state_handler_sz); i++) {
		if(state_handler_sz[i].state == state) {
			handler = &state_handler_sz[i];
			break;
		}
	}

	return handler;
}
