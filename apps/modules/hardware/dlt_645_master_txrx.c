

/*================================================================
 *
 *
 *   文件名称：dlt_645_master_txrx.c
 *   创 建 者：肖飞
 *   创建日期：2020年05月21日 星期四 10时19分55秒
 *   修改日期：2021年07月08日 星期四 11时26分45秒
 *   描    述：
 *
 *================================================================*/
#include "dlt_645_master_txrx.h"
#include <string.h>
#include "object_class.h"

#define LOG_DISABLE
#include "log.h"

static object_class_t *dlt_645_master_class = NULL;

static void free_dlt_645_master_info(dlt_645_master_info_t *dlt_645_master_info)
{
	if(dlt_645_master_info == NULL) {
		return;
	}

	if(dlt_645_master_info->uart_info) {
		dlt_645_master_info->uart_info = NULL;
		dlt_645_master_info->rx_size = 0;
		dlt_645_master_info->tx_size = 0;
	}

	os_free(dlt_645_master_info);
}

static dlt_645_master_info_t *alloc_dlt_645_master_info(uart_info_t *uart_info)
{
	dlt_645_master_info_t *dlt_645_master_info = NULL;

	if(uart_info == NULL) {
		return dlt_645_master_info;
	}

	dlt_645_master_info = (dlt_645_master_info_t *)os_calloc(1, sizeof(dlt_645_master_info_t));

	if(dlt_645_master_info == NULL) {
		return dlt_645_master_info;
	}

	dlt_645_master_info->uart_info = uart_info;
	dlt_645_master_info->rx_timeout = 100;
	dlt_645_master_info->tx_timeout = 100;

	return dlt_645_master_info;
}

static int object_filter(void *o, void *ctx)
{
	int ret = -1;
	dlt_645_master_info_t *dlt_645_master_info = (dlt_645_master_info_t *)o;
	uart_info_t *uart_info = (uart_info_t *)ctx;

	if(dlt_645_master_info->uart_info == uart_info) {
		ret = 0;
	}

	return ret;
}

dlt_645_master_info_t *get_or_alloc_dlt_645_master_info(uart_info_t *uart_info)
{
	dlt_645_master_info_t *dlt_645_master_info = NULL;

	os_enter_critical();

	if(dlt_645_master_class == NULL) {
		dlt_645_master_class = object_class_alloc();
	}

	os_leave_critical();

	dlt_645_master_info = (dlt_645_master_info_t *)object_class_get_or_alloc_object(dlt_645_master_class, object_filter, uart_info, (object_alloc_t)alloc_dlt_645_master_info, (object_free_t)free_dlt_645_master_info);

	return dlt_645_master_info;
}

static int dlt_645_master_request(dlt_645_master_info_t *dlt_645_master_info, dlt_645_addr_t *addr, uint8_t fn, uint8_t *in, size_t in_len, uint8_t **out, size_t out_len)
{
	int ret = -1;
	int rx_size;
	int i;
	uint8_t crc;
	uint8_t *data;
	u_dlt_645_control_t u_dlt_645_control;

	u_dlt_645_control.v = 0;
	u_dlt_645_control.s.fn = fn;

	dlt_645_frame_head_t *dlt_645_frame_head = (dlt_645_frame_head_t *)dlt_645_master_info->tx_buffer;
	dlt_645_frame_data_t *dlt_645_frame_data = (dlt_645_frame_data_t *)(dlt_645_frame_head + 1);
	dlt_645_frame_tail_t *dlt_645_frame_tail = (dlt_645_frame_tail_t *)(dlt_645_frame_data->data + in_len);

	dlt_645_frame_head->addr_start_flag = 0x68;
	memcpy(&dlt_645_frame_head->addr, addr, sizeof(dlt_645_addr_t));
	dlt_645_frame_head->content_start_flag = 0x68;

	dlt_645_frame_head->control.v = u_dlt_645_control.v;
	dlt_645_frame_head->len = sizeof(dlt_645_frame_data_t) + in_len;

	if(in != NULL) {
		memcpy(dlt_645_frame_data->data, in, in_len);
	}

	data = (uint8_t *)dlt_645_frame_data;

	for(i = 0; i < dlt_645_frame_head->len; i++) {
		data[i] = data[i] + 0x33;
	}

	dlt_645_frame_tail->crc = sum_crc8((uint8_t *)dlt_645_frame_head, (uint8_t *)&dlt_645_frame_tail->crc - (uint8_t *)dlt_645_frame_head);
	dlt_645_frame_tail->end_flag = 0x16;

	dlt_645_master_info->tx_size = (uint8_t *)(dlt_645_frame_tail + 1) - (uint8_t *)dlt_645_frame_head;
	dlt_645_master_info->rx_size = sizeof(dlt_645_frame_head_t) + sizeof(dlt_645_frame_data_t) + out_len + sizeof(dlt_645_frame_tail_t);

	rx_size = uart_tx_rx_data(dlt_645_master_info->uart_info,
	                          dlt_645_master_info->tx_buffer, dlt_645_master_info->tx_size,
	                          dlt_645_master_info->rx_buffer, dlt_645_master_info->rx_size,
	                          dlt_645_master_info->rx_timeout);

	if(rx_size != dlt_645_master_info->rx_size) {
		debug("rx_size:%d, dlt_645_master_info->rx_size:%d", rx_size, dlt_645_master_info->rx_size);
		return ret;
	}

	dlt_645_frame_head = (dlt_645_frame_head_t *)dlt_645_master_info->rx_buffer;
	dlt_645_frame_data = (dlt_645_frame_data_t *)(dlt_645_frame_head + 1);
	dlt_645_frame_tail = (dlt_645_frame_tail_t *)(dlt_645_frame_data->data + out_len);

	crc = sum_crc8((uint8_t *)dlt_645_frame_head, (uint8_t *)&dlt_645_frame_tail->crc - (uint8_t *)dlt_645_frame_head);

	if(dlt_645_frame_tail->crc != crc) {
		debug("dlt_645_frame_tail->crc:%02x, crc:%02x", dlt_645_frame_tail->crc, crc);
		return ret;
	}

	u_dlt_645_control.s.frame_type = 1;

	if(u_dlt_645_control.v != dlt_645_frame_head->control.v) {
		debug("");
		return ret;
	}

	data = (uint8_t *)dlt_645_frame_data;

	for(i = 0; i < dlt_645_frame_head->len; i++) {
		data[i] = data[i] - 0x33;
	}

	*out = dlt_645_frame_data->data;

	ret = 0;

	return ret;
}

