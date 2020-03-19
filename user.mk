#
#
#================================================================
#   
#   
#   文件名称：user.mk
#   创 建 者：肖飞
#   创建日期：2019年10月25日 星期五 13时04分38秒
#   修改日期：2020年03月13日 星期五 11时38分32秒
#   描    述：
#
#================================================================

C_INCLUDES += -Iapps
C_SOURCES += apps/app.c
C_SOURCES += apps/os_utils.c
C_SOURCES += apps/usart_txrx.c
C_SOURCES += apps/can_txrx.c
C_SOURCES += apps/spi_txrx.c
C_SOURCES += apps/modbus_txrx.c
C_SOURCES += apps/charger.c
C_SOURCES += apps/charger_handler.c
C_SOURCES += apps/bms.c
C_SOURCES += apps/bms_handler.c
C_SOURCES += apps/test_serial.c
C_SOURCES += apps/test_can.c
C_SOURCES += apps/test_gpio.c
C_SOURCES += apps/test_charger_bms.c
C_SOURCES += apps/test_modbus.c
C_SOURCES += apps/request.c
C_SOURCES += apps/task_probe_tool.c
C_SOURCES += apps/net_protocol_tcp.c
C_SOURCES += apps/net_protocol_udp.c
C_SOURCES += apps/net_protocol_ws.c
C_SOURCES += apps/net_client.c
C_SOURCES += apps/request_default.c
C_SOURCES += apps/request_ws.c
C_SOURCES += apps/net_callback.c
C_SOURCES += apps/eeprom.c
C_SOURCES += apps/flash.c
C_SOURCES += apps/iap.c
C_SOURCES += apps/channels.c
C_SOURCES += apps/event_helper.c
C_SOURCES += apps/bitmap_ops.c
C_SOURCES += apps/https.c
C_SOURCES += apps/test_https.c

BMS_VERSION_YEAR := $(shell date '+%-Y')
BMS_VERSION_MONTH := $(shell date '+%-m')
BMS_VERSION_DAY := $(shell date '+%-d')
BMS_VERSION_SERIAL := $(shell date '+%-H%M')

CFLAGS += -DBMS_VERSION_YEAR=$(BMS_VERSION_YEAR)
CFLAGS += -DBMS_VERSION_MONTH=$(BMS_VERSION_MONTH)
CFLAGS += -DBMS_VERSION_DAY=$(BMS_VERSION_DAY)
CFLAGS += -DBMS_VERSION_SERIAL=$(BMS_VERSION_SERIAL)

ifeq ("$(origin APP)", "command line")
CFLAGS += -DUSER_APP
LDSCRIPT = STM32F207VETx_FLASH_APP.ld
$(shell touch apps/iap.h)
$(info "build app!")
else
LDSCRIPT = STM32F207VETx_FLASH.ld
$(shell touch apps/iap.h)
$(info "build bootloader!")
endif

default: all

cscope: all
	rm cscope e_cs -rf
	mkdir -p cscope
	#$(silent)tags.sh prepare;
	$(silent)touch dep_files;
	$(silent)for f in $$(find . -type f -name "*.d" 2>/dev/null); do \
		for i in $$(cat "$$f" | sed 's/^.*: //g'); do \
			if test -f "$$i";then readlink -f "$$i" >>dep_files;fi; \
			if test "$${i:0:1}" = "/";then :;fi; \
		done; \
	done;
	$(silent)cat dep_files | sort | uniq | sed 's/^\(.*\)$$/\"\1\"/g' >> cscope/cscope.files;
	$(silent)cat dep_files | sort | uniq >> cscope/ctags.files;
	$(silent)rm dep_files
	$(silent)tags.sh cscope;
	$(silent)tags.sh tags;
	$(silent)tags.sh env;

clean: clean-cscope
clean-cscope:
	rm cscope e_cs -rf
