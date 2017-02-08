#! /bin/bash

AWESOMEPKT_ROOT=../../awesomepkt/
i=1
while [ $i -lt 255 ]
do 
len=$(( 30 + ( $i % 16 )))
sudo $AWESOMEPKT_ROOT/asmpkt -i tap-dpdk-1 \
        --ptype eth \
        --ptype ipv4 \
        --dst_mac 11:22:33:44:55:66 --src_mac 3e:d6:46:3c:5b:cc \
        --dst_ip 192.168.1.$i --src_ip 10.0.3.244 \
        --fix $len
	i=`expr $i + 1`
#	sleep 1
done


