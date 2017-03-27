/**
 *	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-03-27
 *
 */

#include <stdint.h>

#include "debug.h"

static void debug_print_binary(char* capital, char* buf, size_t length)
{
	int i = 0;
	
	printf("Capital: [%s]\n", capital);
	while(i < length) {
		if (i%32 == 0)
			printf("0x%016lx --     ", (unsigned long)&buf[i]);
		printf("%02X", (unsigned char)buf[i]);
		i++;
		if (i%32 == 0 && i != length)
			printf("\n");
		else if (i%8 == 0)
			printf(" ");
	}
	printf("\n");
}

void debug_print_mbuf_infos(struct rte_mbuf* buf)
{
	char* p = buf->buf_addr + buf->data_off;
	int l = buf->pkt_len;
	char captial[32];

	printf("mbuf->packet_type: %08x\n", buf->packet_type);
	printf("mbuf->l2_type: %01x\n", buf->l2_type);
	printf("mbuf->l3_type: %01x\n", buf->l3_type);
	printf("mbuf->l4_type: %01x\n", buf->l4_type);
	printf("mbuf->tun_type: %01x\n", buf->tun_type);
	printf("mbuf->inner_l2_type: %01x\n", buf->inner_l2_type);
	printf("mbuf->inner_l3_type: %01x\n", buf->inner_l3_type);
	printf("mbuf->inner_l4_type: %01x\n", buf->inner_l4_type);
	printf("mbuf->ol_flags: %016lx\n", buf->ol_flags);

	snprintf(captial, sizeof(captial), "packet length: %d", l);
	if (l > 1024) {
		printf("%s\n", captial);
	}
	debug_print_binary(captial, p, l);
}

