/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-03-24
 *
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <rte_cycles.h>

#define CONFIG_P_PKT_SNAPLEN		8192
#define CONFIG_ETH_VLAN_EMBED_LIMIT	2
#define CONFIG_FLOW_NUM_LIMIT		0x1000
#define CONFIG_FLOW_TTL_LIMIT		MS_PER_S
#define CONFIG_IP_FRAG_NUM_LIMIT	8
#define CONFIG_DPDK_PREFETCH_OFFSET	3
#define CONFIG_FLOW_IPV4_HASH_ENTRIES	(1024*1024)
#define CONFIG_FLOW_IPV4_TIMEOUT_SECS	5
#define CONFIG_FLOW_IPV4_TIMEOUT_EACH	8
#define CONFIG_TCP_STREAM_ENTRIES	(1024*1024)

#define _PLATFORM_DPDK

#endif /*__CONFIG_H__*/
