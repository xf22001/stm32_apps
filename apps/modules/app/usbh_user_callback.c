

/*================================================================
 *
 *
 *   文件名称：usbh_user_callback.c
 *   创 建 者：肖飞
 *   创建日期：2021年03月25日 星期四 14时15分22秒
 *   修改日期：2021年03月25日 星期四 14时19分51秒
 *   描    述：
 *
 *================================================================*/
#include "usbh_user_callback.h"

#include "mt_file.h"
#include "file_log.h"
#include "vfs.h"

void usbh_user_callback(USBH_HandleTypeDef *phost, uint8_t id)
{
	int ret;

	switch(id) {
		case HOST_USER_CONNECTION: {
			mt_file_environment_init();
		}
		break;

		case HOST_USER_DISCONNECTION: {
			try_to_close_log();
			ret = mt_f_mount(0, "", 0);

			if(ret != FR_OK) {
			}
		}
		break;

		case HOST_USER_CLASS_ACTIVE: {
			ret = mt_f_mount(get_vfs_fs(), "", 0);

			if(ret != FR_OK) {
			}
		}
		break;

		default:
			break;
	}
}
