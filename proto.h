/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-02-27
 *
 */

#ifndef __PROTO_H__
#define __PROTO_H__

#undef addprotocol
#define addprotocol(item) add_proto_item(item, #item),

addprotocol(PROTO_ETHER)
addprotocol(PROTO_IPV4)
addprotocol(PROTO_IPV6)
addprotocol(PROTO_ICMP)
addprotocol(PROTO_IGMP)
addprotocol(PROTO_TCP)
addprotocol(PROTO_UDP)

#endif /*__PROTO_H__*/
