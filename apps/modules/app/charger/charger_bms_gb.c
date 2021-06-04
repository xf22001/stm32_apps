

/*================================================================
 *   
 *   
 *   文件名称：charger_bms_gb.c
 *   创 建 者：肖飞
 *   创建日期：2021年04月10日 星期六 17时01分30秒
 *   修改日期：2021年06月04日 星期五 17时40分32秒
 *   描    述：
 *
 *================================================================*/
#include "charger_bms_gb.h"
#include "charger_bms.h"

#include "log.h"

static int prepare_bms_state_idle(void *_charger_info)
{
	int ret = 0;
	//charger_info_t *charger_info = (charger_info_t *)_charger_info;

	return ret;
}

static int handle_bms_state_idle_request(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_idle_response(void *_charger_info)
{
	int ret = 0;
	return ret;
}

static int prepare_bms_state_chm(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_chm_request(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_chm_response(void *_charger_info)
{
	int ret = -1;

	return ret;
}

static int prepare_bms_state_crm(void *_charger_info)
{
	int ret = 0;
	return ret;
}

static int handle_bms_state_crm_request(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_crm_response(void *_charger_info)
{
	int ret = -1;

	return ret;
}

static int prepare_bms_state_cts_cml(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_cts_cml_request(void *_charger_info)
{
	int ret = 0;
	return ret;
}

static int handle_bms_state_cts_cml_response(void *_charger_info)
{
	int ret = -1;

	return ret;
}

static int prepare_bms_state_cro(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_cro_request(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_cro_response(void *_charger_info)
{
	int ret = -1;

	return ret;
}

static int prepare_bms_state_ccs(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_ccs_request(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_ccs_response(void *_charger_info)
{
	int ret = -1;

	return ret;
}

static int prepare_bms_state_cst(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_cst_request(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_cst_response(void *_charger_info)
{
	int ret = -1;

	return ret;
}

static int prepare_bms_state_csd_cem(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_csd_cem_request(void *_charger_info)
{
	int ret = 0;

	return ret;
}

static int handle_bms_state_csd_cem_response(void *_charger_info)
{
	int ret = -1;

	return ret;
}

static charger_bms_state_handler_t state_handler_sz[] = {
	{
		.bms_state = CHARGER_BMS_STATE_IDLE,
		.prepare = prepare_bms_state_idle,
		.handle_request = handle_bms_state_idle_request,
		.handle_response = handle_bms_state_idle_response,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CHM,
		.prepare = prepare_bms_state_chm,
		.handle_request = handle_bms_state_chm_request,
		.handle_response = handle_bms_state_chm_response,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CRM,
		.prepare = prepare_bms_state_crm,
		.handle_request = handle_bms_state_crm_request,
		.handle_response = handle_bms_state_crm_response,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CTS_CML,
		.prepare = prepare_bms_state_cts_cml,
		.handle_request = handle_bms_state_cts_cml_request,
		.handle_response = handle_bms_state_cts_cml_response,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CRO,
		.prepare = prepare_bms_state_cro,
		.handle_request = handle_bms_state_cro_request,
		.handle_response = handle_bms_state_cro_response,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CCS,
		.prepare = prepare_bms_state_ccs,
		.handle_request = handle_bms_state_ccs_request,
		.handle_response = handle_bms_state_ccs_response,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CST,
		.prepare = prepare_bms_state_cst,
		.handle_request = handle_bms_state_cst_request,
		.handle_response = handle_bms_state_cst_response,
	},
	{
		.bms_state = CHARGER_BMS_STATE_CSD_CEM,
		.prepare = prepare_bms_state_csd_cem,
		.handle_request = handle_bms_state_csd_cem_request,
		.handle_response = handle_bms_state_csd_cem_response,
	},
};

static charger_bms_state_handler_t *get_charger_bms_state_handler(uint8_t bms_state)
{
	int i;
	charger_bms_state_handler_t *charger_bms_state_handler = NULL;

	for(i = 0; i < ARRAY_SIZE(state_handler_sz); i++) {
		charger_bms_state_handler_t *charger_bms_state_handler_item = &state_handler_sz[i];

		if(charger_bms_state_handler_item->bms_state == bms_state) {
			charger_bms_state_handler = charger_bms_state_handler_item;
		}
	}

	return charger_bms_state_handler;
}

static void set_charger_bms_work_state(charger_info_t *charger_info)
{
	switch(charger_info->state) {
		case CHARGER_BMS_STATE_IDLE: {
			charger_info->charger_bms_work_state = CHARGER_BMS_WORK_STATE_IDLE;
		}
		break;

		case CHARGER_BMS_STATE_CHM: {
			charger_info->charger_bms_work_state = CHARGER_BMS_WORK_STATE_STARTING;
		}
		break;

		case CHARGER_BMS_STATE_CRM: {
			charger_info->charger_bms_work_state = CHARGER_BMS_WORK_STATE_STARTING;
		}
		break;

		case CHARGER_BMS_STATE_CTS_CML: {
			charger_info->charger_bms_work_state = CHARGER_BMS_WORK_STATE_STARTING;
		}
		break;

		case CHARGER_BMS_STATE_CRO: {
			charger_info->charger_bms_work_state = CHARGER_BMS_WORK_STATE_STARTING;
		}
		break;

		case CHARGER_BMS_STATE_CCS: {
			charger_info->charger_bms_work_state = CHARGER_BMS_WORK_STATE_RUNNING;
		}
		break;

		case CHARGER_BMS_STATE_CST: {
			charger_info->charger_bms_work_state = CHARGER_BMS_WORK_STATE_STOPPING;
		}
		break;

		case CHARGER_BMS_STATE_CSD_CEM: {
			charger_info->charger_bms_work_state = CHARGER_BMS_WORK_STATE_STOPPING;
		}
		break;

		default: {
		}
		break;
	}
}

static char *get_charger_bms_state_des(uint8_t state)
{
	char *des = NULL;

	switch(state) {
			add_des_case(CHARGER_BMS_STATE_IDLE);
			add_des_case(CHARGER_BMS_STATE_CHM);
			add_des_case(CHARGER_BMS_STATE_CRM);
			add_des_case(CHARGER_BMS_STATE_CTS_CML);
			add_des_case(CHARGER_BMS_STATE_CRO);
			add_des_case(CHARGER_BMS_STATE_CCS);
			add_des_case(CHARGER_BMS_STATE_CST);
			add_des_case(CHARGER_BMS_STATE_CSD_CEM);

		default: {
			des = "unknow state";
		}
		break;
	}

	return des;
}

static void update_charger_bms_state(charger_info_t *charger_info)
{
	charger_bms_state_handler_t *charger_bms_state_handler = NULL;
	uint8_t request_state = charger_info->request_state;

	if((charger_info->state == request_state) && (charger_info->charger_bms_state_handler != NULL)) {
		return;
	}

	charger_bms_state_handler = get_charger_bms_state_handler(request_state);
	OS_ASSERT(charger_bms_state_handler != NULL);

	debug("change state: %s -> %s!", get_charger_bms_state_des(charger_info->state), get_charger_bms_state_des(request_state));

	charger_info->state = request_state;
	set_charger_bms_work_state(charger_info);

	if(charger_bms_state_handler->prepare != NULL) {
		charger_bms_state_handler->prepare(charger_info);
	}

	charger_info->charger_bms_state_handler = charger_bms_state_handler;
}

static int handle_init(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;

	charger_info->charger_bms_state_handler = NULL;
	set_charger_bms_request_state(charger_info, CHARGER_BMS_STATE_IDLE);
	update_charger_bms_state(charger_info);

	return ret;
}

static int handle_request(void *_charger_info)
{
	int ret = 0;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;

	update_charger_bms_state(charger_info);

	if(charger_info->charger_bms_state_handler == NULL) {
		debug("");
		return ret;
	}

	ret = charger_info->charger_bms_state_handler->handle_request(charger_info);

	return ret;
}

static int handle_response(void *_charger_info)
{
	int ret = -1;
	charger_info_t *charger_info = (charger_info_t *)_charger_info;

	if(charger_info->charger_bms_state_handler == NULL) {
		debug("");
		return ret;
	}

	ret = charger_info->charger_bms_state_handler->handle_response(charger_info);

	return ret;
}


charger_bms_handler_t charger_bms_handler_gb = {
	.channel_charger_type = CHANNEL_CHARGER_TYPE_BMS_GB,
	.handle_init = handle_init,
	.handle_request = handle_request,
	.handle_response = handle_response,
};
