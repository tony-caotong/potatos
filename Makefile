include $(RTE_SDK)/mk/rte.vars.mk
V += y

DAP_ROOT = $(RTE_SRCDIR)/sdk/dap_root/

APP = hally
LIBDAP = libdap.a

SRCS-y := main.c
LIBDAP_O = decoder.o

CFLAGS += -Wall
CFLAGS += -g

CXXFLAGS += -I $(DAP_ROOT)dapcomm/lch/util/
CXXFLAGS += -I $(DAP_ROOT)dapcomm/lch/aplog/
CXXFLAGS += -I $(DAP_ROOT)dapcomm/lch/mm/
CXXFLAGS += -I $(DAP_ROOT)dapprotocol/psdecode/base/
CXXFLAGS += -I $(DAP_ROOT)dapprotocol/psdecode/protocol/
CXXFLAGS += -I $(DAP_ROOT)dapprotocol/psdecode/decoder/

LDFLAGS += -lstdc++
LDLIBS += -L$(DAP_ROOT)/dapcomm/ -ldapcomm
LDLIBS += -rpath=$(DAP_ROOT)/dapcomm/
LDLIBS += -L$(DAP_ROOT)/dapprotocol/ -ldapprotocol
LDLIBS += -rpath=$(DAP_ROOT)/dapprotocol/
LDLIBS += -L$(RTE_SRCDIR) -ldap


$(APP): $(LIBDAP)
clean: dapclean

# This include Must be in this line before any custom rules.
include $(RTE_SDK)/mk/rte.extapp.mk

$(LIBDAP_O): %.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $(CFLAGS) $< -o $@

$(LIBDAP): $(LIBDAP_O)
	ar rc $@ $?
	ranlib $@

.PHONY: dapclean
dapclean:
	rm -f $(LIBDAP) $(LIBDAP_O)
