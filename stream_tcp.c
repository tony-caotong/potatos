/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-05
 *
 */

#include <stdlib.h>
#include <assert.h>
#include <netinet/tcp.h>
#include <netinet/in.h>

#include <rte_branch_prediction.h>

#include "flow.h"
#include "stream_tcp.h"

/* macro below Bring in from suricata.
	https://tools.ietf.org/html/rfc793#section-3.4 */
/* Macro's for comparing Sequence numbers
 * Page 810 from TCP/IP Illustrated, Volume 2. */
#define SEQ_EQ(a,b)  ((int32_t)((a) - (b)) == 0)
#define SEQ_LT(a,b)  ((int32_t)((a) - (b)) <  0)
#define SEQ_LEQ(a,b) ((int32_t)((a) - (b)) <= 0)
#define SEQ_GT(a,b)  ((int32_t)((a) - (b)) >  0)
#define SEQ_GEQ(a,b) ((int32_t)((a) - (b)) >= 0)

/*
pkt:	  seq   seq+l5_len
	   v      v
	   |------|
channel:	last_seq     next_seq
	            v            v
	------------|zzzzzzzzzzzz|------------------------------------>
situat:	1. |------|
	2.       |------|
	3.             |-------|
	                   |-----|
	4.                 |----------|
	4.1         |xxxxxxxxxxxxxxxxx|
	4.2                  |xxxxxxxx|
	5.                              |--------|
	5.1                      |xxxxxxxxxxxxxxx|
	5.2                                |xxxxx|
	------------|------------|------------------------------------>
	We have 7 situations about the length of next data pkt recvied.
	1,2,3:
		it was handled by function is_resend(), and drop.
	4.1:
		it was a reassembled packet by sender was pkts resending.
		just mark next_seq is the begining of this pkt for app().
	4.2:
		It was a never happend situation in my opinion.
		but whatever it is, we do the same thing with 4.1.
	5.1:
		It is normal.
	5.2:
		out of order or missing previous packets.
		a. For ofo: cached and waiting the previous one.
		b. actualy, we do not known whether it is messing or not, so
			cached also. but when timing out it still isn't arrived
			, free (cache, stream and flow) or mark it is invalid.
*/
static bool is_resend(struct pkt* pkt, struct channel* c) 
{
	struct tcphdr* h;
	uint32_t next_seq;

	h = pkt->l4_hdr;
	next_seq = ntohl(h->seq) + pkt->l5_len;
	if (SEQ_LEQ(next_seq, c->next_seq))
		return true;
	return false;
}

/*
 *	Actually, it is 'sequence = sequence - 1'
 */	
static bool is_keep_alive_0byte(struct pkt* pkt, struct channel* c)
{
	struct tcphdr* h;

	h = pkt->l4_hdr;
	if (SEQ_LEQ(ntohl(h->seq), c->last_seq))
		return true;
	return false;
}

/* 9.3 push packet into channel cache. */
/* 9.3.1 fix packet out of order. */
/* 9.3.2 fix packet resent. */
/* 9.4 pull lastest packet back from channel cache. */
static int channel_pkt(struct pkt* pkt, struct channel* c)
{
	struct tcphdr* h;
	int16_t datalen;
	uint32_t seq, next_seq;
	int r;

	r = RE_PKT_SUCC;
	h = pkt->l4_hdr;
	datalen = pkt->l5_len;
	seq = ntohl(h->seq);
	next_seq = seq + datalen;
//	TODO
//	int16_t win;
//	win = h->window;

	/* deal with MIDDLE. */
	if (unlikely(c->status == TCP_C_NONE)) {
		if (c->last_seq == 0) {
			/* first MIDDLE pkt. */
			c->last_seq = h->seq;
			c->next_seq = c->last_seq + datalen;
		} else {
			/* following MIDDLE pkt, before newest one was ack'd.
			 * after that, status will be transed to ESTABLISHED.*/
			if (SEQ_LEQ(h->seq, c->last_seq))
				return 0;
			c->last_seq = h->seq;
			c->next_seq = c->last_seq + datalen;
		}
	}

	/* no cared pack. */
	if (c->status != TCP_C_ESTABLISHED)
		return RE_PKT_DROP;
	
	/* now let's do the real channel work. */
	/* Also see the graph commented above function is_resend(). */
	if (unlikely(SEQ_LEQ(next_seq, c->next_seq))) {
		/* situation 1,2,3. */
		/* Never happened, as it's already handled in is_resend(). */
		return RE_PKT_DROP;
	} else if (SEQ_LT(seq, c->next_seq)) {
		assert(SEQ_GT(seq, c->last_seq));
		int dup_size = 0;

		/* situation 4 */
		dup_size = seq - next_seq;
		c->last_seq = seq;
		c->next_seq = next_seq;
		pkt->type |= PKT_TYPE_DULPLICATE_L5DATA;
		pkt->app_begin = pkt->l5_hdr + dup_size;
	} else if (SEQ_GT(seq, c->next_seq)) {
		/* situation 5.2 */
		if (tcp_reassemble_push(&c->assemble_cache, pkt) < 0) {
			r = RE_STREAM_TAINTED;
		} else {
			r = RE_PKT_CACHED;
		}
		return r;
	} else { /* seq == c->next_seq */
		/* situation 5.1 normal pkt. */
		c->last_seq = seq;
		c->next_seq = next_seq;
		r = RE_PKT_SUCC;
	}

	/* 1. find followers who was cached. */
	/* 2. append them together with rte indirect buff. */
	/*
		Yesterday: the interface design like this: 	
			1. tcp_reassemble_pull(&c->assemble_cache, c->next_seq)
			2. chain(pkt, tmp);
		Now:
			As hide function chain() is more elaborate, parameter
			of the func was changed,.
			Only two situation, we can get there, 
				A. a current perfect pkt (5.1).
				B. biger duplicate pkt (4).
			So c->next_seq must equal to pkt->next_seq;
	*/
	tcp_reassemble_pull(&c->assemble_cache, pkt);
	/* 3. update channel param. */
	c->next_seq = seq + pkt->l5_len;
	return r;
}

