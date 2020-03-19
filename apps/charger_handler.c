

/*================================================================
 *
 *
 *   文件名称：charger_handler.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 14时18分42秒
 *   修改日期：2020年03月13日 星期五 11时23分24秒
 *   描    述：
 *
 *================================================================*/
#include "charger_handler.h"
#include "charger.h"
#include "bms_spec.h"
#include <string.h>

static int send_bms_multi_request_response(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	bms_multi_request_response_t *data;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_REQUEST_PRIORITY, FN_MULTI_REQUEST, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_request_response_t);

	data = (bms_multi_request_response_t *)tx_msg.Data;

	data->head.type = FN_MULTI_REQUEST_RESPONSE_TYPE;
	data->packets = charger_info->bms_data_multi_packets;
	data->packet_index = charger_info->bms_data_multi_next_index;
	data->reserved_0 = 0xff;
	data->reserved_1 = 0xff;
	data->reserved_2 = 0x00;
	data->fn = charger_info->bms_data_multi_fn;
	data->reserved_3 = 0x00;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int send_bms_multi_data_response(charger_info_t *charger_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = charger_info->can_info;
	bms_multi_data_response_t *data;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_REQUEST_PRIORITY, FN_MULTI_REQUEST, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_data_response_t);

	data = (bms_multi_data_response_t *)tx_msg.Data;

	data->head.type = FN_MULTI_DATA_RESPONSE_TYPE;
	data->bytes = charger_info->bms_data_multi_bytes;
	data->packets = charger_info->bms_data_multi_packets;
	data->reserved_0 = 0xff;
	data->reserved_1 = 0x00;
	data->fn = charger_info->bms_data_multi_fn;
	data->reserved_2 = 0x00;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_common_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_MULTI_REQUEST: {
			bms_multi_request_head_t *request_head = (bms_multi_request_head_t *)rx_msg->Data;

			switch(request_head->type) {
				case FN_MULTI_REQUEST_TYPE: {
					bms_multi_request_t *data = (bms_multi_request_t *)rx_msg->Data;

					charger_info->bms_data_multi_bytes = data->bytes;
					charger_info->bms_data_multi_packets = data->packets;
					charger_info->bms_data_multi_next_index = 1;
					charger_info->bms_data_multi_fn = data->fn;
					send_bms_multi_request_response(charger_info);
					ret = 0;
				}
				break;

				case FN_MULTI_REQUEST_RESPONSE_TYPE: {
					bms_multi_request_response_t *data = (bms_multi_request_response_t *)rx_msg->Data;

					if((data->packets == charger_info->bms_data_multi_packets)
					   && (data->fn == charger_info->bms_data_multi_fn)) {
						charger_info->bms_data_multi_next_index = data->packet_index;
						ret = 0;
					}
				}
				break;

				case FN_MULTI_DATA_RESPONSE_TYPE: {
					bms_multi_data_response_t *data = (bms_multi_data_response_t *)rx_msg->Data;

					if((data->bytes == charger_info->bms_data_multi_bytes) && (data->packets == charger_info->bms_data_multi_packets) && (data->fn == charger_info->bms_data_multi_fn)) {
						ret = 0;
					}
				}
				break;

				default:
					break;
			}
		}
		break;

		case FN_MULTI_DATA: {
			bms_multi_data_t *data = (bms_multi_data_t *)rx_msg->Data;

			if(data->index == charger_info->bms_data_multi_next_index) {
				memcpy(charger_info->bms_data_multi_sz + (7 * (data->index - 1)), data->data, 7);

				if(data->index < charger_info->bms_data_multi_packets) {
					charger_info->bms_data_multi_next_index++;
				} else if(data->index == charger_info->bms_data_multi_packets) {
					send_bms_multi_data_response(charger_info);
				}
			}

			ret = 0;
		}
		break;

		case FN_BST: {
			bst_data_t *data = (bst_data_t *)rx_msg->Data;

			charger_info->settings->bst_data = *data;

			set_charger_state(charger_info, CHARGER_STATE_CST);

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

static uint8_t is_bms_data_multi_received(charger_info_t *charger_info, bms_fn_t fn)
{
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;
	uint8_t ret = 0;

	head.v = rx_msg->ExtId;

	if(head.pdu.pf == FN_MULTI_DATA) {
		bms_multi_data_t *data = (bms_multi_data_t *)rx_msg->Data;

		if((data->index == charger_info->bms_data_multi_packets) && (data->index == charger_info->bms_data_multi_next_index) && (charger_info->bms_data_multi_fn == fn)) {
			ret = 1;
		}
	}

	return ret;
}

static int prepare_state_idle(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	charger_info->stamp = ticks;
	return ret;
}

static int handle_state_idle_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - charger_info->stamp >= BMS_GENERIC_TIMEOUT) {
		set_charger_state(charger_info, CHARGER_STATE_CHM);
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

	charger_info->send_stamp = ticks;
	charger_info->stamp = ticks;

	charger_info->insulation_check_stamp = ticks;
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

	charger_info->settings->chm_data.version_0 = 0x0001;
	charger_info->settings->chm_data.version_1 = 0x01;
	data->version_0 = charger_info->settings->chm_data.version_0;
	data->version_1 = charger_info->settings->chm_data.version_1;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_chm_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - charger_info->stamp >= BMS_GENERIC_TIMEOUT) {//定时发送
		charger_info->settings->cem_data.u1.s.brm_timeout = 0x01;//收不到bhm, 报brm_timeout
		set_charger_state(charger_info, CHARGER_STATE_CST);
	} else {
		//绝缘检测
		if(ticks - charger_info->insulation_check_stamp >= (3 * 1000)) {
			set_charger_state(charger_info, CHARGER_STATE_CRM);
		} else {
			if(ticks - charger_info->send_stamp >= FN_CHM_SEND_PERIOD) {
				send_chm(charger_info);
				charger_info->send_stamp = ticks;
			}
		}
	}

	return ret;
}

