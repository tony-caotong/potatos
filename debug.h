/**
 *	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-03-27
 *
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <rte_mbuf.h>

void debug_print_mbuf_infos(struct rte_mbuf* buf);

#endif /*__DEBUG_H__*/
