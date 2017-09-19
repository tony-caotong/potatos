/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-26
 *
 */

#include <assert.h>
#include <netinet/tcp.h>

#include "config.h"
#include "pkt.h"
#include "decode_tcp.h"

int decode_tcp(char* raw, uint32_t len, struct pkt* pkt, uint32_t plen)
{
	struct tcphdr* hdr;
	struct rte_mbuf* m;
	uint32_t sport, dport;
	int16_t hdr_len;

	hdr = (struct tcphdr*)raw;
	sport = hdr->source;
	dport = hdr->dest;

	hdr_len = hdr->doff * 4;

	/* check header length */
	if (hdr_len > 60 || hdr_len < 20) {
		assert(1);
		return -1;
	}
	pkt->tuple5.sport = sport;
	pkt->tuple5.dport = dport;
	pkt->l4_hdr = hdr;

	/* assign l5 header */
	pkt->l5_hdr = raw + hdr_len;
	pkt->l5_len = plen - hdr_len;
	m = pkt->mbuf;
	m->l4_len = hdr_len;

	return 0;
}
