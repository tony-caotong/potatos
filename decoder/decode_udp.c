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
	struct udphdr* hdr;
	uint32_t sport, dport;

	hdr = (struct udphdr*)raw;
	sport = hdr->source;
	dport = hdr->dest;

	pkt->tuple5.sport = sport;
	pkt->tuple5.dport = dport;

	return 0;
}
