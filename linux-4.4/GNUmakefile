#CROSS_COMPILE	:= $(ARM_TOOLCHAIN_PREFIX)-
#ARCH		:= arm

INSTALL_PATH		:= $(MAKETOP)/boot
INSTALL_MOD_PATH	:= $(MAKETOP)
INSTALL_HDR_PATH	:= $(MAKETOP)/usr
INSTALL_MOD_STRIP	:="--strip-unneeded -R .GCC-command-line"

INSTALL_EXTRA_MOD_PATH	:= $(MAKETOP)/lib/modules/4.4.8/combined

INSTALL_EXTRA_MOD	:= drivers/combined/combined.ko

MKIMAGE			:= $(PLATFORM)/tools/bin/mkuboot.sh

ifeq ($(MAKETOP), )
maketop_err = $(error Please set MAKETOP)
$(call maketop_err)
endif

MFLAGS		+= O=$(KERNEL_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) INSTALL_PATH=$(INSTALL_PATH) INSTALL_MOD_PATH=$(INSTALL_MOD_PATH) INSTALL_MOD_STRIP=$(INSTALL_MOD_STRIP) INSTALL_HDR_PATH=$(INSTALL_HDR_PATH)
ifneq (, $(wildcard $(MKIMAGE)))
MFLAGS		+= MKIMAGE=$(MKIMAGE)
endif

.PHONY: _all
_all:

.config: arch/$(ARCH)/configs/defconfig
	$(MAKE) -f Makefile $(MFLAGS) defconfig

install: .config
	$(MAKE) -f Makefile $(MFLAGS)
#	$(MAKE) -f Makefile $(MFLAGS) install
	install -c -m 0755 -D build-$(ARCH)/arch/$(ARCH)/boot/Image $(INSTALL_PATH)/vmlinux
	$(MAKE) -f Makefile $(MFLAGS) modules
	$(MAKE) -f Makefile $(MFLAGS) modules_install
	mkdir -p $(INSTALL_EXTRA_MOD_PATH)
	for mod in $(INSTALL_EXTRA_MOD) ; do	\
		if [ -f build-$(ARCH)/$$mod ]; then cp -vp build-$(ARCH)/$$mod $(INSTALL_EXTRA_MOD_PATH); fi; \
	done
	if [ -d $(MAKETOP)/lib/modules/4.4.8/kernel/drivers/dtvdd64 ]; then \
		cp -var $(MAKETOP)/lib/modules/4.4.8/kernel/drivers/dtvdd64 $(MAKETOP)/lib/modules/4.4.8; \
	fi;
	# tentative
	if [ -f $(MAKETOP)/lib/modules/4.4.8/kernel/drivers/iosc/kernel/iosc.ko ]; then \
		cp -vp $(MAKETOP)/lib/modules/4.4.8/kernel/drivers/iosc/kernel/iosc.ko $(MAKETOP)/lib/modules/4.4.8/combined; \
	fi;

install32: .config
#	$(MAKE) -f Makefile $(MFLAGS)
#	$(MAKE) -f Makefile $(MFLAGS) uinstall
	$(MAKE) -f Makefile $(MFLAGS) headers_install
	install -c -m 0644 -D build-$(ARCH)/.config $(INSTALL_PATH)/kernelconfig
	scripts/headers_install_sni.sh

GNUmakefile: ;

%:: FORCE
	$(MAKE) -f Makefile $(MFLAGS) $@

FORCE:
