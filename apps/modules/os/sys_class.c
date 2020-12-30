

/*================================================================
 *
 *
 *   文件名称：sys_class.c
 *   创 建 者：肖飞
 *   创建日期：2020年12月29日 星期二 09时52分23秒
 *   修改日期：2020年12月30日 星期三 08时34分35秒
 *   描    述：
 *
 *================================================================*/
#include "sys_class.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "os_utils.h"

static sys_class_t *sys_class = NULL;

int sys_class_init(void)
{
	int ret = -1;
	osMutexDef(mutex);

	if(sys_class != NULL) {
		return ret;
	}

	sys_class = os_alloc(sizeof(sys_class_t));

	if(sys_class == NULL) {
		return ret;
	}

	memset(sys_class, 0, sizeof(sys_class_t));
	INIT_LIST_HEAD(&sys_class->list);

	sys_class->mutex = osMutexCreate(osMutex(mutex));

	if(sys_class->mutex == NULL) {
		goto failed;
	}

	return ret;

failed:

	if(sys_class != NULL) {
		os_free(sys_class);
		sys_class = NULL;
	}

	return ret;
}

void sys_classs_uninit(void)
{
	osStatus os_status;
	LIST_HEAD(uninit_list);

	if(sys_class == NULL) {
		return;
	}

	os_status = osMutexWait(sys_class->mutex, osWaitForever);

	if(os_status != osOK) {
	}

	if(!list_empty(&sys_class->list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &sys_class->list) {
			list_move_tail(pos, &uninit_list);
		}
	}

	os_status = osMutexRelease(sys_class->mutex);

	if(os_status != osOK) {
	}

	if(!list_empty(&uninit_list)) {
		struct list_head *pos;
		struct list_head *n;

		list_for_each_safe(pos, n, &sys_class->list) {
			sys_class_info_t *info = list_entry(pos, sys_class_info_t, list);

			list_del(pos);

			if(info->uninit != NULL) {
				info->uninit(info->ctx);
			}

			os_free(info);
		}
	}

	os_status = osMutexDelete(sys_class->mutex);

	if(os_status != osOK) {
	}

	os_free(sys_class);
	sys_class = NULL;
}

sys_class_info_t *sys_class_info_alloc(void *ctx, const char *fmt, ...)
{
	va_list ap;
	int ret;
	sys_class_info_t *sys_class_info = (sys_class_info_t *)os_alloc(sizeof(sys_class_info_t));

	if(sys_class_info == NULL) {
		return sys_class_info;
	}

	memset(sys_class_info, 0, sizeof(sys_class_info_t));

	va_start(ap, fmt);

	ret = vsnprintf(sys_class_info->name, SYS_CLASS_NAME_SIZE, fmt, ap);

	if(ret < 0) {
	}

	va_end(ap);

	sys_class_info->hash = str_hash(sys_class_info->name);
	sys_class_info->ctx = ctx;

	return sys_class_info;
}

sys_class_info_t *sys_class_info_find(const char *fmt, ...)
{
	sys_class_info_t *info = NULL;
	va_list ap;
	char *buffer = os_alloc(SYS_CLASS_NAME_SIZE);
	uint32_t hash;
	osStatus os_status;
	int ret;

	if(buffer == NULL) {
		return info;
	}

	va_start(ap, fmt);

	ret = vsnprintf(buffer, SYS_CLASS_NAME_SIZE, fmt, ap);

	if(ret < 0) {
	}

	va_end(ap);

	hash = str_hash(buffer);

	os_status = osMutexWait(sys_class->mutex, osWaitForever);

	if(os_status != osOK) {
	}

	if(!list_empty(&sys_class->list)) {
		sys_class_info_t *pos;

		list_for_each_entry(pos, &sys_class->list, sys_class_info_t, list) {
			if(pos->hash != hash) {
				continue;
			}

			if(strcmp(pos->name, buffer) == 0) {
				info = pos;
				break;
			}
		}
	}

	os_status = osMutexRelease(sys_class->mutex);

	if(os_status != osOK) {
	}

	os_free(buffer);

	return info;
}

int sys_class_info_register(sys_class_info_t *info)
{
	int ret = -1;
	osStatus os_status;
	uint8_t found = 0;

	if(info == NULL) {
		return ret;
	}

	if(sys_class == NULL) {
		return ret;
	}

	os_status = osMutexWait(sys_class->mutex, osWaitForever);

	if(os_status != osOK) {
	}

	if(!list_empty(&sys_class->list)) {
		sys_class_info_t *pos;

		list_for_each_entry(pos, &sys_class->list, sys_class_info_t, list) {
			if(pos->hash != info->hash) {
				continue;
			}

			if(strcmp(pos->name, info->name) == 0) {
				found = 1;
				break;
			}
		}
	}

	if(found == 0) {
		list_add_tail(&info->list, &sys_class->list);
		ret = 0;
	}

	os_status = osMutexRelease(sys_class->mutex);

	if(os_status != osOK) {
	}

	if(ret == 0) {
		if(info->init != NULL) {
			info->init(info->ctx);
		}
	}

	return ret;
}

int sys_class_info_unregister(sys_class_info_t *info)
{
	int ret = -1;
	osStatus os_status;

	if(info == NULL) {
		return ret;
	}

	if(sys_class == NULL) {
		return ret;
	}

	os_status = osMutexWait(sys_class->mutex, osWaitForever);

	if(os_status != osOK) {
	}

	if(!list_empty(&sys_class->list)) {
		sys_class_info_t *pos;

		list_for_each_entry(pos, &sys_class->list, sys_class_info_t, list) {
			if(pos->hash != info->hash) {
				continue;
			}

			if(strcmp(pos->name, info->name) == 0) {
				ret = 0;
				list_del(&pos->list);
				break;
			}
		}
	}

	os_status = osMutexRelease(sys_class->mutex);

	if(os_status != osOK) {
	}

	if(ret == 0) {
		if(info->uninit != NULL) {
			info->uninit(info->ctx);
		}
	}

	return ret;
}
