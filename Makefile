include $(RTE_SDK)/mk/rte.vars.mk
APP=potatos
SRCS-y := main.o
SRCS-y += decoder.o
SRCS-y += hw_features.o
CFLAGS += -Wall
CFLAGS += -g
V += y

#.PHONY: test
#test:
#	echo $(SRCS-y)

include $(RTE_SDK)/mk/rte.extapp.mk
