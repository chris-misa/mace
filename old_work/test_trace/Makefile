obj-m += trace_test.o

release := $(shell uname -r)

all:
	make -C /lib/modules/$(release)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(release)/build M=$(PWD) clean
