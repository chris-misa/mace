obj-m += mace.o
mace-objs := src/module.o src/ring_buffer.o src/dev_file.o src/namespace_set.o

CONFIG_DEBUG_INFO=y
DEBUG_KPROBE_EVENT=y

release := $(shell uname -r)

all:
	make -C /lib/modules/$(release)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(release)/build M=$(PWD) clean