static int ack(struct pkt* pkt, uint8_t pkt_orient, struct stream_tcp* s)
{
	struct channel* c, *op;
	struct tcphdr* h;
	uint32_t ack;

	if (unlikely(s->status == TCP_S_NONE)) 
		return 0;

	h = pkt->l4_hdr;
	ack = ntohl(h->ack_seq);
	if (s->up_orient == pkt_orient) {
		c = &s->up;
		op = &s->down;
	} else {
		c = &s->down;
		op = &s->up;
	}
	
	if (unlikely(s->status == TCP_S_MIDDLEING)) {
		/* this flag was taken from data(). two things need to do:
			1. ack the before arriving data in opposite channel.
			2. trans MIDDLEING to CONNECTED.

		   NOTE: Might there be a situation ? 
			the ack pkt always ack'd the the front one of the last
			pkt int oppo channel,
		   acked_seq == 0 is the flag of connecting finished in this
		   channel.	*/
		/* also op->status = TCP_C_NONE is the same thing.
			which one is better ? 
		*/
		if (op->acked_seq == 0) {
			if (op->next_seq == ack) {
				/* half CONNECTED now. */
				op->acked_seq = ack;
				op->status = TCP_C_ESTABLISHED;
				/* do another half. */
				if (c->status == TCP_C_ESTABLISHED) {
					assert(c->acked_seq != 0);
					s->status = TCP_S_CONNECTED;
				}
			}
		} else {
			/* half CONNECTED, do normal ack things.*/
			if (SEQ_GEQ(op->next_seq, ack)
					&& SEQ_LT(op->acked_seq, ack)) {
				op->acked_seq = ack;
			}
			return 0;
		}
	}

	/* now let's do the real ack work. */
	else if (s->status == TCP_S_CONNECTING) {
		if (op->next_seq == ack) {
			assert(op->acked_seq == 0);
			op->acked_seq = ack;
			op->status = TCP_C_ESTABLISHED;
			if (c->status == TCP_C_ESTABLISHED) {
				s->status = TCP_S_CONNECTED;
			}
		}
	} else if (s->status == TCP_S_CLOSING) {
		if (op->status == TCP_C_FIN_WAIT1 && op->next_seq == ack) {
			op->acked_seq = ack;
			op->status = TCP_C_FIN_WAIT2;
			if (c->status == TCP_C_FIN_WAIT2) {
				op->status = TCP_C_CLOSED;
				c->status = TCP_C_CLOSED;
				/*
				   s->status = TCP_S_CLOSING_WAIT;
				   Should we support CLOSING_WAIT ?
				*/
				s->status = TCP_S_CLOSED;
				return RE_STREAM_CLOSED;
			}
		}
	} else if (s->status == TCP_S_CONNECTED) {
		/* both resend and ack will be drop. */
		if (is_keep_alive_0byte(pkt, c))
			return 0;
		if (SEQ_LT(op->next_seq, ack) && SEQ_LT(op->acked_seq, ack)) {
			op->acked_seq = ack;
		}
	} else {
		/* skip */;
	}
	return 0;
}

