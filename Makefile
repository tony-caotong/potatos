include $(RTE_SDK)/mk/rte.vars.mk

HALLY_ROOT = $(RTE_SRCDIR)/sdk/hally_root/
APP = potatos
SRCS-y := main.c
CFLAGS += -Wall
CFLAGS += -g
LDFLAGS += -lstdc++
LDLIBS += -L$(HALLY_ROOT)/dapcomm/ -ldapcomm
LDLIBS += -rpath=$(HALLY_ROOT)/dapcomm/
LDLIBS += -L$(HALLY_ROOT)/dapprotocol/ -ldapprotocol
LDLIBS += -rpath=$(HALLY_ROOT)/dapprotocol/
LDLIBS += -L$(RTE_SRCDIR) -lhally
V += y

#LIBHALLY=libhally.a
#
#$(APP): $(LIBHALLY)
#
#.PHONY: $(LIBHALLY)
#$(LIBHALLY):
#	make -f hally.mk
#
#clean:
#	make -f hally.mk clean

include $(RTE_SDK)/mk/rte.extapp.mk
