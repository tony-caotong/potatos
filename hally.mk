
SRC_ROOT = $(shell pwd)
HALLY_ROOT = $(SRC_ROOT)/sdk/hally_root/
CFLAGS = -I $(HALLY_ROOT)dapcomm/lch/util/ \
	-I $(HALLY_ROOT)dapprotocol/psdecode/decoder/ \
	-I $(HALLY_ROOT)dapcomm/lch/aplog/ \
	-I $(HALLY_ROOT)dapcomm/lch/mm/ \
	-I $(HALLY_ROOT)dapprotocol/psdecode/base/ \
	-I $(HALLY_ROOT)dapprotocol/psdecode/protocol/ \

BUILD_ROOT = $(SRC_ROOT)/build/hally/

all:
	g++ $(CFLAGS) -g -c $(SRC_ROOT)/decoder.cpp
	ar rc libhally.a decoder.o
	ranlib libhally.a
clean:
	rm -f libhally.a decoder.o