static int data(struct pkt* pkt, uint8_t pkt_orient, struct stream_tcp* s)
{
	int r;
	struct channel* c;

	/* *. Got this packet from middle of a connecting. 
	   The first data packet will change status from TCP_S_NONE to 
		TCP_S_MIDDLEING.
	   then in MIDDLEING status, the first data of each channel will be
		arriving, and the ack packet of this two will be arriving,
	   and the last one of this two ack packets will change the status
		from TCP_S_MIDDLEING to TCP_S_CONNECTED. the code is located
		under ack().

	   This also determined by channel:
		each ack will trans TCP_C_NONE to TCP_C_ESTABLISHED.
		final, two TCP_C_ESTABLISHED will trigger TCP_S_MIDDLEING 
		to TCP_S_CONNECTED.
	*/
	if (unlikely(s->status == TCP_S_NONE)) {
		s->up_orient = FLOW_NORTH;
		s->status = TCP_S_MIDDLEING;
		s->flags |= STREAM_TCP_FLAG_MID_CREATE;
	}

	/* 9.1 determine channel . */
	if (s->up_orient == pkt_orient)
		c = &s->up;
	else
		c = &s->down;
		
	if (s->status == TCP_S_CONNECTED) {
		/* 9.2 determine: window probe & keep alive packet.
			WINDOW_PROBE
			1. data_length == 1
			2. sender send the next 1 byte. only with flag 'ack'.
			3. RESPONSE: acknowledge = current ack; 
				with flag 'ack'; with window == 0;

			KEEP_ALIVE
			1. sequence = sequence - 1
			2. RESPONSE: acknowledge = sequence + 1
			3. 0 <= data_length <= 1
		*/
		/* But, actually we will accept the 1byte from the first
			WINDOW_PROBE packet.
		   and if KEEP_ALIVE is with 1byte data, it actually is
			a resend.
		   so only handle resend and 0byte_keep_alive() under ack().
		*/
		if (is_resend(pkt, c))
			return 0;
	}

	if (s->status != TCP_S_MIDDLEING && s->status != TCP_S_CONNECTED
		&& s->status != TCP_S_CLOSING) {
		/* skip unexpecting pkt. */
		return 0;
	}
	
	r = channel_pkt(pkt, c);
	if (r == RE_STREAM_TAINTED)
		s->status |= STREAM_TCP_FLAG_INVALID;
	return r;
}

static int syn(struct pkt* pkt, uint8_t pkt_orient, struct stream_tcp* s)
{
	struct tcphdr* h;
	struct channel* c, *op, *u, *d;
	uint32_t seq;

	h = pkt->l4_hdr;
	seq = h->seq;

	u = &s->up;
	d = &s->down;

	/* it will be never TCP_S_CLOSED, as design in the process of last pkt
		who set the CLOSED flag will trigger free(flow).*/
	/* FLAG: syn 1. */
	if (s->status != TCP_S_NONE && s->status != TCP_S_CONNECTING)
		return 0;

	if (s->up_orient == FLOW_ORIENT_UNKNOWN) {
		assert(s->status == TCP_S_NONE);
		/* means: we are the first pkt of this stream. */
		if (h->ack) {
			/* but: this is a syn-ack pkt. so it's MIDDLE.*/
			/* another problem is ofo of syn:
				it is also going here, and the second syn will
				be droped(in FLAG: syn 1.).
			*/
			s->status = TCP_S_MIDDLEING;
			s->flags |= STREAM_TCP_FLAG_MID_CREATE;

			/* opposite */
			if (pkt_orient == FLOW_NORTH) 
				s->up_orient = FLOW_SOUTH;
			else
				s->up_orient = FLOW_NORTH;
			c = d;

			/* do the similar things as Middle. */
			c->last_seq = ntohl(seq);
			c->next_seq = c->last_seq + 1;

			return 0;
		} else {
			/* first syn. */
			s->status = TCP_S_CONNECTING;
			s->up_orient = pkt_orient;

			c = u;
			op = d;
			c->status = TCP_C_SYN_SENT;
			c->last_seq = ntohl(seq);
			c->next_seq = c->last_seq + 1;
			/* TCP_C_SYN_RECV: it's not really useful, 
				TCP_C_SYN_SENT in both side is the situation
				to ESTABLISHED. */
			op->status = TCP_C_SYN_RECV;
		}
	} else {
		assert(s->status == TCP_S_CONNECTING
			|| s->status == TCP_S_MIDDLEING);
		if (unlikely(!h->ack)) {
			/* open together. */
		} else {
			/* second syn. */
		}

		c = d;
		assert(c->status == TCP_C_SYN_RECV);
		c->last_seq = ntohl(seq);
		c->next_seq = c->last_seq + 1;
		c->status = TCP_C_SYN_SENT;
	}
	return 0;
}

