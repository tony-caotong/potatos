/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-03-24
 *
 */

#ifndef __HW_FEATURES__
#define __HW_FEATURES__

#define MAX_VLAN_EMBED 2
#define IS_HW_VLAN_PKT(ol) ((ol) & PKT_RX_VLAN_PKT)

int is_hw_parse_ptype_ipv4(int port_id);
uint16_t sw_ptype_parse_callback(uint8_t port, uint16_t queue,
	struct rte_mbuf *pkts[], uint16_t nb_pkts, uint16_t max_pkts,
			void *user_param);

#endif /*__HW_FEATURES__*/
