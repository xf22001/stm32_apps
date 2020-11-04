

/*================================================================
 *   
 *   
 *   文件名称：file_log.h
 *   创 建 者：肖飞
 *   创建日期：2020年11月03日 星期二 13时55分49秒
 *   修改日期：2020年11月03日 星期二 16时19分22秒
 *   描    述：
 *
 *================================================================*/
#ifndef _FILE_LOG_H
#define _FILE_LOG_H
#ifdef __cplusplus
extern "C"
{
#endif

#include "app_platform.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include <time.h>

#include "mt_file.h"

#ifdef __cplusplus
}
#endif

time_t *get_time(void);
uint8_t is_log_file_out_of_date(void);
int open_log(void);
void close_log(void);
FIL *get_log_file(void);

#define file_log_printf(str, ...) mt_f_printf(get_log_file(), "[%s] " str, asctime(localtime(get_time())), ## __VA_ARGS__)

#endif //_FILE_LOG_H
