/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-21
 *
 */


#ifndef __WEDGE_H__
#define __WEDGE_H__

#include "config.h"

#ifdef _PLATFORM_DPDK
#include <rte_mbuf.h>

struct wedge_dpdk {
	uint32_t lcore_id;
	struct rte_mbuf* buf;
	uint64_t cur_tsc;
};
#endif

#endif /*__WEDGE_H__*/
