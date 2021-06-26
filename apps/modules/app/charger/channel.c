

/*================================================================
 *
 *
 *   文件名称：channel.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月08日 星期四 09时51分12秒
 *   修改日期：2021年06月26日 星期六 15时51分11秒
 *   描    述：
 *
 *================================================================*/
#include "channel.h"

#include "channel_handler_dc.h"
#include "channel_handler_ac.h"
#include "channel_handler_proxy.h"
#include "channel_record_handler.h"
#include "charger.h"
#include "energy_meter.h"
#include "hw_adc.h"
#include "ntc_temperature.h"

#include "log.h"

static channel_handler_t *channel_handler_sz[] = {
	&channel_handler_dc,
	&channel_handler_ac,
	&channel_handler_proxy,
};

static channel_handler_t *get_channel_handler(channel_type_t channel_type)
{
	int i;
	channel_handler_t *channel_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(channel_handler_sz); i++) {
		channel_handler_t *channel_handler_item = channel_handler_sz[i];

		if(channel_handler_item->channel_type == channel_type) {
			channel_handler = channel_handler_item;
		}
	}

	return channel_handler;
}

char *get_channel_state_des(channel_state_t state)
{
	char *des = "unknow";

	switch(state) {
			add_des_case(CHANNEL_STATE_NONE);
			add_des_case(CHANNEL_STATE_IDLE);
			add_des_case(CHANNEL_STATE_START);
			add_des_case(CHANNEL_STATE_STARTING);
			add_des_case(CHANNEL_STATE_CHARGING);
			add_des_case(CHANNEL_STATE_STOP);
			add_des_case(CHANNEL_STATE_STOPPING);
			add_des_case(CHANNEL_STATE_END);

		default: {
		}
		break;
	}

	return des;
}

int set_channel_request_state(channel_info_t *channel_info, channel_state_t state)
{
	int ret = -1;

	if(channel_info->request_state == CHANNEL_STATE_NONE) {
		ret = 0;
	}

	debug("request_state %s -> %s", get_channel_state_des(channel_info->request_state), get_channel_state_des(state));
	channel_info->request_state = state;

	return ret;
}

void channel_set_stop_reason(channel_info_t *channel_info, channel_record_item_stop_reason_t stop_reason)
{
	switch(channel_info->state) {
		case CHANNEL_STATE_NONE:
		case CHANNEL_STATE_IDLE: {
			return;
		}
		break;

		default: {
		}
		break;
	}

	if(channel_info->channel_record_item.stop_reason == CHANNEL_RECORD_ITEM_STOP_REASON_NONE) {
		channel_info->channel_record_item.stop_reason = stop_reason;
	}
}

static void handle_channel_request_state(channel_info_t *channel_info)
{
	if(channel_info->request_state == CHANNEL_STATE_NONE) {
		return;
	}

	if(channel_info->state != channel_info->request_state) {
		do_callback_chain(channel_info->state_changed_chain, channel_info);
	}

	channel_info->request_state = CHANNEL_STATE_NONE;
}

