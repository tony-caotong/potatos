# '?=' means: set it if not set.
RTE_SDK ?= $(shell pwd)/sdk/dpdk_root/
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

APP=potatos
DECODER_DIR = $(SRCDIR)/decoder

SRCS-y := $(patsubst %.c, %.o, $(wildcard $(SRCDIR)\/*.c))
SRCS-y += $(patsubst %.c, %.o, $(wildcard $(DECODER_DIR)\/*.c))
CFLAGS += -I$(SRCDIR)/
CFLAGS += -I$(DECODER_DIR)/
CFLAGS += -Wall
CFLAGS += -O0 -g

include $(RTE_SDK)/mk/rte.extapp.mk

.PHONY: potatosclean
potatosclean:
	@rm -f $(SRCDIR)/.*.o.cmd
	@rm -f $(SRCDIR)/.*.o.d
	@rm -f $(SRCDIR)/*.o
	@rm -f $(DECODER_DIR)/.*.o.cmd
	@rm -f $(DECODER_DIR)/.*.o.d
	@rm -f $(DECODER_DIR)/*.o

clean: potatosclean

.PHONY: tags
tags:
	@ctags -R --c-kinds=+p ./
