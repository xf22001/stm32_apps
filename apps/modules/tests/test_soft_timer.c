

/*================================================================
 *
 *
 *   文件名称：test_soft_timer.c
 *   创 建 者：肖飞
 *   创建日期：2021年01月22日 星期五 13时09分20秒
 *   修改日期：2021年01月22日 星期五 14时34分09秒
 *   描    述：
 *
 *================================================================*/
#include "test_soft_timer.h"
#include "soft_timer.h"

#include "log.h"

static void fn(void *fn_ctx, void *chain_ctx)
{
	char *des = (char *)fn_ctx;

	debug("%s run in %p\n", des, chain_ctx);
}

void test_soft_timer(void)
{
	int ret;
	int i;
	soft_timer_info_t *soft_timer_info_0 = get_or_alloc_soft_timer_info(0);
	soft_timer_info_t *soft_timer_info_1 = get_or_alloc_soft_timer_info(1);


	soft_timer_ctx_t *ctx_0_timer_0 = register_soft_timer(
	                                      soft_timer_info_0,
	                                      fn,
	                                      "ctx_0_timer_0",
	                                      1000, SOFT_TIMER_FN_TYPE_ONESHOT);

	soft_timer_ctx_t *ctx_1_timer_0 = register_soft_timer(
	                                      soft_timer_info_0,
	                                      fn,
	                                      "ctx_1_timer_0",
	                                      250, SOFT_TIMER_FN_TYPE_REPEAT);

	soft_timer_ctx_t *ctx_2_timer_1 = register_soft_timer(
	                                      soft_timer_info_1,
	                                      fn,
	                                      "ctx_2_timer_1",
	                                      1000, SOFT_TIMER_FN_TYPE_REPEAT);

	soft_timer_ctx_t *ctx_3_timer_1 = register_soft_timer(
	                                      soft_timer_info_1,
	                                      fn,
	                                      "ctx_3_timer_1",
	                                      1000, SOFT_TIMER_FN_TYPE_REPEAT);

	if(ctx_0_timer_0 == NULL) {
		debug("\n");
		return;
	}

	if(ctx_1_timer_0 == NULL) {
		debug("\n");
		return;
	}

	if(ctx_2_timer_1 == NULL) {
		debug("\n");
		return;
	}

	if(ctx_3_timer_1 == NULL) {
		debug("\n");
		return;
	}

	ret = start_soft_timer(ctx_0_timer_0);

	if(ret != 0) {
		debug("\n");
	}

	debug("start ctx_0_timer_0\n");

	ret = start_soft_timer(ctx_1_timer_0);

	if(ret != 0) {
		debug("\n");
	}

	debug("start ctx_1_timer_0\n");

	ret = start_soft_timer(ctx_2_timer_1);

	if(ret != 0) {
		debug("\n");
	}

	debug("start ctx_2_timer_1\n");

	ret = start_soft_timer(ctx_3_timer_1);

	if(ret != 0) {
		debug("\n");
	}

	debug("start ctx_3_timer_1\n");

	osDelay(5000);

	ret = cancel_soft_timer(ctx_1_timer_0);

	if(ret != 0) {
		debug("\n");
	}

	debug("stop ctx_1_timer_0\n");

	ret = cancel_soft_timer(ctx_2_timer_1);

	if(ret != 0) {
		debug("\n");
	}

	debug("stop ctx_2_timer_1\n");

	ret = cancel_soft_timer(ctx_3_timer_1);

	if(ret != 0) {
		debug("\n");
	}

	debug("stop ctx_3_timer_1\n");

	for(i = 0; i < 5; i++) {
		osDelay(200);

		ctx_3_timer_1 = register_soft_timer(
		                    soft_timer_info_1,
		                    fn,
		                    "ctx_3_timer_1",
		                    20, SOFT_TIMER_FN_TYPE_REPEAT);
		ret = start_soft_timer(ctx_3_timer_1);

		if(ret != 0) {
			debug("\n");
		}

		debug("start ctx_3_timer_1\n");

		osDelay(200);

		if(ctx_3_timer_1 != NULL) {
			int ret = cancel_soft_timer(ctx_3_timer_1);

			if(ret != 0) {
				debug("\n");
			}

			debug("stop ctx_3_timer_1\n");
		} else {
			debug("\n");
		}
	}
}
