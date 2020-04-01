

/*================================================================
 *
 *
 *   文件名称：bms_handler.c
 *   创 建 者：肖飞
 *   创建日期：2019年10月31日 星期四 14时18分53秒
 *   修改日期：2020年04月01日 星期三 11时16分32秒
 *   描    述：
 *
 *================================================================*/
#include "bms_handler.h"
#include "bms.h"
#include "bms_spec.h"
#include <string.h>
#define UDP_LOG
#include "task_probe_tool.h"

static int send_bms_multi_request_response(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bms_multi_request_response_t *data;
	multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.rx_des;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_REQUEST_PRIORITY, FN_MULTI_REQUEST, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_request_response_t);

	data = (bms_multi_request_response_t *)tx_msg.Data;

	data->head.type = FN_MULTI_REQUEST_RESPONSE_TYPE;
	data->packets = multi_packets_des->bms_data_multi_packets;
	data->packet_index = multi_packets_des->bms_data_multi_next_index;
	data->reserved_0 = 0xff;
	data->reserved_1 = 0xff;
	data->reserved_2 = 0x00;
	data->fn = multi_packets_des->bms_data_multi_fn;
	data->reserved_3 = 0x00;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int send_bms_multi_data_response(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bms_multi_data_response_t *data;
	multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.rx_des;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_REQUEST_PRIORITY, FN_MULTI_REQUEST, BMS_ADDR, CHARGER_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_data_response_t);

	data = (bms_multi_data_response_t *)tx_msg.Data;

	data->head.type = FN_MULTI_DATA_RESPONSE_TYPE;
	data->bytes = multi_packets_des->bms_data_multi_bytes;
	data->packets = multi_packets_des->bms_data_multi_packets;
	data->reserved_0 = 0xff;
	data->reserved_1 = 0x00;
	data->fn = multi_packets_des->bms_data_multi_fn;
	data->reserved_2 = 0x00;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static uint8_t *get_multi_packet_data_by_fn(bms_info_t *bms_info, uint8_t fn)
{
	uint8_t *data = NULL;

	switch(fn) {
		default:
			break;
	}

	return data;
}

static int handle_common_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_MULTI_REQUEST: {
			bms_multi_request_head_t *request_head = (bms_multi_request_head_t *)rx_msg->Data;

			switch(request_head->type) {
				case FN_MULTI_REQUEST_TYPE: {//处理多包请求
					bms_multi_request_t *data = (bms_multi_request_t *)rx_msg->Data;
					multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.rx_des;

					multi_packets_des->bms_data_multi_bytes = data->bytes;
					multi_packets_des->bms_data_multi_packets = data->packets;
					multi_packets_des->bms_data_multi_next_index = 1;
					multi_packets_des->bms_data_multi_fn = data->fn;
					multi_packets_des->bms_data = get_multi_packet_data_by_fn(bms_info, data->fn);

					if(multi_packets_des->bms_data) {
						send_bms_multi_request_response(bms_info);
						ret = 0;
					}
				}
				break;

				case FN_MULTI_REQUEST_RESPONSE_TYPE: {//处理多包请求响应
					multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.tx_des;
					bms_multi_request_response_t *data = (bms_multi_request_response_t *)rx_msg->Data;

					if((data->packets == multi_packets_des->bms_data_multi_packets)
					   && (data->fn == multi_packets_des->bms_data_multi_fn)) {
						multi_packets_des->bms_data_multi_next_index = data->packet_index;
						ret = 0;
					}
				}
				break;

				case FN_MULTI_DATA_RESPONSE_TYPE: {//处理多包数据响应
					multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.tx_des;
					bms_multi_data_response_t *data = (bms_multi_data_response_t *)rx_msg->Data;

					if((data->bytes == multi_packets_des->bms_data_multi_bytes) && (data->packets == multi_packets_des->bms_data_multi_packets) && (data->fn == multi_packets_des->bms_data_multi_fn)) {
						ret = 0;
					}
				}
				break;

				default:
					break;
			}
		}
		break;

		case FN_MULTI_DATA: {//处理多包数据
			bms_multi_data_t *data = (bms_multi_data_t *)rx_msg->Data;
			uint16_t received = (7 * (data->index - 1));
			uint16_t receive = 7;
			multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.rx_des;

			if(received < multi_packets_des->bms_data_multi_bytes) {
				if(receive > multi_packets_des->bms_data_multi_bytes - received) {
					receive = multi_packets_des->bms_data_multi_bytes - received;
				}

				if(data->index == multi_packets_des->bms_data_multi_next_index) {
					if(multi_packets_des->bms_data != NULL) {
						memcpy(multi_packets_des->bms_data + received, data->data, receive);
					}

					if(data->index < multi_packets_des->bms_data_multi_packets) {
						multi_packets_des->bms_data_multi_next_index++;
					} else if(data->index == multi_packets_des->bms_data_multi_packets) {
						send_bms_multi_data_response(bms_info);
					}
				}

				ret = 0;
			}
		}
		break;

		case FN_CST: {
			cst_data_t *data = (cst_data_t *)rx_msg->Data;
			bms_info->settings->cst_data = *data;

			set_bms_state(bms_info, BMS_STATE_BST);

			ret = 0;
		}
		break;

		default:
			break;
	}

	return ret;
}

