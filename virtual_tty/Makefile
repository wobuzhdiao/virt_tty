KERNEL_SOURCE=/lib/modules/$(shell uname -r)/build
CURR_DIR=$(shell pwd)
OBJ_NAME=virtual_tty_controller
TARGET=$(OBJ_NAME).ko

obj-m += $(OBJ_NAME).o

all:
	$(MAKE) -C $(KERNEL_SOURCE)  M=$(CURR_DIR) modules
clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURR_DIR) clean

install:
	$(HOST_INSTALL) $(TARGET) $(IMAGES_DIR)

.PHONY:all clean install
