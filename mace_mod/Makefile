obj-m += mace.o
mace-objs := module.o sysfs.o ring_buffer.o


release := $(shell uname -r)

all:
	make -C /lib/modules/$(release)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(release)/build M=$(PWD) clean
