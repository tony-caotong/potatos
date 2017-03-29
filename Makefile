include $(RTE_SDK)/mk/rte.vars.mk
APP=potatos
SRCS-y := $(patsubst %.c, %.o, $(wildcard $(SRCDIR)\/*.c))
CFLAGS += -Wall
CFLAGS += -O0 -g

include $(RTE_SDK)/mk/rte.extapp.mk

.PHONY: potatosclean
potatosclean:
	@rm -f $(SRCDIR)/.*.o.cmd
	@rm -f $(SRCDIR)/.*.o.d
	@rm -f $(SRCDIR)/*.o

clean: potatosclean
