

/*================================================================
 *
 *
 *   文件名称：channel_record_handler.c
 *   创 建 者：肖飞
 *   创建日期：2021年06月19日 星期六 12时12分16秒
 *   修改日期：2021年06月19日 星期六 17时38分26秒
 *   描    述：
 *
 *================================================================*/
#include "channel_record_handler.h"
#include "channel_record.h"

#include "log.h"

typedef struct {
	channel_record_task_info_t *channel_record_task_info;
	callback_item_t channel_record_sync_callback_item;
	callback_item_t start_callback_item;
	callback_item_t stop_callback_item;
} channel_record_handler_ctx_t;

static void channel_record_sync_data(void *_channel_info, void *_channel_record_task_info)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channel_record_task_info_t *channel_record_task_info = (channel_record_task_info_t *)_channel_record_task_info;

	channel_record_update(channel_record_task_info, &channel_info->channel_record_item);
}

static void start(void *_channel_info, void *__channel_info)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channel_record_handler_ctx_t *channel_record_handler_ctx = channel_info->channel_record_handler_ctx;

	OS_ASSERT(alloc_channel_record_item_id(channel_record_handler_ctx->channel_record_task_info, &channel_info->channel_record_item) == 0);

	channel_info->channel_record_item.state = CHANNEL_RECORD_ITEM_STATE_UPDATE;

	channel_record_handler_ctx->channel_record_sync_callback_item.fn = channel_record_sync_data;
	channel_record_handler_ctx->channel_record_sync_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_record_handler_ctx->channel_record_task_info->channel_record_sync_chain, &channel_record_handler_ctx->channel_record_sync_callback_item) == 0);

	channel_record_sync(channel_record_handler_ctx->channel_record_task_info);
	debug("");
}

static void stop(void *_channel_info, void *__channel_info)
{
	channel_info_t *channel_info = (channel_info_t *)_channel_info;
	channel_record_handler_ctx_t *channel_record_handler_ctx = channel_info->channel_record_handler_ctx;

	OS_ASSERT(remove_callback(channel_record_handler_ctx->channel_record_task_info->channel_record_sync_chain, &channel_record_handler_ctx->channel_record_sync_callback_item) == 0);
	channel_info->channel_record_item.state = CHANNEL_RECORD_ITEM_STATE_FINISH;
	channel_record_sync_data(channel_info, channel_record_handler_ctx);
	debug("");
}

int channel_record_handler_init(channel_info_t *channel_info)
{
	channel_record_handler_ctx_t *channel_record_handler_ctx = os_calloc(1, sizeof(channel_record_handler_ctx_t));

	OS_ASSERT(channel_record_handler_ctx != NULL);

	channel_record_handler_ctx->channel_record_task_info = get_or_alloc_channel_record_task_info(0);
	OS_ASSERT(channel_record_handler_ctx->channel_record_task_info != NULL);

	channel_record_handler_ctx->start_callback_item.fn = start;
	channel_record_handler_ctx->start_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->start_chain, &channel_record_handler_ctx->start_callback_item) == 0);

	channel_record_handler_ctx->stop_callback_item.fn = stop;
	channel_record_handler_ctx->stop_callback_item.fn_ctx = channel_info;
	OS_ASSERT(register_callback(channel_info->stop_chain, &channel_record_handler_ctx->stop_callback_item) == 0);

	channel_info->channel_record_handler_ctx = channel_record_handler_ctx;

	return 0;
}