static int send_multi_request(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.tx_des;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_REQUEST_PRIORITY, FN_MULTI_REQUEST, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_request_t);

	bms_multi_request_t *data = (bms_multi_request_t *)tx_msg.Data;
	data->head.type = FN_MULTI_REQUEST_TYPE;
	data->bytes = multi_packets_des->bms_data_multi_bytes;
	data->packets = multi_packets_des->bms_data_multi_packets;
	data->reserved_0 = 0xff;
	data->reserved_1 = 0x00;
	data->fn = multi_packets_des->bms_data_multi_fn;
	data->reserved_2 = 0x00;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int send_multi_data(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.tx_des;
	int i;

	tx_msg.ExtId = get_pdu_id(FN_MULTI_DATA_PRIORITY, FN_MULTI_DATA, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bms_multi_data_t);

	for(i = multi_packets_des->bms_data_multi_next_index; i <= multi_packets_des->bms_data_multi_packets; i = multi_packets_des->bms_data_multi_next_index) {
		bms_multi_data_t *data = (bms_multi_data_t *)tx_msg.Data;
		uint8_t sent_bytes = (7 * (i - 1));
		uint8_t send_bytes = 7;
		data->index = i;

		if(send_bytes > (multi_packets_des->bms_data_multi_bytes - sent_bytes)) {
			send_bytes = multi_packets_des->bms_data_multi_bytes - sent_bytes;
		}

		memset(data->data, 0, sizeof(bms_multi_data_t));

		if(multi_packets_des->bms_data != NULL) {
			memcpy(data->data, multi_packets_des->bms_data + sent_bytes, send_bytes);
		}

		ret = can_tx_data(can_info, &tx_msg, 5);

		if(ret != 0) {
			break;
		}

		multi_packets_des->bms_data_multi_next_index++;
	}

	return ret;
}

static int send_multi_packets(bms_info_t *bms_info, uint8_t *data, uint16_t fn, uint16_t bytes, uint16_t packets, uint32_t period)
{
	int ret = -1;
	multi_packets_des_t *multi_packets_des = &bms_info->multi_packets_info.tx_des;
	uint32_t ticks = osKernelSysTick();

	if(multi_packets_des->bms_data_multi_fn != FN_INVALID) {
		if(ticks - multi_packets_des->start_stamp >= period) {
			multi_packets_des->bms_data_multi_fn = FN_INVALID;
			multi_packets_des->bms_data = NULL;
		}
	}

	if(multi_packets_des->bms_data_multi_fn == FN_INVALID) {
		multi_packets_des->bms_data_multi_fn = fn;
		multi_packets_des->bms_data_multi_bytes = bytes;
		multi_packets_des->bms_data_multi_packets = packets;
		multi_packets_des->bms_data_multi_next_index = 0;
		multi_packets_des->bms_data = data;
		multi_packets_des->start_stamp = ticks;
		send_multi_request(bms_info);
	} else if(multi_packets_des->bms_data_multi_fn == fn) {
		if((multi_packets_des->bms_data_multi_next_index >= 1) && (multi_packets_des->bms_data_multi_next_index <= multi_packets_des->bms_data_multi_packets)) {
			send_multi_data(bms_info);

			if(multi_packets_des->bms_data_multi_next_index > multi_packets_des->bms_data_multi_packets) {
				multi_packets_des->bms_data_multi_fn = FN_INVALID;
				multi_packets_des->bms_data = NULL;
				ret = 0;
			}
		}
	}

	return ret;
}