static int fin(struct pkt* pkt, uint8_t pkt_orient, struct stream_tcp* s)
{
	struct channel* c;

	if (s->status != TCP_S_CONNECTED && s->status != TCP_S_CLOSING)
		return 0;

	if (s->up_orient == pkt_orient) {
		c = &s->up;
	} else {
		c = &s->down;
	}

	if (c->status <= TCP_C_ESTABLISHED) {
		c->next_seq += 1;
		c->status = TCP_C_FIN_WAIT1;
		s->status = TCP_S_CLOSING;
	}
	return 0;
}

static int rst(struct pkt* pkt, uint8_t pkt_orient, struct stream_tcp* s)
{
	s->up.status = TCP_C_CLOSED;
	s->down.status = TCP_C_CLOSED;
	s->status = TCP_S_CLOSED;
	
	return RE_STREAM_CLOSED;
}

static int urg(struct pkt* pkt, uint8_t pkt_orient,  struct stream_tcp* s)
{
	return 0;
}

/* 
	1. get tcphdr.
	2. pkt type determine.
	3. we split pkts to five status and two logic line: 
		a. process line. 
			conn, close, data, others
		  the pack: who is fin and have payload too, will be handled 
		and closing logic.
		b. acknowledge line. 
			ack
	4. compare with status: drop status and logic line unmatched
		 packet.

	5. deal with bad packets. TODO
		lots of under snort, just ignore here.
	6. deal with connecting packets.
	7. deal with closing packets and reset packet.
	8. deal with urgent data.
	9. deal with data packets.
	99. in different phase, set different timeout value. TODO

	RETURN value:
		RE_STREAM_CLOSED;
		RE_PKT_CACHED;
*/
int stream_tcp_pkt(struct pkt* pkt, uint8_t pkt_orient,
		struct stream_tcp* stream)
{
	/* 1. */
	struct tcphdr* h = pkt->l4_hdr;
	int16_t datalen = pkt->l5_len;
	int r = -1;

	if ((stream->flags & STREAM_TCP_FLAG_INVALID) != 0)
		return RE_COMMON_ERR;

	/* 2. */
	/* 3. 4. */
	/* order is important as fin pack might be with data. */
	if (unlikely(h->urg)) {
		/* 8. TODO: unsupporting now. */
		r = urg(pkt, pkt_orient, stream);
		if (r == 0)
			r = ack(pkt, pkt_orient, stream);
		goto skip;
	}
	/* do ack() before data(), as pkt could be cached in data(). */
	/* But if 'fin' was cached, what should we do ? */
	if (likely(h->ack))
		/* 3. */
		r = ack(pkt, pkt_orient, stream);
	if (likely(datalen)) {
		if (unlikely(h->fin && stream->status <= TCP_S_MIDDLEING))
			/* As MIDDLE create features here, let fin escape. */
			;
		else {
			/* But a ofo fin could be going here, than reassembled
				and go back, itself or chained with by leaders.
				and the leader will inheriting the FIN flag. 
			   This logic was implemented in function chain().
			 */
			/* 9. */
			r = data(pkt, pkt_orient, stream);
			if (r < 0 || r == RE_PKT_CACHED)
				goto skip;
		}
	}
	if (h->syn)
		/* 6. */
		r = syn(pkt, pkt_orient, stream);
	if (unlikely(h->rst))
		r = rst(pkt, pkt_orient, stream);
	if (h->fin)
		/* 7. */
		r = fin(pkt, pkt_orient, stream);

	if (unlikely(r == -1))
		assert(true);
	/* 10. return right status. */
skip:
	return r;
}

int stream_tcp_init(struct stream_tcp* tcp)
{
	struct tcp_reassemble* rsb;

	tcp->up_orient = FLOW_ORIENT_UNKNOWN;
	tcp->up.last_seq = 0;
	tcp->up.status = TCP_C_NONE;
	tcp->down.last_seq = 0;
	tcp->down.status = TCP_C_NONE;
	tcp->flags = 0;

	rsb = &(tcp->up.assemble_cache);
	tcp_reassemble_init(rsb);
	rsb = &(tcp->down.assemble_cache);
	tcp_reassemble_init(rsb);

	return 0;
}

int stream_tcp_destroy(struct stream_tcp* tcp)
{
	struct tcp_reassemble* rsb;

	rsb = &(tcp->up.assemble_cache);
	tcp_reassemble_destory(rsb);
	rsb = &(tcp->down.assemble_cache);
	tcp_reassemble_destory(rsb);

	return 0;
}

