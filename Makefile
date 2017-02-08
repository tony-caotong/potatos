include $(RTE_SDK)/mk/rte.vars.mk
APP=potato
SRCS-y := main.c
CFLAGS += -g
include $(RTE_SDK)/mk/rte.extapp.mk
