CONFIG_MODULE_SIG = n
TARGET_MODULE := revdrv

obj-m := $(TARGET_MODULE).o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: write
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) write
load:
	sudo insmod $(TARGET_MODULE).ko
unload:
	sudo rmmod $(TARGET_MODULE) || true >/dev/null

write: write.c
	$(CC) -o $@ $^

check: all
	$(MAKE) unload
	$(MAKE) load
	@echo "===example 1==="
	sudo ./write "9527" -O TRUNC /dev/reverse
	sudo cat /dev/reverse && echo
	@echo "===example 2==="
	sudo ./write "this is a test" -O TRUNC /dev/reverse
	sudo cat /dev/reverse && echo
	@echo "===example 3==="
	echo -n "123456789" | sudo tee /dev/reverse
	@echo ""
	echo -n " abcdefg" | sudo tee -a /dev/reverse
	@echo ""
	sudo cat /dev/reverse && echo
	sudo ./write "xyz" -s 8 /dev/reverse
	sudo cat /dev/reverse && echo
	sudo ./write "wxyz" -s -9 -S END /dev/reverse
	sudo cat /dev/reverse && echo
	$(MAKE) unload