int dlt_645_master_get_energy_get_addr(dlt_645_master_info_t *dlt_645_master_info, dlt_645_addr_t *addr)
{
	int ret = -1;
	dlt_645_addr_t id_addr;
	dlt_645_addr_t *out_addr;
	memset(id_addr.data, 0xaa, sizeof(id_addr.data));

	if(dlt_645_master_request(dlt_645_master_info, &id_addr, 0x13, NULL, 0, (uint8_t **)&out_addr, sizeof(dlt_645_addr_t)) == 0) {
		*addr = *out_addr;
		ret = 0;
	}

	return ret;
}

#pragma pack(push, 1)

typedef struct {
	uint8_t flag;
	uint8_t chennel;//0:总 0x01-0x3f:费率1-63 0xff:最大
	uint8_t type;
	uint8_t domain;
} data_type_t;

typedef union {
	data_type_t s;
	uint32_t v;
} u_data_type_t;

typedef struct {
	u_data_type_t data_type;
	u_uint32_bytes_t energy;
} energy_data_block_t;

#pragma pack(pop)

int dlt_645_master_get_energy(dlt_645_master_info_t *dlt_645_master_info, dlt_645_addr_t *addr, uint32_t *energy)
{
	int ret = -1;

	energy_data_block_t *data = NULL;
	u_data_type_t data_type;

	data_type.v = 0x00010000;

	if(dlt_645_master_request(dlt_645_master_info, addr, 0x11, (uint8_t *)&data_type, sizeof(data_type), (uint8_t **)&data, sizeof(energy_data_block_t)) == 0) {
		if(data_type.v == data->data_type.v) {
			*energy = data->energy.v;
			ret = 0;
		}
	}

	return ret;
}

#pragma pack(push, 1)

typedef struct {
	u_data_type_t data_type;
	u_uint16_bytes_t va;
	u_uint16_bytes_t vb;
	u_uint16_bytes_t vc;
} voltage_data_block_t;

#pragma pack(pop)

int dlt_645_master_get_voltage(dlt_645_master_info_t *dlt_645_master_info, dlt_645_addr_t *addr, uint16_t *va, uint16_t *vb, uint16_t *vc)
{
	int ret = -1;

	voltage_data_block_t *data = NULL;
	u_data_type_t data_type;

	data_type.v = 0x0201ff00;

	if(dlt_645_master_request(dlt_645_master_info, addr, 0x11, (uint8_t *)&data_type, sizeof(data_type), (uint8_t **)&data, sizeof(voltage_data_block_t)) == 0) {
		if(data_type.v == data->data_type.v) {
			*va = data->va.v;
			*vb = data->vb.v;
			*vc = data->vc.v;
			ret = 0;
		}
	}

	return ret;
}

#pragma pack(push, 1)

typedef struct {
	u_data_type_t data_type;
	uint8_t ca_b0;
	uint8_t ca_b1;
	uint8_t ca_b2;
	uint8_t cb_b0;
	uint8_t cb_b1;
	uint8_t cb_b2;
	uint8_t cc_b0;
	uint8_t cc_b1;
	uint8_t cc_b2;
} current_data_block_t;

#pragma pack(pop)

int dlt_645_master_get_current(dlt_645_master_info_t *dlt_645_master_info, dlt_645_addr_t *addr, uint16_t *ca, uint16_t *cb, uint16_t *cc)
{
	int ret = -1;

	current_data_block_t *data = NULL;
	u_data_type_t data_type;

	data_type.v = 0x0202ff00;

	if(dlt_645_master_request(dlt_645_master_info, addr, 0x11, (uint8_t *)&data_type, sizeof(data_type), (uint8_t **)&data, sizeof(current_data_block_t)) == 0) {
		if(data_type.v == data->data_type.v) {
			*ca = get_u32_from_bcd_b0123(
			          data->ca_b0,
			          data->ca_b1,
			          data->ca_b2,
			          0);

			*cb = get_u32_from_bcd_b0123(
			          data->cb_b0,
			          data->cb_b1,
			          data->cb_b2,
			          0);

			*cc = get_u32_from_bcd_b0123(
			          data->cc_b0,
			          data->cc_b1,
			          data->cc_b2,
			          0);
			ret = 0;
		}
	}

	return ret;
}
