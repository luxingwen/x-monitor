ifeq ($(ARCH),arm)
  # *UPDATE* 'KDIR' below to point to the ARM Linux kernel source tree on your box
  KDIR ?= ~/rpi_work/kernel_rpi/linux
else ifeq ($(ARCH),arm64)
  # *UPDATE* 'KDIR' below to point to the ARM64 (Aarch64) Linux kernel source
  # tree on your box
  KDIR ?= ~/kernel/linux-4.14
else ifeq ($(ARCH),powerpc)
  # *UPDATE* 'KDIR' below to point to the PPC64 Linux kernel source tree on your box
  KDIR ?= ~/kernel/linux-4.9.1
else
  # 'KDIR' is the Linux 'kernel headers' package on your host system; this is
  # usually an x86_64, but could be anything, really (f.e. building directly
  # on a Raspberry Pi implies that it's the host)
  KDIR ?= /lib/modules/$(shell uname -r)/build
endif

PWD := $(shell pwd)
obj-m += calmwu_mmap_dev.o
MY_CFLAGS += -g -DDEBUG -Werror -Wno-sign-compare
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}

all:
	@echo
	@echo '--- Building : KDIR=${KDIR} ARCH=${ARCH} MY_CFLAGS=${MY_CFLAGS} ---'
	@echo
	make -C $(KDIR) M=$(PWD) modules

install:
	@echo
	@echo "--- installing ---"
	@echo
	make -C $(KDIR) M=$(PWD) modules_install

clean:
	@echo
	@echo "--- cleaning ---"
	@echo
	make -C $(KDIR) M=$(PWD) clean
	rm -f *~   # from 'indent'