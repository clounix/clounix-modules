EXTRA_CFLAGS += -I$(shell pwd)/inc/
EXTRA_CFLAGS += -DCLX_EN_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_COMPILER_SUPPORT_LONG_LONG
ifeq ($(shell uname -m),x86_64)
EXTRA_CFLAGS += -DCLX_EN_HOST_64_BIT_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_64BIT_ADDR
else
EXTRA_CFLAGS += -DCLX_EN_HOST_32_BIT_LITTLE_ENDIAN
endif

DRIVER_MODULE_NAME         := clx_nb
BUILD_OUTPUT_DIR    := $(shell pwd)/build

obj-m:=$(DRIVER_MODULE_NAME).o
DRIVER_OBJS_TOTAL          += ./src/clx_nb.o
$(DRIVER_MODULE_NAME)-objs := $(DRIVER_OBJS_TOTAL)

OS_PATH=/lib/modules/$(shell uname -r)/build/
all:
	test -d $(BUILD_OUTPUT_DIR) || mkdir $(BUILD_OUTPUT_DIR)
	test -d $(BUILD_OUTPUT_DIR)/src || mkdir $(BUILD_OUTPUT_DIR)/src
	touch $(BUILD_OUTPUT_DIR)/Makefile
	make -C $(OS_PATH) src=$(shell pwd)/ M=$(BUILD_OUTPUT_DIR)/ modules EXTRA_CFLAGS="$(EXTRA_CFLAGS)"

clean:
	make -C $(OS_PATH) M=$(BUILD_OUTPUT_DIR) clean

.PHONY: all clean