static int handle_state_chm_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	ret = handle_common_response(charger_info);

	if(ret == 0) {
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BHM: {
			bhm_data_t *data = (bhm_data_t *)rx_msg->Data;

			charger_info->settings->bhm_data.max_charge_voltage = data->max_charge_voltage;

			charger_info->stamp = osKernelSysTick();

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

	charger_info->send_stamp = ticks;
	charger_info->stamp = ticks;

	charger_info->settings->crm_data.crm_result = 0x00;
	charger_info->settings->crm_data.charger_sn = 0x01;
	charger_info->bms_data_multi_next_index = 0;

	charger_info->brm_received = 0;
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

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_crm_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - charger_info->stamp >= BMS_GENERIC_TIMEOUT) {//定时发送
		if(charger_info->brm_received == 0) {
			charger_info->settings->cem_data.u1.s.brm_timeout = 0x01;
		} else {
			charger_info->settings->cem_data.u2.s.bcp_timeout = 0x01;
		}

		set_charger_state(charger_info, CHARGER_STATE_CST);
	} else {
		if(ticks - charger_info->send_stamp >= FN_CRM_SEND_PERIOD) {
			send_crm(charger_info);
			charger_info->send_stamp = ticks;
		}
	}

	return ret;
}

static int handle_state_crm_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	ret = handle_common_response(charger_info);

	if(ret == 0) {
		if(is_bms_data_multi_received(charger_info, FN_BRM)) {
			brm_data_multi_t *data = (brm_data_multi_t *)charger_info->bms_data_multi_sz;

			charger_info->settings->brm_data = *data;

			if(charger_info->brm_received == 0) {
				charger_info->settings->crm_data.crm_result = 0xaa;
				charger_info->brm_received = 1;
				charger_info->stamp = osKernelSysTick();
			}
		} else if(is_bms_data_multi_received(charger_info, FN_BCP)) {//bcp timeout
			bcp_data_multi_t *data = (bcp_data_multi_t *)charger_info->bms_data_multi_sz;

			charger_info->settings->bcp_data = *data;

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

	charger_info->send_stamp = ticks;
	charger_info->send_stamp_1 = ticks;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

	charger_info->settings->cts_data.S = 0xff;
	charger_info->settings->cts_data.M = 0xff;
	charger_info->settings->cts_data.H = 0xff;
	charger_info->settings->cts_data.d = 0xff;
	charger_info->settings->cts_data.m = 0xff;
	charger_info->settings->cts_data.Y = 0xffff;

	charger_info->settings->cml_data.max_output_voltage = 7500;
	charger_info->settings->cml_data.min_output_voltage = 2000;
	charger_info->settings->cml_data.max_output_current = 4000 - (100 * 10);
	charger_info->settings->cml_data.min_output_current = 4000 - (2.5 * 10);

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

	ret = can_tx_data(can_info, &tx_msg, 1000);

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

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_cts_cml_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - charger_info->stamp >= BMS_GENERIC_TIMEOUT) {//定时发送
		charger_info->settings->cem_data.u2.s.bro_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	} else if(ticks - charger_info->stamp_1 >= FN_BRO_0xAA_TIMEOUT) {
		charger_info->settings->cem_data.u2.s.bro_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	} else {
		if(ticks - charger_info->send_stamp >= FN_CTS_SEND_PERIOD) {
			send_cts(charger_info);
			charger_info->send_stamp = ticks;
		}

		if(ticks - charger_info->send_stamp_1 >= FN_CML_SEND_PERIOD) {
			send_cml(charger_info);
			charger_info->send_stamp_1 = ticks;
		}
	}

	return ret;
}

static int handle_state_cts_cml_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

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

	charger_info->send_stamp = ticks;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

	charger_info->settings->cro_data.cro_result = 0xaa;
	charger_info->bcl_received = 0;
	charger_info->bcs_received = 0;

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

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_cro_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - charger_info->stamp >= FN_BCL_TIMEOUT) {//定时发送
		charger_info->settings->cem_data.u3.s.bcl_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	} else if(ticks - charger_info->stamp_1 >= BMS_GENERIC_TIMEOUT) {//定时发送
		charger_info->settings->cem_data.u3.s.bcs_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	} else if(ticks - charger_info->send_stamp >= FN_CRO_SEND_PERIOD) {
		send_cro(charger_info);
		charger_info->send_stamp = ticks;
	}

	return ret;
}

