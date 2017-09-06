/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-05
 *
 */

#include <netinet/tcp.h>

/* process logic line */
#define NONE 0
#define CONNECTING 1
#define CLOSING 2
#define TRANSING 3
#define OTHERS 4
/* acknownlege logic line */
#define NOT_ACK 0
#define IS_ACK 1

int stream_tcp_pkt(struct pkt* pkt, struct flow_item* flow)
{
	/* TODO: in different phase, set different timeout value.*/
	/* 1. get tcphdr */
	struct wedge_dpdk w = pkt->platform_wedge;
	rte_mbuf* buf = w.buf;

	struct tcphdr* h = pkt->l4_hdr;

	/* 2. pkt type determine. */
	int16_t hdr_len = h->doff * 4;

	/* 3. we split pkts to five status and two logic line: */
	int pline_stat;
	int aline_stat;

	/*	a. process line.  */
	if (h->syn)
		pline_stat = CONNECTING;
	else if (h->fin)
		/* the pack: who is fin and have payload too, will be handled 
		and closing logic. */
		pline_stat = CLOSING;
	else if (likely(hdr_len != 0))
		pline_stat = TRANSING;
	else
		pline_stat = OTHERS;

	/*	b. acknowledge line. */
	if (likely(h->ack)) 
		aline_stat = IS_ACK;
	else
		aline_stat = NOT_ACK;

	/* 4. compare with status: drop status and logic line unmatched
		 packet. */
	struct stream_tcp* stream = flow->stream;

	switch (stream->status) {
	case CONNECTING:
		if pline_stat != CONNECTIONG
		break;
	case CONNECTED:
		break;
	case CLOSING:
		break;
	case CLOSED:
		break;
	case NONE:
	default:
		break;
	}

	/* 5. deal with bad packets. */
	/* 6. deal with connecting packets. */
	/* 7. deal with closing packets and reset packet. */
	/* 8. deal with urgent data. */
	/* 9. deal with data packets. */

	/* 9.1 determine channel . */
	/* 9.2 determine: window probe & keep alive packet.
		WINDOW_PROBE
		1. data_length == 1
		2. sender send the next 1 byte. only with flag 'ack'.
		3. RESPONSE: acknowledge = current ack; with flag 'ack';
		with window == 0;

		KEEP_ALIVE
		1. sequence = sequence - 1
		2. RESPONSE: acknowledge = sequence + 1
		3. 0 <= data_length <= 1
	 */
	/* 9.3 push packet into channel cache. */
	/* 9.3.1 fix packet out of order. */
	/* 9.3.2 fix packet resent. */
	/* 9.4 pull lastest packet back from channel cache. */

	/* 10. return right packet & right status. */

	return 0;
}
