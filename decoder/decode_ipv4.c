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
#include "decode_tcp.h"
#include "decode_udp.h"

int decode_ipv4(char* raw, int len, struct pkt* pkt)
{
	int r = RE_DECODER_SUCC;
	uint32_t sip __attribute__((unused)), dip __attribute__((unused)); 
	uint16_t frag_off, id __attribute__((unused)), protocol;
	uint32_t l, hlen, snaplen = len;
	char* p = raw;
	struct iphdr* iph = (struct iphdr*)p;
	
	char* out;
	unsigned ol;

	if (snaplen < 20)
		return -1;
	if (iph->version != 4)
		return -1;

	/* TODO: Filter could be process here. */

	/* 1. reassemble ipv4 frag pkts. */
	frag_off = ntohs(iph->frag_off);
	if (IPV4_IS_FRAG(frag_off)) {
#ifdef _PLATFORM_DPDK
		struct wedge_dpdk* wedge = pkt->platform_wedge;
		struct rte_mbuf* m = wedge->buf;
		
		m->l3_len = iph->ihl * 4;
#endif
		if ((p = ipv4_reassemble(raw, len, &out, &ol, pkt)) == NULL)
			return RE_DECODER_CACHED;
		iph = (struct iphdr*)p;
		snaplen = ol;
		pkt->type |= PKT_TYPE_REASSEMBLED;
#ifdef _PLATFORM_DPDK
		fprintf(stderr, "packets reassembled!\n");
		debug_print_mbuf_infos(wedge->buf);
#endif
	}

	/* 2. decode sub protocol. */
	hlen = iph->ihl * 4;
	l = ntohs(iph->tot_len);
	/* do not check valid of 'l' and 'len', as pkt might be snaped. */
	protocol = iph->protocol;
	/*
	sip = iph->saddr;
	dip = iph->daddr;
	id = iph->id;
	*/

	pkt->tuple5.sip = iph->saddr;
	pkt->tuple5.dip = iph->daddr;
	pkt->tuple5.l4_proto = protocol;
	pkt->l3_hdr = iph;

	if (!proto_push(pkt, iph, PROTO_IPV4))
		return -1;

	switch (protocol) {
	case IPPROTO_TCP:
		r = decode_tcp(p + hlen, len - hlen, pkt, l - hlen);
		break;
	case IPPROTO_UDP:
		r = decode_udp(p + hlen, len - hlen, pkt, l - hlen);
		break;
	case IPPROTO_ICMP:
		break;
	default:
		pkt->tuple5.l4_proto = 0;
		r = RE_DECODER_DROP;
		break;
	}
	return r;
}