static int handle_state_cro_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	ret = handle_common_response(charger_info);

	if(ret == 0) {
		if(is_bms_data_multi_received(charger_info, FN_BCS)) {
			bcs_data_t *data = (bcs_data_t *)charger_info->bms_data_multi_sz;

			charger_info->settings->bcs_data = *data;

			charger_info->stamp_1 = osKernelSysTick();

			charger_info->bcs_received = 1;
		}
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BCL: {
			bcl_data_t *data = (bcl_data_t *)rx_msg->Data;

			charger_info->settings->bcl_data = *data;

			charger_info->stamp = osKernelSysTick();

			charger_info->bcl_received = 1;

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

	charger_info->send_stamp = ticks;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

	charger_info->settings->ccs_data.output_voltage = 6640;
	charger_info->settings->ccs_data.output_current = 4000 - 192;
	charger_info->settings->ccs_data.total_charge_time = 13;
	charger_info->settings->ccs_data.u1.s.charge_enable = 0x01;

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
	*data = charger_info->settings->ccs_data;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_ccs_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - charger_info->stamp >= FN_BCL_TIMEOUT) {//定时发送
		charger_info->settings->cem_data.u3.s.bcl_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	} else if(ticks - charger_info->stamp_1 >= BMS_GENERIC_TIMEOUT) {//定时发送
		charger_info->settings->cem_data.u3.s.bcs_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CST);
	} else {
		if(ticks - charger_info->send_stamp >= FN_CCS_SEND_PERIOD) {
			send_ccs(charger_info);
			charger_info->send_stamp = ticks;
		}
	}

	return ret;
}

static int handle_state_ccs_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	ret = handle_common_response(charger_info);

	if(ret == 0) {
		if(is_bms_data_multi_received(charger_info, FN_BCS)) {
			bcs_data_t *data = (bcs_data_t *)charger_info->bms_data_multi_sz;

			charger_info->settings->bcs_data = *data;

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

			charger_info->stamp = osKernelSysTick();

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

	charger_info->send_stamp = ticks;
	charger_info->stamp = ticks;

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

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_cst_request(charger_info_t *charger_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - charger_info->stamp >= BMS_GENERIC_TIMEOUT) {//定时发送
		charger_info->settings->cem_data.u3.s.bst_timeout = 0x01;
		set_charger_state(charger_info, CHARGER_STATE_CSD_CEM);
	} else {
		if(ticks - charger_info->send_stamp >= FN_CST_SEND_PERIOD) {
			send_cst(charger_info);
			charger_info->send_stamp = ticks;
		}
	}

	return ret;
}

static int handle_state_cst_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	//ret = handle_common_response(charger_info);

	//if(ret == 0) {
	//}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BST: {
			bst_data_t *data = (bst_data_t *)rx_msg->Data;

			charger_info->settings->bst_data = *data;

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

	charger_info->send_stamp = ticks;
	charger_info->send_stamp_1 = ticks;
	charger_info->stamp = ticks;
	charger_info->stamp_1 = ticks;

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

	ret = can_tx_data(can_info, &tx_msg, 1000);

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

	ret = can_tx_data(can_info, &tx_msg, 1000);

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
	uint32_t ticks = osKernelSysTick();

	if(ticks - charger_info->stamp >= FN_BSD_TIMEOUT + (1 * 1000)) {//关掉辅助电源/重新进入crm 停止发送
		set_charger_state(charger_info, CHARGER_STATE_IDLE);
	} else if(ticks - charger_info->stamp_1 >= FN_BSD_TIMEOUT) {//10秒结束
		charger_info->settings->cem_data.u4.s.bsd_timeout = 0x01;
	}

	if(ticks - charger_info->send_stamp >= FN_CST_SEND_PERIOD) {
		send_csd(charger_info);
		charger_info->send_stamp = ticks;
	}

	if(is_cem_valid(charger_info) == 1) {
		if(ticks - charger_info->send_stamp_1 >= FN_CEM_SEND_PERIOD) {
			send_cem(charger_info);
			charger_info->send_stamp_1 = ticks;
		}
	}

	return ret;
}

static int handle_state_csd_cem_response(charger_info_t *charger_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(charger_info->can_info);
	u_pdu_head_t head;

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_BSD: {
			bsd_data_t *data = (bsd_data_t *)rx_msg->Data;

			charger_info->settings->bsd_data = *data;

			charger_info->stamp = osKernelSysTick();

			ret = 0;
		}
		break;

		case FN_BEM: {
			bem_data_t *data = (bem_data_t *)rx_msg->Data;

			charger_info->settings->bem_data = *data;

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

	for(i = 0; i < (sizeof(state_handler_sz) / sizeof(charger_state_handler_t)); i++) {
		if(state_handler_sz[i].state == state) {
			handler = &state_handler_sz[i];
			break;
		}
	}

	return handler;
}
