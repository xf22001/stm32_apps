# -*- coding: utf-8 -*-
#!/usr/bin/env python
#================================================================
#   
#   
#   文件名称：instance.py
#   创 建 者：肖飞
#   创建日期：2020年09月10日 星期四 14时20分13秒
#   修改日期：2020年09月10日 星期四 14时29分01秒
#   描    述：
#
#================================================================
import sys
import optparse

template = """

#include "cmsis_os.h"
#include "app_platform.h"
#include "list_utils.h"

typedef struct {
	struct list_head list;
	TYPE *KEY;
} NAME_info_t;

static LIST_HEAD(NAME_info_list);
static osMutexId NAME_info_list_mutex = NULL;

static NAME_info_t *get_NAME_info(TYPE *KEY)
{
	NAME_info_t *NAME_info = NULL;
	NAME_info_t *NAME_info_item = NULL;
	osStatus os_status;

	if(NAME_info_list_mutex == NULL) {
		return NAME_info;
	}

	os_status = osMutexWait(NAME_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_for_each_entry(NAME_info_item, &NAME_info_list, NAME_info_t, list) {
		if(NAME_info_item->KEY == KEY) {
			NAME_info = NAME_info_item;
			break;
		}
	}

	os_status = osMutexRelease(NAME_info_list_mutex);

	if(os_status != osOK) {
	}

	return NAME_info;
}

void free_NAME_info(NAME_info_t *NAME_info)
{
	osStatus os_status;

	if(NAME_info == NULL) {
		return;
	}

	if(NAME_info_list_mutex == NULL) {
		return;
	}

	os_status = osMutexWait(NAME_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_del(&NAME_info->list);

	os_status = osMutexRelease(NAME_info_list_mutex);

	if(os_status != osOK) {
	}

	os_free(NAME_info);
}

NAME_info_t *get_or_alloc_NAME_info(TYPE *KEY)
{
	NAME_info_t *NAME_info = NULL;
	osStatus os_status;

	NAME_info = get_NAME_info(KEY);

	if(NAME_info != NULL) {
		return NAME_info;
	}

	if(NAME_info_list_mutex == NULL) {
		osMutexDef(NAME_info_list_mutex);
		NAME_info_list_mutex = osMutexCreate(osMutex(NAME_info_list_mutex));

		if(NAME_info_list_mutex == NULL) {
			return NAME_info;
		}
	}

	NAME_info = (NAME_info_t *)os_alloc(sizeof(NAME_info_t));

	if(NAME_info == NULL) {
		return NAME_info;
	}

	NAME_info->KEY = KEY;

	os_status = osMutexWait(NAME_info_list_mutex, osWaitForever);

	if(os_status != osOK) {
	}

	list_add_tail(&NAME_info->list, &NAME_info_list);

	os_status = osMutexRelease(NAME_info_list_mutex);

	if(os_status != osOK) {
	}

	return NAME_info;
}
"""

def main(argv):
    options = optparse.OptionParser()
    options.add_option('-n', '--name', dest='name', help='name', default=None)
    options.add_option('-t', '--type', dest='type', help='type', default=None)
    options.add_option('-k', '--key', dest='key', help='key', default=None)
    opts, args = options.parse_args(argv)
    #print('opts:%s' %(opts))
    #print('args:%s' %(args))
    if len(args):
        options.print_help()
        return

    if not opts.name:
        options.print_help()
        return
    if not opts.type:
        options.print_help()
        return
    if not opts.key:
        options.print_help()
        return
    content = template
    content = content.replace('NAME', opts.name)
    content = content.replace('TYPE', opts.type)
    content = content.replace('KEY', opts.key)
    print(content)

if '__main__' == __name__:
    main(sys.argv[1:])