static int prepare_state_idle(bms_info_t *bms_info)
{
	int ret = 0;
	set_gun_on_off(bms_info, 0);
	memset(&bms_info->settings->bst_data, 0, sizeof(bst_data_t));
	memset(&bms_info->settings->bem_data, 0, sizeof(bem_data_t));
	return ret;
}

static int handle_state_idle_request(bms_info_t *bms_info)
{
	int ret = 0;
	return ret;
}

static int handle_state_idle_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_CHM: {
			//idle状态下，可以在界面操作继电器,所以退出idle再设定继电器为断开
			set_gun_on_off(bms_info, 0);
			set_bms_state(bms_info, BMS_STATE_BHM);
			ret = 0;
		}
		break;

		case FN_CRM: {
			//idle状态下，可以在界面操作继电器,所以退出idle再设定继电器为断开
			set_gun_on_off(bms_info, 0);
			set_bms_state(bms_info, BMS_STATE_BRM);
			ret = 0;
		}
		break;

		default:
			break;
	}


	return ret;
}

static int prepare_state_bhm(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	bms_info->send_stamp = ticks - FN_BHM_SEND_PERIOD;
	bms_info->stamp = ticks;
	return ret;
}

static int send_bhm(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bhm_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_BHM_PRIORITY, FN_BHM, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bhm_data_t);

	data = (bhm_data_t *)tx_msg.Data;
	*data = bms_info->settings->bhm_data;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_bhm_request(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - bms_info->stamp >= FN_CRM_TIMEOUT) {//收到chm,30s内没收到brm,进入停止状态
		bms_info->settings->bem_data.u1.s.crm_00_timeout = 0x01;
		set_bms_state(bms_info, BMS_STATE_BSD_BEM);
	} else {
		if(ticks - bms_info->send_stamp >= FN_BHM_SEND_PERIOD) {
			if(bms_info->modbus_data->disable_bhm == 0) {
				send_bhm(bms_info);
				bms_info->send_stamp = ticks;
			}
		}
	}

	return ret;
}