static void handle_channel_state(channel_info_t *channel_info)
{
	switch(channel_info->state) {
		case CHANNEL_STATE_IDLE: {
			do_callback_chain(channel_info->idle_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_START: {
			do_callback_chain(channel_info->start_chain, channel_info);
			set_channel_request_state(channel_info, CHANNEL_STATE_STARTING);
		}
		break;

		case CHANNEL_STATE_STARTING: {
			do_callback_chain(channel_info->starting_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_CHARGING: {
			do_callback_chain(channel_info->charging_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_STOP: {
			do_callback_chain(channel_info->stop_chain, channel_info);
			set_channel_request_state(channel_info, CHANNEL_STATE_STOPPING);
		}
		break;

		case CHANNEL_STATE_STOPPING: {
			do_callback_chain(channel_info->stopping_chain, channel_info);
		}
		break;

		case CHANNEL_STATE_END: {
			do_callback_chain(channel_info->end_chain, channel_info);
			set_channel_request_state(channel_info, CHANNEL_STATE_IDLE);
		}
		break;

		default: {
		}
		break;
	}

	handle_channel_request_state(channel_info);
}

time_t get_ts_by_seg_index(uint8_t seg_index)
{
	time_t ts = 0;
	OS_ASSERT((86400 % PRICE_SEGMENT_SIZE) == 0);
	ts = seg_index * 86400 / PRICE_SEGMENT_SIZE;
	ts = ts % 86400;

	return ts;
}

uint8_t get_seg_index_by_ts(time_t ts)
{
	uint8_t seg_index;

	OS_ASSERT((86400 % PRICE_SEGMENT_SIZE) == 0);
	seg_index = (ts % 86400) / (86400 / PRICE_SEGMENT_SIZE);
	OS_ASSERT(seg_index < PRICE_SEGMENT_SIZE);

	return seg_index;
}

static uint32_t get_current_price(channels_info_t *channels_info, time_t ts)
{
	price_info_t *price_info = &channels_info->channels_settings.price_info;
	uint8_t price_index;

	price_index = price_info->seg[get_seg_index_by_ts(ts)];
	OS_ASSERT(price_index < PRICE_ARRAY_SIZE);

	return price_info->price[price_index];
}

void handle_channel_amount(channel_info_t *channel_info)
{
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;
	time_t ts = get_time();
	uint32_t price;
	uint32_t delta_energy;

	if(channel_info->state != CHANNEL_STATE_CHARGING) {
		return;
	}

	price = get_current_price(channels_info, ts);
	delta_energy = channel_info->total_energy - channel_info->channel_record_item.start_total_energy;

	//todo clear
	channel_info->channel_record_item.energy_seg[get_seg_index_by_ts(ts)] += delta_energy;
	channel_info->channel_record_item.energy += delta_energy;
	channel_info->channel_record_item.amount += delta_energy * price;

	channel_info->channel_record_item.start_total_energy = channel_info->total_energy;
}

static void handle_channel_stop_amount(channel_info_t *channel_info)
{
	if(channel_info->channel_record_item.amount >= channel_info->channel_record_item.account_balance) {
		channel_set_stop_reason(channel_info, CHANNEL_RECORD_ITEM_STOP_REASON_AMOUNT);
		set_channel_request_state(channel_info, CHANNEL_STATE_STOP);
	}
}

static void handle_channel_stop(channel_info_t *channel_info)
{
	switch(channel_info->state) {
		case CHANNEL_STATE_START:
		case CHANNEL_STATE_STARTING:
		case CHANNEL_STATE_CHARGING: {
			handle_channel_stop_amount(channel_info);
		}
		break;

		default: {
		}
		break;
	}
}

static void handle_channel_periodic(void *_channel_info, void *chain_ctx)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;

	//debug("channel_info %d periodic!", channel_info->channel_id);

	handle_channel_amount(channel_info);
	handle_channel_stop(channel_info);
	handle_channel_state(channel_info);
}

static int _handle_channel_event(channel_info_t *channel_info, channel_event_t *channel_event)
{
	int ret = -1;

	debug("channel_id %d process handler event %s!", channel_info->channel_id, get_channel_event_type_des(channel_event->type));

	switch(channel_event->type) {
		case CHANNEL_EVENT_TYPE_START_CHANNEL: {
			switch(channel_info->state) {
				case CHANNEL_STATE_IDLE: {
					set_channel_request_state(channel_info, CHANNEL_STATE_START);
					ret = 0;
				}
				break;

				default: {
				}
				break;
			}
		}
		break;

		case CHANNEL_EVENT_TYPE_STOP_CHANNEL: {
			switch(channel_info->state) {
				case CHANNEL_STATE_START:
				case CHANNEL_STATE_STARTING:
				case CHANNEL_STATE_CHARGING: {
					channel_set_stop_reason(channel_info, channel_event->reason);
					set_channel_request_state(channel_info, CHANNEL_STATE_STOP);
					ret = 0;
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

	return ret;
}

static void handle_channel_event(void *_channel_info, void *_channels_event)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;
	channel_event_t *channel_event;
	uint8_t match = 0;

	if(channels_event->type != CHANNELS_EVENT_CHANNEL_EVENT) {
		return;
	}

	channel_event = &channels_event->event.channel_event;

	if(channel_event->channel_id == 0xff) { //broadcast
		match = 1;
	}

	if(channel_event->channel_id == channel_info->channel_id) {
		match = 1;
	}

	if(match == 0) {
		return;
	}

	_handle_channel_event(channel_info, channel_event);
}

static int channel_init(channel_info_t *channel_info)
{
	int ret = 0;
	channel_config_t *channel_config = channel_info->channel_config;
	channels_info_t *channels_info = (channels_info_t *)channel_info->channels_info;

	channel_info->faults = alloc_bitmap(CHANNEL_FAULT_SIZE);
	OS_ASSERT(channel_info->faults != NULL);

	channel_info->request_state = CHANNEL_STATE_NONE;
	channel_info->state = CHANNEL_STATE_IDLE;

	channel_info->idle_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->idle_chain != NULL);
	channel_info->start_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->start_chain != NULL);
	channel_info->starting_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->starting_chain != NULL);
	channel_info->charging_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->charging_chain != NULL);
	channel_info->stop_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->stop_chain != NULL);
	channel_info->stopping_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->stopping_chain != NULL);
	channel_info->end_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->end_chain != NULL);
	channel_info->state_changed_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->state_changed_chain != NULL);
	channel_info->charger_connect_changed_chain = alloc_callback_chain();
	OS_ASSERT(channel_info->charger_connect_changed_chain != NULL);


	debug("channel %d init channel record %s", channel_info->channel_id, get_channel_config_charger_type(channel_config->charger_config.channel_charger_type));
	channel_record_handler_init(channel_info);

	debug("channel %d init charger %s", channel_info->channel_id, get_channel_config_channel_type(channel_config->channel_type));
	channel_info->channel_handler = get_channel_handler(channel_config->channel_type);

	if((channel_info->channel_handler != NULL) && (channel_info->channel_handler->init != NULL)) {
		OS_ASSERT(channel_info->channel_handler->init(channel_info) == 0);
	}

	channel_info->periodic_callback_item.fn = handle_channel_periodic;
	channel_info->periodic_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channel_info->periodic_callback_item) == 0);

	channel_info->event_callback_item.fn = handle_channel_event;
	channel_info->event_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channel_info->event_callback_item) == 0);

	debug("channel %d alloc charger %s", channel_info->channel_id, get_channel_config_charger_type(channel_config->charger_config.channel_charger_type));
	channel_info->charger_info = alloc_charger_info(channel_info);
	debug("channel %d alloc energy_meter %s", channel_info->channel_id, get_channel_config_energy_meter_type(channel_config->energy_meter_config.energy_meter_type));
	channel_info->energy_meter_info = alloc_energy_meter_info(channel_info);

	return ret;
}

