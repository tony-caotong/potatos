/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-18
 *
 */

#include <net/ethernet.h>
#include <arpa/inet.h>

#include "config.h"
#include "trans_decoder.h"
#include "decode_ipv4.h"
#include "ip_reassemble.h"

#ifdef _PLATFORM_DPDK
#include "rte_mbuf.h"
#endif

int decode_pkt(char* raw, int len, struct pkt* pkt)
{
	struct ether_header* hdr;
	uint16_t type;
	int i, l = len, r = 0;
	void* p;
	
	int l2_len;
	struct rte_mbuf* m;

	hdr = (struct ether_header*)raw;
	p = raw + sizeof(struct ether_header);
	l -= sizeof(struct ether_header);
	l2_len = sizeof(struct ether_header);
	if (!proto_push(pkt, hdr, PROTO_ETHER))
		return -1;
	type = ntohs(hdr->ether_type);
	for (i = 0; i < CONFIG_ETH_VLAN_EMBED_LIMIT
			&& type == ETHERTYPE_VLAN; i++) {
		struct vlan_header *vh = p;
		type = ntohs(vh->eth_proto);
		p = vh + 1;
		l -= sizeof(struct vlan_header);
		l2_len += sizeof(struct vlan_header);
		if (!proto_push(pkt, vh, PROTO_VLAN))
			return -1;
	}

	m = pkt->mbuf;
	m->l2_len = l2_len;

	pkt->l3_proto = type;
	switch (type) {
	case ETHERTYPE_IP:
		r = decode_ipv4(p, l, pkt);
		break;
	case ETHERTYPE_IPV6:
		/*r = decode_ipv6(p, l, pkt);*/
		break;
	case ETHERTYPE_ARP:
		/*r = decode_arp(p, l, pkt);*/
		break;
	default:
		pkt->l3_proto = 0;
		break;
	}

	return r;
}
