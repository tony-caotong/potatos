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

LIBHALLY= -lhally

$(APP): $(LIBHALLY)
clean: hallyclean

include $(RTE_SDK)/mk/rte.extapp.mk

$(LIBHALLY):
	make  -C $(RTE_SRCDIR) -f hally.mk

.PHONY: hallyclean
hallyclean:
	make -f $(RTE_SRCDIR)/hally.mk clean

