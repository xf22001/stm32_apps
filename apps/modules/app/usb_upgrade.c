

/*================================================================
 *
 *
 *   文件名称：usb_upgrade.c
 *   创 建 者：肖飞
 *   创建日期：2021年05月08日 星期六 20时15分37秒
 *   修改日期：2021年05月10日 星期一 11时17分58秒
 *   描    述：
 *
 *================================================================*/
#include "usb_upgrade.h"
#include "mt_file.h"
#include "flash.h"
#include "iap.h"
#include "app.h"

#include "log.h"

typedef enum {
	USB_UPGRADE_STATE_IDLE = 0,
	USB_UPGRADE_STATE_CHECK_FIRMWARE,
	USB_UPGRADE_STATE_FLUSH_FIRMWARE,
} usb_upgrade_state_t;

static usb_upgrade_state_t state = USB_UPGRADE_STATE_IDLE;

void start_usb_upgrade(void)
{
	if(state == USB_UPGRADE_STATE_IDLE) {
		if(is_app() == 1) {
			app_info_t *app_info = get_app_info();

			if(app_info->mechine.upgrade_enable != 0) {
				state = USB_UPGRADE_STATE_CHECK_FIRMWARE;
			} else {
				debug("");
			}
		} else {
			state = USB_UPGRADE_STATE_CHECK_FIRMWARE;
		}
	}
}

typedef enum {
	FW_TYPE_NONE = 0,
	FW_TYPE_BOOTLOADER,
	FW_TYPE_APP,
} fw_type_t;

#pragma pack(push, 1)

typedef struct {
	uint32_t crc;
	uint32_t id;
	uint8_t fw_type;
} fw_header_t;

#pragma pack(pop)

static int check_firmware(void)
{
	int ret = -1;
	FIL file;
	FRESULT r;
	fw_header_t fw_header;
	uint32_t crc = 0;
	UINT read;
	uint8_t buffer[16];
	FILINFO file_info;
	uint32_t count = 0;
	//0x411 f207
	//0x413 f407
	uint32_t id = DBGMCU->IDCODE & DBGMCU_IDCODE_DEV_ID;

	r = mt_f_stat("/fw.bin", &file_info)					/* Get file status */;

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	r = mt_f_open(&file, "/fw.bin", FA_READ);

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	if(file_info.fsize < sizeof(fw_header)) {
		debug("");
		goto exit;
	}

	r = mt_f_read(&file, &fw_header, sizeof(fw_header), &read);

	if(r != FR_OK) {
		debug("");
		goto close;
	}

	if(read != sizeof(fw_header)) {
		debug("");
		goto close;
	}

	if(fw_header.id != id) {
		debug("");
		goto close;
	}

	if(fw_header.fw_type != FW_TYPE_APP) {
		debug("");
		goto close;
	}

	count += sizeof(fw_header);

	while(count < file_info.fsize) {
		r = mt_f_read(&file, buffer, 16, &read);

		if(r != FR_OK) {
			debug("");
			goto close;
		}

		count += read;
		crc += sum_crc32(buffer, read);
	}

	if(crc == fw_header.crc) {
		debug("ok!");
		ret = 0;
	}

close:
	mt_f_close(&file);
exit:
	return ret;
}

static int flush_firmware(void)
{
	int ret = -1;
	FIL file;
	FRESULT r;
	UINT read;
	fw_header_t fw_header;
	uint32_t crc = 0;
	uint8_t buffer[16];
	FILINFO file_info;
	uint32_t count = 0;
	uint32_t offset = 0;

	//擦除第6和7扇
	if(flash_erase_sector(FLASH_SECTOR_6, 2) != 0) {
		debug("");
		goto exit;
	}

	r = mt_f_stat("/fw.bin", &file_info)					/* Get file status */;

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	r = mt_f_open(&file, "/fw.bin", FA_READ);

	if(r != FR_OK) {
		debug("");
		goto exit;
	}

	r = mt_f_read(&file, &fw_header, sizeof(fw_header), &read);

	if(r != FR_OK) {
		debug("");
		goto close;
	}

	if(read != sizeof(fw_header)) {
		debug("");
		goto close;
	}

	count += sizeof(fw_header);

	while(count < file_info.fsize) {
		r = mt_f_read(&file, buffer, 16, &read);

		if(r != FR_OK) {
			debug("");
			goto close;
		}

		if(flash_write(USER_FLASH_FIRST_PAGE_ADDRESS + offset, buffer, read) != 0) {
			debug("");
			goto close;
		}

		offset += read;
		count += read;
	}

	crc = sum_crc32((void *)USER_FLASH_FIRST_PAGE_ADDRESS, offset);

	if(crc == fw_header.crc) {
		uint8_t flag = 0x01;

		if(flash_write(APP_CONFIG_ADDRESS, &flag, 1) != 0) {
			debug("");
			goto close;
		}

		debug("ok!");
		ret = 0;
	}

close:
	mt_f_close(&file);
exit:
	return ret;
}

void handle_usb_upgrade(void)
{
	switch(state) {
		case USB_UPGRADE_STATE_CHECK_FIRMWARE: {
			int ret = check_firmware();

			if(ret == 0) {
				if(is_app() == 1) {
					uint8_t flag = 0x00;
					flash_write(APP_CONFIG_ADDRESS, &flag, 1);
					debug("reset for upgrade!\n");
					HAL_NVIC_SystemReset();
				} else {
					state = USB_UPGRADE_STATE_FLUSH_FIRMWARE;
				}
			} else {
				state = USB_UPGRADE_STATE_IDLE;
			}
		}
		break;

		case USB_UPGRADE_STATE_FLUSH_FIRMWARE: {
			int ret = flush_firmware();

			if(ret == 0) {
				app_info_t *app_info = get_app_info();
				app_info->mechine.upgrade_enable = 0;
				app_save_config();
				HAL_NVIC_SystemReset();
			}
		}
		break;

		default: {
		}
		break;
	}
}