static void handle_channels_temperature(channels_info_t *channels_info)
{
	adc_info_t *adc_info;
	uint16_t temperature_ad;

	if(channels_info->channels_config->board_temperature_adc == NULL) {
		return;
	}

	adc_info = get_or_alloc_adc_info(channels_info->channels_config->board_temperature_adc);
	OS_ASSERT(adc_info != NULL);

	temperature_ad = get_adc_value(adc_info, channels_info->channels_config->board_temperature_adc_rank);
	channels_info->temperature = get_ntc_temperature(10000, temperature_ad, 4095);
	//debug("current temperature:%d", channels_info->temperature);
}

static void handle_channels_force_stop(channels_info_t *channels_info)
{
	uint8_t force_stop = 0;

	if(channels_info->channels_config->force_stop_port == NULL) {
		return;
	}

	if (HAL_GPIO_ReadPin(channels_info->channels_config->force_stop_port, channels_info->channels_config->force_stop_pin) == GPIO_PIN_SET) {
		force_stop = 1;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_FORCE_STOP) != force_stop) {
		set_fault(channels_info->faults, CHANNELS_FAULT_FORCE_STOP, force_stop);
	}
}

static int handle_channel_electric_leakage_detect_b(channels_info_t *channels_info)
{
	int ret = 0;
	uint32_t ticks = osKernelSysTick();

	switch(channels_info->electric_leakage_detect_b_state) {
		case ELECTRIC_LEAKAGE_DETECT_B_STATE_CAL_PREPARE: {
			OS_ASSERT(channels_info->channels_config->electric_leakage_detect_cal_port != NULL);
			OS_ASSERT(channels_info->channels_config->electric_leakage_detect_test_port != NULL);
			OS_ASSERT(channels_info->channels_config->electric_leakage_detect_trip_port != NULL);
			HAL_GPIO_WritePin(channels_info->channels_config->electric_leakage_detect_cal_port, channels_info->channels_config->electric_leakage_detect_cal_pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(channels_info->channels_config->electric_leakage_detect_test_port, channels_info->channels_config->electric_leakage_detect_test_pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(channels_info->channels_config->electric_leakage_detect_trip_port, channels_info->channels_config->electric_leakage_detect_trip_pin, GPIO_PIN_RESET);
			channels_info->electric_leakage_detect_b_stamps = ticks;
			channels_info->electric_leakage_detect_b_state = ELECTRIC_LEAKAGE_DETECT_B_STATE_CAL_START;
		}
		break;

		case ELECTRIC_LEAKAGE_DETECT_B_STATE_CAL_START: {
			if(ticks_duration(ticks, channels_info->electric_leakage_detect_b_stamps) >= 100) {
				OS_ASSERT(channels_info->channels_config->electric_leakage_detect_cal_port != NULL);
				HAL_GPIO_WritePin(channels_info->channels_config->electric_leakage_detect_cal_port, channels_info->channels_config->electric_leakage_detect_cal_pin, GPIO_PIN_RESET);
				channels_info->electric_leakage_detect_b_stamps = ticks;
				channels_info->electric_leakage_detect_b_state = ELECTRIC_LEAKAGE_DETECT_B_STATE_TEST_PREPARE;
			}
		}
		break;

		case ELECTRIC_LEAKAGE_DETECT_B_STATE_TEST_PREPARE: {
			if(ticks_duration(ticks, channels_info->electric_leakage_detect_b_stamps) >= 75) {
				OS_ASSERT(channels_info->channels_config->electric_leakage_detect_cal_port != NULL);
				HAL_GPIO_WritePin(channels_info->channels_config->electric_leakage_detect_cal_port, channels_info->channels_config->electric_leakage_detect_cal_pin, GPIO_PIN_SET);
				channels_info->electric_leakage_detect_b_stamps = ticks;
				channels_info->electric_leakage_detect_b_state = ELECTRIC_LEAKAGE_DETECT_B_STATE_TEST_START;
			}
		}
		break;

		case ELECTRIC_LEAKAGE_DETECT_B_STATE_TEST_START: {
			if(ticks_duration(ticks, channels_info->electric_leakage_detect_b_stamps) >= 500) {
				OS_ASSERT(channels_info->channels_config->electric_leakage_detect_test_port != NULL);
				HAL_GPIO_WritePin(channels_info->channels_config->electric_leakage_detect_test_port, channels_info->channels_config->electric_leakage_detect_test_pin, GPIO_PIN_SET);
				channels_info->electric_leakage_detect_b_stamps = ticks;
				channels_info->electric_leakage_detect_b_state = ELECTRIC_LEAKAGE_DETECT_B_STATE_WAIT_TRIP;
			}
		}
		break;

		case ELECTRIC_LEAKAGE_DETECT_B_STATE_WAIT_TRIP: {
			if(ticks_duration(ticks, channels_info->electric_leakage_detect_b_stamps) >= 400) {
				OS_ASSERT(channels_info->channels_config->electric_leakage_detect_test_port != NULL);
				HAL_GPIO_WritePin(channels_info->channels_config->electric_leakage_detect_test_port, channels_info->channels_config->electric_leakage_detect_test_pin, GPIO_PIN_RESET);
				channels_info->electric_leakage_detect_b_state = ELECTRIC_LEAKAGE_DETECT_B_STATE_ERROR;
			} else {
				if(channels_info->channels_config->electric_leakage_detect_trip_port != NULL) {
					if(HAL_GPIO_ReadPin(channels_info->channels_config->electric_leakage_detect_trip_port, channels_info->channels_config->electric_leakage_detect_trip_pin) == GPIO_PIN_SET) {
						channels_info->electric_leakage_detect_b_stamps = ticks;
						channels_info->electric_leakage_detect_b_state = ELECTRIC_LEAKAGE_DETECT_B_STATE_PREPARE_DETECT;
					}
				}
			}
		}
		break;

		case ELECTRIC_LEAKAGE_DETECT_B_STATE_PREPARE_DETECT: {
			if(ticks_duration(ticks, channels_info->electric_leakage_detect_b_stamps) >= 200) {
				channels_info->electric_leakage_detect_b_state = ELECTRIC_LEAKAGE_DETECT_B_STATE_DETECT;
			}
		}
		break;

		case ELECTRIC_LEAKAGE_DETECT_B_STATE_DETECT: {
			if(HAL_GPIO_ReadPin(channels_info->channels_config->electric_leakage_detect_trip_port, channels_info->channels_config->electric_leakage_detect_trip_pin) == GPIO_PIN_SET) {
				ret = -1;
			}
		}
		break;

		case ELECTRIC_LEAKAGE_DETECT_B_STATE_ERROR: {
			ret = -2;
		}
		break;

		default: {
		}
		break;
	}

	return ret;
}

static void handle_channels_electric_leakage_detect(channels_info_t *channels_info)
{
	uint8_t electric_leakage_trip = 0;
	uint8_t electric_leakage_cal = 0;

	switch(channels_info->electric_leakage_detect_type) {
		case ELECTRIC_LEAKAGE_DETECT_TYPE_B: {
			int ret = handle_channel_electric_leakage_detect_b(channels_info);

			if(ret == -1) {
				electric_leakage_trip = 1;
			} else if(ret == -2) {
				electric_leakage_cal = 1;
			}
		}
		break;

		default: {
			if(channels_info->channels_config->electric_leakage_detect_trip_port != NULL) {
				if(HAL_GPIO_ReadPin(channels_info->channels_config->electric_leakage_detect_trip_port, channels_info->channels_config->electric_leakage_detect_trip_pin) == GPIO_PIN_SET) {
					electric_leakage_trip = 1;
				}
			}

		}
		break;
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_ELECTRIC_LEAK_CALIBRATION) != electric_leakage_cal) {
		set_fault(channels_info->faults, CHANNELS_FAULT_ELECTRIC_LEAK_CALIBRATION, electric_leakage_cal);
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_ELECTRIC_LEAK_PROTECT) != electric_leakage_trip) {
		set_fault(channels_info->faults, CHANNELS_FAULT_ELECTRIC_LEAK_PROTECT, electric_leakage_trip);
	}
}

