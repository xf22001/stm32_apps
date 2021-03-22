

/*================================================================
 *
 *
 *   文件名称：test_soft_timer.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月22日 星期五 13时09分20秒
 *   修改日期：2021年01月28日 星期四 17时19分59秒
 *   描    述：
 *
 *================================================================*/
#include "test_soft_timer.h"
#include "soft_timer.h"

#include "log.h"
extern RNG_HandleTypeDef hrng;

static void fn(void *fn_ctx, void *chain_ctx)
{
	char *des = (char *)fn_ctx;

	debug("%s run in %p", des, chain_ctx);
}

typedef struct {
	char *des;
	soft_timer_ctx_t *ctx;
} fn1_ctx_t;

static void fn1(void *fn_ctx, void *chain_ctx)
{
	fn1_ctx_t *fn1_ctx = (fn1_ctx_t *)fn_ctx;
	soft_timer_ctx_t *soft_timer_ctx = fn1_ctx->ctx;
	soft_timer_info_t *soft_timer_info = fn1_ctx->ctx->soft_timer_info;
	int ret;
	uint32_t period;

	debug("%s run in %p", fn1_ctx->des, chain_ctx);

	debug("remove %s", fn1_ctx->des);
	ret = remove_soft_timer(soft_timer_ctx);

	if(ret != 0) {
		debug("remove %s error", fn1_ctx->des);
		return;
	}

	period = HAL_RNG_GetRandomNumber(&hrng) % 1000;

	debug("period:%d", period);

	fn1_ctx->ctx = add_soft_timer(soft_timer_info,
	                              fn1,
	                              fn1_ctx,
	                              period, SOFT_TIMER_FN_TYPE_ONESHOT);

	if(fn1_ctx->ctx == NULL) {
		debug("create %s failed", fn1_ctx->des);
		return;
	}

	debug("start %s", fn1_ctx->des);
	ret = start_soft_timer(fn1_ctx->ctx);

	if(ret != 0) {
		debug("");
		debug("start %s error", fn1_ctx->des);
	}
}

fn1_ctx_t fn1_ctx;

void test_soft_timer(void)
{
	int ret;
	int i;
	soft_timer_info_t *soft_timer_info_0 = get_or_alloc_soft_timer_info(0);
	soft_timer_info_t *soft_timer_info_1 = get_or_alloc_soft_timer_info(1);

	soft_timer_ctx_t *ctx_0_timer_0 = add_soft_timer(
	                                      soft_timer_info_0,
	                                      fn,
	                                      "ctx_0_timer_0",
	                                      1000, SOFT_TIMER_FN_TYPE_ONESHOT);

	soft_timer_ctx_t *ctx_1_timer_0 = add_soft_timer(
	                                      soft_timer_info_0,
	                                      fn,
	                                      "ctx_1_timer_0",
	                                      250, SOFT_TIMER_FN_TYPE_REPEAT);

	soft_timer_ctx_t *ctx_2_timer_1 = add_soft_timer(
	                                      soft_timer_info_1,
	                                      fn,
	                                      "ctx_2_timer_1",
	                                      1000, SOFT_TIMER_FN_TYPE_REPEAT);

	soft_timer_ctx_t *ctx_3_timer_1 = add_soft_timer(
	                                      soft_timer_info_1,
	                                      fn1,
	                                      &fn1_ctx,
	                                      1000, SOFT_TIMER_FN_TYPE_ONESHOT);

	if(ctx_0_timer_0 == NULL) {
		debug("");
		return;
	}

	if(ctx_1_timer_0 == NULL) {
		debug("");
		return;
	}

	if(ctx_2_timer_1 == NULL) {
		debug("");
		return;
	}

	if(ctx_3_timer_1 == NULL) {
		debug("");
		return;
	}

	fn1_ctx.des = "ctx_3_timer_1";
	fn1_ctx.ctx = ctx_3_timer_1;

	debug("start ctx_0_timer_0");
	ret = start_soft_timer(ctx_0_timer_0);

	if(ret != 0) {
		debug("start ctx_0_timer_0 error");
	}


	debug("start ctx_1_timer_0");
	ret = start_soft_timer(ctx_1_timer_0);

	if(ret != 0) {
		debug("start ctx_1_timer_0 error");
	}


	debug("start ctx_2_timer_1");
	ret = start_soft_timer(ctx_2_timer_1);

	if(ret != 0) {
		debug("start ctx_2_timer_1 error");
	}


	debug("start ctx_3_timer_1");
	ret = start_soft_timer(ctx_3_timer_1);

	if(ret != 0) {
		debug("start ctx_3_timer_1 error");
	}


	osDelay(5000);

	debug("stop ctx_0_timer_0");
	ret = stop_soft_timer(ctx_0_timer_0);

	if(ret != 0) {
		debug("stop ctx_0_timer_0 error");
	}


	debug("stop ctx_1_timer_0");
	ret = stop_soft_timer(ctx_1_timer_0);

	if(ret != 0) {
		debug("stop ctx_1_timer_0 error");
	}


	debug("stop ctx_2_timer_1");
	ret = stop_soft_timer(ctx_2_timer_1);

	if(ret != 0) {
		debug("stop ctx_2_timer_1 error");
	}


	debug("remove ctx_0_timer_0");
	ret = remove_soft_timer(ctx_0_timer_0);

	if(ret != 0) {
		debug("remove ctx_0_timer_0 error");
	}


	debug("remove ctx_1_timer_0");
	ret = remove_soft_timer(ctx_1_timer_0);

	if(ret != 0) {
		debug("remove ctx_1_timer_0 error");
	}


	debug("remove ctx_2_timer_1");
	ret = remove_soft_timer(ctx_2_timer_1);

	if(ret != 0) {
		debug("remove ctx_2_timer_1 error");
	}

	for(i = 0; i < 10; i++) {
		ctx_1_timer_0 = add_soft_timer(
		                    soft_timer_info_0,
		                    fn,
		                    "ctx_1_timer_0",
		                    50, SOFT_TIMER_FN_TYPE_REPEAT);

		if(ctx_1_timer_0 == NULL) {
			debug("");
			return;
		}


		debug("start ctx_1_timer_0");
		ret = start_soft_timer(ctx_1_timer_0);

		if(ret != 0) {
			debug("start ctx_1_timer_0 error");
		}

		osDelay(500);

		debug("stop ctx_1_timer_0");
		ret = stop_soft_timer(ctx_1_timer_0);

		if(ret != 0) {
			debug("stop ctx_1_timer_0 error");
		}

		debug("remove ctx_1_timer_0");
		ret = remove_soft_timer(ctx_1_timer_0);

		if(ret != 0) {
			debug("remove ctx_1_timer_0 error");
		}
	}
}
