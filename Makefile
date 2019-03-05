obj-m += mace.o
mace-objs := src/module.o src/ring_buffer.o src/dev_file.o

release := $(shell uname -r)

all:
	make -C /lib/modules/$(release)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(release)/build M=$(PWD) clean