static void handle_channels_pe_detect(channels_info_t *channels_info)
{
	uint8_t pd_detect = 0;

	if(channels_info->channels_config->pe_detect_port != NULL) {
		if(HAL_GPIO_ReadPin(channels_info->channels_config->pe_detect_port, channels_info->channels_config->pe_detect_pin) == GPIO_PIN_SET) {
			pd_detect = 1;
		}
	}

	if(get_fault(channels_info->faults, CHANNELS_FAULT_PE_PROTECT) != pd_detect) {
		set_fault(channels_info->faults, CHANNELS_FAULT_PE_PROTECT, pd_detect);
	}
}

static void handle_channels_common_periodic(void *_channels_info, void *__channels_info)
{
	channels_info_t *channels_info = (channels_info_t *)_channels_info;

	//debug("channels periodic!");
	handle_channels_temperature(channels_info);
	handle_channels_force_stop(channels_info);
	handle_channels_electric_leakage_detect(channels_info);
	handle_channels_pe_detect(channels_info);
}

static void handle_channels_common_event(void *_channels_info, void *_channels_event)
{
	//channels_info_t *channels_info = (channels_info_t *)_channels_info;
	channels_event_t *channels_event = (channels_event_t *)_channels_event;

	debug("channels_info common process event %s!", get_channel_event_type_des(channels_event->type));

	switch(channels_event->type) {
		default: {
		}
		break;
	}
}

channel_info_t *alloc_channels_channel_info(channels_info_t *channels_info)
{
	channels_config_t *channels_config = channels_info->channels_config;
	channel_info_t *channel_info = NULL;
	int i;

	channels_info->channel_number = channels_config->channel_number;
	channel_info = (channel_info_t *)os_calloc(channels_info->channel_number, sizeof(channel_info_t));
	OS_ASSERT(channel_info != NULL);

	for(i = 0; i < channels_info->channel_number; i++) {
		channel_info_t *channel_info_item = channel_info + i;

		channel_info_item->channels_info = channels_info;
		channel_info_item->channel_config = channels_config->channel_config[i];
		channel_info_item->channel_id = i;

		channel_init(channel_info_item);
	}

	channels_info->periodic_callback_item.fn = handle_channels_common_periodic;
	channels_info->periodic_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_periodic_chain, &channels_info->periodic_callback_item) == 0);

	channels_info->event_callback_item.fn = handle_channels_common_event;
	channels_info->event_callback_item.fn_ctx = channels_info;
	OS_ASSERT(register_callback(channels_info->common_event_chain, &channels_info->event_callback_item) == 0);

	return channel_info;
}