static int handle_state_bhm_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(bms_info);

	if(ret == 0) {
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_CHM: {
			chm_data_t *data = (chm_data_t *)rx_msg->Data;
			bms_info->settings->chm_data = *data;

			ret = 0;
		}
		break;

		case FN_CRM: {
			crm_data_t *data = (crm_data_t *)rx_msg->Data;
			bms_info->settings->crm_data = *data;
			set_bms_state(bms_info, BMS_STATE_BRM);

			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static int prepare_state_brm(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	memset(&bms_info->settings->bst_data, 0, sizeof(bst_data_t));
	memset(&bms_info->settings->bem_data, 0, sizeof(bem_data_t));

	bms_info->send_stamp = ticks - FN_BRM_SEND_PERIOD;
	bms_info->stamp = ticks;
	bms_info->multi_packets_info.tx_des.bms_data_multi_fn = FN_INVALID;

	bms_info->settings->crm_data.crm_result = 0xff;

	return ret;
}

//static int send_brm(bms_info_t *bms_info)
//{
//	int ret = 0;
//	can_tx_msg_t tx_msg;
//	can_info_t *can_info = bms_info->can_info;
//
//	tx_msg.ExtId = get_pdu_id(FN_BRM_PRIORITY, FN_BRM, CHARGER_ADDR, BMS_ADDR);
//	tx_msg.IDE = CAN_ID_EXT;
//	tx_msg.RTR = CAN_RTR_DATA; //	tx_msg.DLC = sizeof(brm_data_t);
//
//	brm_data_t *data = (brm_data_t *)tx_msg.Data;
//	*data = bms_info->settings->brm_data.brm_data;
//
//	ret = can_tx_data(can_info, &tx_msg, 1000);
//
//	return ret;
//}

static int handle_state_brm_request(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - bms_info->stamp >= BMS_GENERIC_TIMEOUT) {//超时出错
		bms_info->settings->bem_data.u1.s.crm_aa_timeout = 0x01;

		set_bms_state(bms_info, BMS_STATE_BSD_BEM);
	} else {
		if(ticks - bms_info->send_stamp >= FN_BRM_SEND_PERIOD) {
			if(bms_info->modbus_data->disable_brm == 0) {
				//send_brm(bms_info);
				if(send_multi_packets(bms_info,
				                      (uint8_t *)&bms_info->settings->brm_data,
				                      FN_BRM,
				                      sizeof(brm_data_multi_t),
				                      (sizeof(brm_data_multi_t) + (7 - 1)) / 7,
				                      FN_BRM_SEND_PERIOD) == 0) {
					bms_info->send_stamp = ticks;
				}
			}
		}
	}

	return ret;
}

static int handle_state_brm_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(bms_info);

	if(ret == 0) {
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_CRM: {
			crm_data_t *data = (crm_data_t *)rx_msg->Data;
			bms_info->settings->crm_data = *data;

			if(data->crm_result == 0xaa) {
				set_bms_state(bms_info, BMS_STATE_BCP);
			}

			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static int prepare_state_bcp(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	bms_info->send_stamp = ticks - FN_BCP_SEND_PERIOD;
	bms_info->stamp = ticks;

	bms_info->multi_packets_info.tx_des.bms_data_multi_fn = FN_INVALID;
	return ret;
}

static int handle_state_bcp_request(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - bms_info->stamp >= BMS_GENERIC_TIMEOUT) {//超时出错
		bms_info->settings->bem_data.u2.s.cts_cml_timeout = 0x01;
		set_bms_state(bms_info, BMS_STATE_BSD_BEM);
	} else {
		if(ticks - bms_info->send_stamp >= FN_BCP_SEND_PERIOD) {
			if(bms_info->modbus_data->disable_bcp == 0) {
				if(send_multi_packets(bms_info,
				                      (uint8_t *)&bms_info->settings->bcp_data,
				                      FN_BCP,
				                      sizeof(bcp_data_multi_t),
				                      (sizeof(bcp_data_multi_t) + (7 - 1)) / 7,
				                      FN_BCP_SEND_PERIOD) == 0) {
					bms_info->send_stamp = ticks;
				}
			}
		}
	}

	return ret;
}

static int handle_state_bcp_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(bms_info);

	if(ret == 0) {
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_CML: {
			cml_data_t *data = (cml_data_t *)rx_msg->Data;
			bms_info->settings->cml_data = *data;

			set_bms_state(bms_info, BMS_STATE_BRO);

			ret = 0;
		}
		break;

		case FN_CTS: {
			cts_data_t *data = (cts_data_t *)rx_msg->Data;
			bms_info->settings->cts_data = *data;

			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static int prepare_state_bro(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	bms_info->send_stamp = ticks - FN_BRO_SEND_PERIOD;
	bms_info->stamp = ticks;
	bms_info->settings->bro_data.bro_result = 0x00;
	bms_info->settings->cro_data.cro_result = 0xff;

	return ret;
}

static int send_bro(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bro_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_BRO_PRIORITY, FN_BRO, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bro_data_t);

	data = (bro_data_t *)tx_msg.Data;

	*data = bms_info->settings->bro_data;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static uint8_t is_bro_ready(bms_info_t *bms_info)
{
	uint8_t ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - bms_info->stamp >= 1000) {
		ret = 1;
	}

	return ret;
}

static int handle_state_bro_request(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();
	uint32_t cro_timeout = (bms_info->settings->cro_data.cro_result == 0xff) ? BMS_GENERIC_TIMEOUT : FN_CRO_AA_TIMEOUT;

	if(ticks - bms_info->stamp >= cro_timeout) {//超时出错
		bms_info->settings->bem_data.u2.s.cro_timeout = 0x01;
		set_bms_state(bms_info, BMS_STATE_BSD_BEM);
	} else {
		if(ticks - bms_info->send_stamp >= FN_BRO_SEND_PERIOD) {
			if(is_bro_ready(bms_info) == 1) {
				bms_info->settings->bro_data.bro_result = 0xaa;
				set_gun_on_off(bms_info, 1);
			}

			if(bms_info->modbus_data->disable_bro == 0) {
				send_bro(bms_info);
				bms_info->send_stamp = ticks;
			}
		}
	}

	return ret;
}

static int handle_state_bro_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(bms_info);

	if(ret == 0) {
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_CRO: {
			cro_data_t *data = (cro_data_t *)rx_msg->Data;
			bms_info->settings->cro_data = *data;

			if(bms_info->settings->cro_data.cro_result == 0xaa) {
				set_bms_state(bms_info, BMS_STATE_BCL_BCS_BSM_BMV_BMT_BSP);
			}

			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static int prepare_state_bcl_bcs_bsm_bmv_bmt_bsp(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	bms_info->send_stamp = ticks - FN_BCL_SEND_PERIOD;//bcl
	bms_info->send_stamp_1 = ticks - FN_BCS_SEND_PERIOD;//bcs
	bms_info->send_stamp_2 = ticks - FN_BSM_SEND_PERIOD;//bsm
	bms_info->stamp = ticks;

	bms_info->received_ccs = 0;

	bms_info->multi_packets_info.tx_des.bms_data_multi_fn = FN_INVALID;

	return ret;
}

static int send_bcl(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bcl_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_BCL_PRIORITY, FN_BCL, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bcl_data_t);

	data = (bcl_data_t *)tx_msg.Data;

	*data = bms_info->settings->bcl_data;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

//static int send_bcs(bms_info_t *bms_info)
//{
//	int ret = 0;
//	can_tx_msg_t tx_msg;
//	can_info_t *can_info = bms_info->can_info;
//	bcs_data_t *data;
//
//	tx_msg.ExtId = get_pdu_id(FN_BCS_PRIORITY, FN_BCS, CHARGER_ADDR, BMS_ADDR);
//	tx_msg.IDE = CAN_ID_EXT;
//	tx_msg.RTR = CAN_RTR_DATA;
//	tx_msg.DLC = sizeof(bcs_data_t);
//
//	data = (bcs_data_t *)tx_msg.Data;
//
//	*data = bms_info->settings->bcs_data;
//
//	ret = can_tx_data(can_info, &tx_msg, 1000);
//
//	return ret;
//}

static int send_bsm(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bsm_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_BSM_PRIORITY, FN_BSM, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bsm_data_t);

	data = (bsm_data_t *)tx_msg.Data;

	*data = bms_info->settings->bsm_data;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_bcl_bcs_bsm_bmv_bmt_bsp_request(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - bms_info->stamp >= FN_CCS_TIMEOUT) {//超时出错
		bms_info->settings->bem_data.u3.s.ccs_timeout = 0x01;
		set_bms_state(bms_info, BMS_STATE_BSD_BEM);
	} else {
		if(ticks - bms_info->send_stamp >= FN_BCL_SEND_PERIOD) {
			if(bms_info->modbus_data->disable_bcl == 0) {
				send_bcl(bms_info);
				bms_info->send_stamp = ticks;
			}
		}

		if(ticks - bms_info->send_stamp_1 >= FN_BCS_SEND_PERIOD) {
			if(bms_info->modbus_data->disable_bcs == 0) {
				if(send_multi_packets(bms_info,
				                      (uint8_t *)&bms_info->settings->bcs_data,
				                      FN_BCS,
				                      sizeof(bcs_data_t),
				                      (sizeof(bcs_data_t) + (7 - 1)) / 7,
				                      FN_BCS_SEND_PERIOD) == 0) {
					bms_info->send_stamp_1 = ticks;
				}
			}
		}

		if(bms_info->received_ccs == 1) {
			if(ticks - bms_info->send_stamp_2 >= FN_BSM_SEND_PERIOD) {
				if(bms_info->modbus_data->disable_bsm == 0) {
					send_bsm(bms_info);
					bms_info->send_stamp_2 = ticks;
				}
			}
		}
	}

	return ret;
}

static int handle_state_bcl_bcs_bsm_bmv_bmt_bsp_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	ret = handle_common_response(bms_info);

	if(ret == 0) {
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_CCS: {
			ccs_data_t *data = (ccs_data_t *)rx_msg->Data;
			bms_info->settings->ccs_data = *data;

			bms_info->received_ccs = 1;
			bms_info->stamp = osKernelSysTick();

			ret = 0;
		}
		break;


		default: {
		}
		break;
	}

	return ret;
}

static int prepare_state_bst(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	bms_info->send_stamp = ticks - FN_BST_SEND_PERIOD;//bcl
	bms_info->stamp = ticks;
	bms_info->stamp_1 = ticks;//保存首次发cst的时间

	return ret;
}

static int send_bst(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bst_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_BST_PRIORITY, FN_BST, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bst_data_t);

	data = (bst_data_t *)tx_msg.Data;

	*data = bms_info->settings->bst_data;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int handle_state_bst_request(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(ticks - bms_info->stamp >= BMS_GENERIC_TIMEOUT) {//超时出错
		bms_info->settings->bem_data.u3.s.cst_timeout = 0x01;
		set_bms_state(bms_info, BMS_STATE_BSD_BEM);
	} else {
		if(ticks - bms_info->send_stamp >= FN_BST_SEND_PERIOD) {
			if(bms_info->modbus_data->disable_bst == 0) {
				send_bst(bms_info);
				bms_info->send_stamp = ticks;
			}
		} else {
			if((bms_info->settings->cst_data.u1.s.stop_reason_condition == 0x01) || (bms_info->settings->cst_data.u1.s.stop_reason_manual == 0x01) || (bms_info->settings->cst_data.u1.s.stop_reason_fault == 0x01)) {
				set_bms_state(bms_info, BMS_STATE_BSD_BEM);
			}
		}
	}

	return ret;
}

static int handle_state_bst_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_CST: {
			cst_data_t *data = (cst_data_t *)rx_msg->Data;
			bms_info->settings->cst_data = *data;

			set_bms_state(bms_info, BMS_STATE_BSD_BEM);

			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static int prepare_state_bsd_bem(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	bms_info->stamp = ticks;

	bms_info->send_stamp = ticks - FN_BEM_SEND_PERIOD;//bcl
	bms_info->send_stamp_1 = ticks - FN_BSD_SEND_PERIOD;//bcl

	bms_info->received_csd = 0;
	bms_info->received_cem = 0;
	bms_info->sent_bem = 0;

	return ret;
}

static int send_bsd(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bsd_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_BSD_PRIORITY, FN_BSD, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bsd_data_t);

	data = (bsd_data_t *)tx_msg.Data;

	*data = bms_info->settings->bsd_data;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static int send_bem(bms_info_t *bms_info)
{
	int ret = 0;
	can_tx_msg_t tx_msg;
	can_info_t *can_info = bms_info->can_info;
	bem_data_t *data;

	tx_msg.ExtId = get_pdu_id(FN_BEM_PRIORITY, FN_BEM, CHARGER_ADDR, BMS_ADDR);
	tx_msg.IDE = CAN_ID_EXT;
	tx_msg.RTR = CAN_RTR_DATA;
	tx_msg.DLC = sizeof(bem_data_t);

	data = (bem_data_t *)tx_msg.Data;

	*data = bms_info->settings->bem_data;

	ret = can_tx_data(can_info, &tx_msg, 1000);

	return ret;
}

static uint8_t is_bem_valid(bms_info_t *bms_info)
{
	uint8_t ret = 0;
	bem_data_t *bem_data = &bms_info->settings->bem_data;

	if(bem_data->u1.s.crm_00_timeout != 0x00) {
		ret = 1;
	}

	if(bem_data->u1.s.crm_aa_timeout != 0x00) {
		ret = 1;
	}

	if(bem_data->u2.s.cts_cml_timeout != 0x00) {
		ret = 1;
	}

	if(bem_data->u2.s.cro_timeout != 0x00) {
		ret = 1;
	}

	if(bem_data->u3.s.ccs_timeout != 0x00) {
		ret = 1;
	}

	if(bem_data->u3.s.cst_timeout != 0x00) {
		ret = 1;
	}

	if(bem_data->u4.s.csd_timeout != 0x00) {
		ret = 1;
	}

	if(bem_data->u4.s.other != 0x00) {
		ret = 1;
	}

	return ret;
}

static uint8_t is_no_current(bms_info_t *bms_info)//电流降到5a以下----无法检测电流，把判断条件改为辅助电源断开
{
	uint8_t ret = 0;

	if(is_bms_poweron_enable(bms_info) == 0) {//输助电源断开
		ret = 1;
	}

	return ret;
}

static int handle_state_bsd_bem_request(bms_info_t *bms_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	if(is_bem_valid(bms_info) == 1) {
		if((bms_info->sent_bem == 0) || (is_no_current(bms_info) == 0)) {
			if(ticks - bms_info->send_stamp >= FN_BEM_SEND_PERIOD) {
				if(bms_info->modbus_data->disable_bem == 0) {
					send_bem(bms_info);
					bms_info->send_stamp = ticks;
					bms_info->sent_bem = 1;
				}
			}
		}
	} else {
		if(bms_info->received_csd == 0) {
			if(ticks - bms_info->send_stamp_1 >= FN_BSD_SEND_PERIOD) {
				if(bms_info->modbus_data->disable_bsd == 0) {
					send_bsd(bms_info);
					bms_info->send_stamp_1 = ticks;
				}
			}
		}

	}

	if((bms_info->received_cem == 1) || (bms_info->sent_bem == 1)) {
		if(ticks - bms_info->stamp >= FN_CRM_TIMEOUT) {//收到cem,或发出bem, 30秒没收到crm,停止
			set_bms_state(bms_info, BMS_STATE_IDLE);
		}
	} else {//没收到cem或发出bem,最多等10s停机
		if(is_no_current(bms_info) == 0) {//电流没到5a以下
			if(ticks - bms_info->stamp_1 >= FN_BSD_TIMEOUT) {//bst发送起超过10s电流没到5a以下
				set_bms_state(bms_info, BMS_STATE_IDLE);
			}
		} else {
			if(bms_info->received_csd == 1) {
				if(ticks - bms_info->stamp_1 >= BMS_GENERIC_TIMEOUT) {
					set_bms_state(bms_info, BMS_STATE_IDLE);
				}
			}
		}
	}

	return ret;
}

static int handle_state_bsd_bem_response(bms_info_t *bms_info)
{
	int ret = -1;
	can_rx_msg_t *rx_msg = can_get_msg(bms_info->can_info);
	u_pdu_head_t head;

	if(rx_msg == NULL) {
		return ret;
	}

	head.v = rx_msg->ExtId;

	switch(head.pdu.pf) {
		case FN_CSD: {
			csd_data_t *data = (csd_data_t *)rx_msg->Data;
			bms_info->settings->csd_data = *data;

			if(bms_info->received_csd == 0) {
				bms_info->received_csd = 1;
			}

			ret = 0;
		}
		break;

		case FN_CEM: {
			cem_data_t *data = (cem_data_t *)rx_msg->Data;
			bms_info->settings->cem_data = *data;

			if(bms_info->received_cem == 0) {
				bms_info->received_cem = 1;
			}

			ret = 0;
		}
		break;

		case FN_CRM: {
			crm_data_t *data = (crm_data_t *)rx_msg->Data;
			bms_info->settings->crm_data = *data;
			set_bms_state(bms_info, BMS_STATE_BRM);

			ret = 0;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static bms_state_handler_t state_handler_sz[] = {
	{
		.state = BMS_STATE_IDLE,
		.prepare = prepare_state_idle,
		.handle_request = handle_state_idle_request,
		.handle_response = handle_state_idle_response,
	},
	{
		.state = BMS_STATE_BHM,
		.prepare = prepare_state_bhm,
		.handle_request = handle_state_bhm_request,
		.handle_response = handle_state_bhm_response,
	},
	{
		.state = BMS_STATE_BRM,
		.prepare = prepare_state_brm,
		.handle_request = handle_state_brm_request,
		.handle_response = handle_state_brm_response,
	},
	{
		.state = BMS_STATE_BCP,
		.prepare = prepare_state_bcp,
		.handle_request = handle_state_bcp_request,
		.handle_response = handle_state_bcp_response,
	},
	{
		.state = BMS_STATE_BRO,
		.prepare = prepare_state_bro,
		.handle_request = handle_state_bro_request,
		.handle_response = handle_state_bro_response,
	},
	{
		.state = BMS_STATE_BCL_BCS_BSM_BMV_BMT_BSP,
		.prepare = prepare_state_bcl_bcs_bsm_bmv_bmt_bsp,
		.handle_request = handle_state_bcl_bcs_bsm_bmv_bmt_bsp_request,
		.handle_response = handle_state_bcl_bcs_bsm_bmv_bmt_bsp_response,
	},
	{
		.state = BMS_STATE_BST,
		.prepare = prepare_state_bst,
		.handle_request = handle_state_bst_request,
		.handle_response = handle_state_bst_response,
	},
	{
		.state = BMS_STATE_BSD_BEM,
		.prepare = prepare_state_bsd_bem,
		.handle_request = handle_state_bsd_bem_request,
		.handle_response = handle_state_bsd_bem_response,
	},
};

bms_state_handler_t *bms_get_state_handler(bms_state_t state)
{
	bms_state_handler_t *handler = NULL;
	int i;

	for(i = 0; i < (sizeof(state_handler_sz) / sizeof(bms_state_handler_t)); i++) {
		if(state_handler_sz[i].state == state) {
			handler = &state_handler_sz[i];
			break;
		}
	}

	return handler;
}
