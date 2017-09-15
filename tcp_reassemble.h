/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-08
 *
 */

#ifndef __TCP_REASSEMBLE_H__
#define __TCP_REASSEMBLE_H__

#include <stdbool.h>

#include "pkt.h"
#include "list.h"

#define TCP_REASSEMBLE_COUNT_LIMIT 5
#define TCP_REASSEMBLE_TS_LIMIT 3
/*
 * mechanism of timeout/resend/pkt-missing/ofo:
 *	1. normal business data-flow.
		1.1 ==> 2.3.2
 *	2. unnormal 
		2.1 no data pkts.
			2.1.1 no any data pkts/ control pkts.
				** XXX: flow timeout1. **
			2.1.2 no data pkts but keep_alive.
				** XXX: flow timeout2. **
			2.1.3 no data pkts but window_probe.(pending/blocked)
				** XXX: flow timeout3. **
		2.2 pkts was missing in traffic link.
			** XXX: reassemble cache timeout. **
			2.2.1 one pkt was always resending.
		2.3 pkts was missing in mirror link.
			** XXX: reassemble cache timeout. **
			2.3.1 reassemble cache was growing up, means reassemble
				was blocked at the missed pkt position.
			2.3.2 not really missing, the newest pkt hit the
				setting of cache limit. we consider it is 
				missing.
 */
struct tcp_reassemble {
	struct list_head list;
	uint32_t count;
	uint32_t seq;
	time_t ts;
};

int tcp_reassemble_init(struct tcp_reassemble* tr);
int tcp_reassemble_push(struct tcp_reassemble* tr, struct pkt* pkt);
int tcp_reassemble_pull(struct tcp_reassemble* tr, struct pkt* pkt);
int tcp_reassemble_destory(struct tcp_reassemble* tr);

bool tcp_reassemble_timeout(struct tcp_reassemble* tr);

#endif /*__TCP_REASSEMBLE_H__*/
