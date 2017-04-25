/**
 *	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-03-24
 *
 */

#include <stdbool.h>
#include <netinet/ip.h>

#include <rte_ethdev.h>

#include "config.h"
#include "hw_features.h"

static inline void sw_ptype_parse_l4(struct rte_mbuf *m);

/* It is just use for guess, as HW could be not supported check. --!
 * */
int is_hw_parse_ptype_ipv4(int portid)
{
	int i, ret;
	bool ptype_l3_ipv4 = false;
	uint32_t ptype_mask = RTE_PTYPE_L3_MASK;

	/* if 0: may be it's just not support checking, but support parsing. */
	ret = rte_eth_dev_get_supported_ptypes(portid, ptype_mask, NULL, 0);
	if (ret <= 0)
		return false;

	uint32_t ptypes[ret];

	ret = rte_eth_dev_get_supported_ptypes(portid, ptype_mask, ptypes, ret);
	for (i = 0; i < ret; ++i) {
		if (ptypes[i] & RTE_PTYPE_L3_IPV4)
			ptype_l3_ipv4 = 1;
	}

	if (ptype_l3_ipv4)
		printf("port %d cannot parse RTE_PTYPE_L3_IPV4\n", portid);
	else
		return true;

	return false;
}

uint16_t sw_ptype_parse_callback(uint8_t port __rte_unused,
			uint16_t queue __rte_unused,
			struct rte_mbuf *pkts[], uint16_t nb_pkts,
			uint16_t max_pkts __rte_unused,
			void *user_param __rte_unused)
{
	unsigned i;

	for (i = 0; i < nb_pkts; ++i)
		sw_ptype_parse_l4(pkts[i]);

	return nb_pkts;
}

static inline void sw_ptype_parse_l4(struct rte_mbuf *m)
{
	struct ether_hdr *eh;
	void* p;
	uint32_t packet_type = RTE_PTYPE_UNKNOWN;
	uint16_t ether_type;

	eh = rte_pktmbuf_mtod(m, struct ether_hdr *);
	ether_type = eh->ether_type;
	packet_type |= RTE_PTYPE_L2_ETHER;
	p = eh + 1;

	/* deal with just 2 vlans. */
	int i;
	for (i = 0; i < CONFIG_ETH_VLAN_EMBED_LIMIT && ether_type 
			== rte_cpu_to_be_16(ETHER_TYPE_VLAN); i++) {
		struct vlan_hdr *vh = p;
		ether_type = vh->eth_proto;
		m->ol_flags |= PKT_RX_VLAN_PKT;
		p = vh + 1;
	}

	if (ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4)) {
		struct iphdr *iph = p;
		packet_type |= RTE_PTYPE_L3_IPV4_EXT_UNKNOWN;
		
		/* TODO: found Frag or not. */
		
		switch (iph->protocol) {
		case IPPROTO_TCP:
			packet_type |= RTE_PTYPE_L4_TCP;
			break;
		case IPPROTO_UDP:
			packet_type |= RTE_PTYPE_L4_UDP;
			break;
		/* TODO: 82599EB do not support, I'm not too..
		case IPPROTO_ICMP:
			break;
		case IPPROTO_SCTP:
			break;
		*/
		default:
			break;
		}
		
	} else if (ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv6)) {
		packet_type |= RTE_PTYPE_L3_IPV6_EXT_UNKNOWN;
	}
	m->packet_type = packet_type;
}

