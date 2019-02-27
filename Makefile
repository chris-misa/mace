obj-m += mace.o
mace-objs := src/module.o src/sysfs.o src/ring_buffer.o

release := $(shell uname -r)

all:
	make -C /lib/modules/$(release)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(release)/build M=$(PWD) clean
