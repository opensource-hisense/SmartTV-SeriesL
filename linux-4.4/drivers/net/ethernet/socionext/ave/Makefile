#
# makefile for AVEther driver
#
# Copyright (C) 2015-, Socionext Corporation
#

obj-$(CONFIG_AVE) += ave.o

ave-objs := ave_sn.o ave_sn_platform.o

EXTRA_CFLAGS += -DCONFIG_AMLINUX
EXTRA_CFLAGS += -DCONFIG_ARM
EXTRA_CFLAGS += -DAVE_ENABLE_PF_IF

