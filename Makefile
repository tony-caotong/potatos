include $(RTE_SDK)/mk/rte.vars.mk
APP=potato
SRCS-y := main.c
CFLAGS += -Wall
CFLAGS += -g
V += y
include $(RTE_SDK)/mk/rte.extapp.mk
