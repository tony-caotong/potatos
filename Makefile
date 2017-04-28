# '?=' conditional. '=' recursively. '::=' or ':=' simply expanded.

RTE_SDK ?= $(shell pwd)/sdk/dpdk_root/
RTE_TARGET ?= x86_64-native-linuxapp-gcc

export RTE_SDK
export RTE_TARGET

include $(RTE_SDK)/mk/rte.vars.mk

APP=potatos
DECODER_DIR = $(SRCDIR)/decoder
FLOWER_DIR = $(SRCDIR)/flower
UTILS_DIR = $(SRCDIR)/utils

SRCS-y := $(patsubst %.c, %.o, $(wildcard $(SRCDIR)\/*.c))
SRCS-y += $(patsubst %.c, %.o, $(wildcard $(DECODER_DIR)\/*.c))
SRCS-y += $(patsubst %.c, %.o, $(wildcard $(FLOWER_DIR)\/*.c))
SRCS-y += $(patsubst %.c, %.o, $(wildcard $(UTILS_DIR)\/*.c))
CFLAGS += -I$(SRCDIR)/
CFLAGS += -I$(DECODER_DIR)/
CFLAGS += -I$(FLOWER_DIR)/
CFLAGS += -I$(UTILS_DIR)/

CFLAGS += -Wall
CFLAGS += -O0 -g

include $(RTE_SDK)/mk/rte.extapp.mk

.PHONY: potatosclean
potatosclean:
	@find $(SRCDIR) -name '.*.o.cmd' -delete
	@find $(SRCDIR) -name '.*.o.d' -delete
	@find $(SRCDIR) -name '*.o' -delete

clean: potatosclean

.PHONY: tags
tags:
	@ctags -R --c-kinds=+p ./
