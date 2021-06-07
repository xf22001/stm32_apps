

/*================================================================
 *   
 *   
 *   文件名称：bms_multi_data.h
 *   创 建 者：肖飞
 *   创建日期：2020年04月09日 星期四 13时58分33秒
 *   修改日期：2021年06月07日 星期一 15时03分28秒
 *   描    述：
 *
 *================================================================*/
#ifndef _BMS_MULTI_DATA_H
#define _BMS_MULTI_DATA_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "can_txrx.h"
#include "bms_spec.h"

#ifdef __cplusplus
}
#endif

int handle_multi_data_response(can_info_t *can_info, multi_packets_info_t *multi_packets_info, bms_data_t *bms_data);
uint8_t is_bms_data_multi_received(can_info_t *can_info, multi_packets_info_t *multi_packets_info, bms_fn_t fn);
int send_multi_packets(can_info_t *can_info, multi_packets_info_t *multi_packets_info, bms_data_t *bms_data, uint8_t *data, uint16_t fn, uint16_t bytes, uint16_t packets, uint32_t period);

#endif //_BMS_MULTI_DATA_H
