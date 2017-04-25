/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-18
 *
 */

#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/in.h>

#include "config.h"
#include "wedge.h"
#include "debug.h"
#include "decode_ipv4.h"
#include "ip_reassemble.h"

int decode_ipv4(char* raw, int len, struct pkt* pkt)
{
	int r = 0;
	uint32_t sip __attribute__((unused)), dip __attribute__((unused)); 
	uint16_t frag_off, id __attribute__((unused)), protocol;
	uint32_t l __attribute__((unused)), hlen __attribute__((unused)),
		snaplen = len;
	char* p = raw;
	struct iphdr* iph = (struct iphdr*)p;
	
	if (snaplen < 20)
		return -1;
	if (iph->version != 4)
		return -1;

	frag_off = ntohs(iph->frag_off);
	if (IPV4_IS_FRAG(frag_off)) {
		char* out;
		unsigned ol;
		
#ifdef _PLATFORM_DPDK
		struct wedge_dpdk* wedge = pkt->platform_wedge;
		struct rte_mbuf* m = wedge->buf;
		
		m->l3_len = iph->ihl * 4;
#endif
		if ((p = ipv4_reassemble(raw, len, &out, &ol, pkt)) == NULL)
			return r;
		iph = (struct iphdr*)p;
		snaplen = ol;
#ifdef _PLATFORM_DPDK
		fprintf(stderr, "packets reassembled!\n");
		debug_print_mbuf_infos(wedge->buf);
#endif
	}

	hlen = iph->ihl * 4;
	l = ntohs(iph->tot_len);
	/* do not check valid of 'l' and 'len', as pkt might be snaped. */
	protocol = iph->protocol;
	sip = iph->saddr;
	dip = iph->daddr;

	switch (protocol) {
	case IPPROTO_TCP:
		break;
	case IPPROTO_UDP:
		break;
	default:
		break;
	}

	return r;
}
