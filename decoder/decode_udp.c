/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-26
 *
 */

#include <netinet/udp.h>

#include "config.h"
#include "pkt.h"
#include "decode_udp.h"

int decode_udp(char* raw, uint32_t len, struct pkt* pkt, uint32_t plen)
{
	struct rte_mbuf* m;
	struct udphdr* hdr;
	uint32_t sport, dport;
	uint16_t hdr_len;

	hdr = (struct udphdr*)raw;
	hdr_len = sizeof(struct udphdr);
	sport = hdr->source;
	dport = hdr->dest;

	pkt->tuple5.sport = sport;
	pkt->tuple5.dport = dport;

	/* assign l5 header */
	pkt->l5_hdr = raw + hdr_len;
	pkt->l5_len = plen - hdr_len;
	m = pkt->mbuf;
	m->l4_len = hdr_len;

	return 0;
}
