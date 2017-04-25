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
	char* p;
	int l;
	struct rte_mbuf* m = buf;
	char captial[32];

	printf("mbuf->packet_type: %08x\n", m->packet_type);
	printf("mbuf->l2_type: %01x\n", m->l2_type);
	printf("mbuf->l3_type: %01x\n", m->l3_type);
	printf("mbuf->l4_type: %01x\n", m->l4_type);
	printf("mbuf->tun_type: %01x\n", m->tun_type);
	printf("mbuf->inner_l2_type: %01x\n", m->inner_l2_type);
	printf("mbuf->inner_l3_type: %01x\n", m->inner_l3_type);
	printf("mbuf->inner_l4_type: %01x\n", m->inner_l4_type);
	printf("mbuf->ol_flags: %016lx\n", m->ol_flags);
	printf("mbuf->l2_len: %d\n", m->l2_len);
	printf("mbuf->l3_len: %d\n", m->l3_len);
	printf("mbuf->pkt_len: %d\n", m->pkt_len);
	printf("mbuf->buf_len: %d\n", m->buf_len);
	printf("mbuf->data_len: %d\n", m->data_len);
	printf("mbuf->priv_size: %d\n", m->priv_size);
	printf("mbuf->nb_segs: %d\n", m->nb_segs);
	printf("mbuf->next: %016lx\n", (uint64_t)(m->next));

	do {
		snprintf(captial, sizeof(captial), "data length: %d",
			m->data_len);
		l = m->data_len;
		p = m->buf_addr + m->data_off;
		debug_print_binary(captial, p, l);
		m = m->next;
	} while (m != NULL);
}

