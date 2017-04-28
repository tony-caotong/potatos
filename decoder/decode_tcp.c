/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-26
 *
 */

#include <netinet/tcp.h>

#include "config.h"
#include "pkt.h"
#include "decode_tcp.h"

int decode_tcp(char* raw, uint32_t len, struct pkt* pkt)
{
	struct tcphdr* hdr;
	uint32_t sport, dport;

	hdr = (struct tcphdr*)raw;
	sport = hdr->source;
	dport = hdr->dest;

	pkt->tuple5.sport = sport;
	pkt->tuple5.dport = dport;

	return 0;
}
