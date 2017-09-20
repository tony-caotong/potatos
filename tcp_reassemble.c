/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-08
 *
 */

#include <assert.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include "list.h"
#include "tcp_reassemble.h"

static inline void seq(struct pkt* pkt, uint32_t* seq, uint32_t* nseq)
{
	struct tcphdr* h;

	h = pkt->l4_hdr;
	*seq = ntohl(h->seq);
	/* event if this packet is a merged one. 
		The l5_len is also maintained well. */
	*nseq = *seq + pkt->l5_len;
}

/*
 *	Before reading this code, first:
 *	Better to learn from ipv4_frag_reassemble():
 *		dpdk_root/lib/librte_ip_frag/rte_ipv4_reassembly.c 
 */
static int chain(struct pkt* first, struct pkt* second)
{
	/* first pkt may be a chained one,
		the second pkt might be chained too. */

	struct rte_mbuf* f, *s;
	struct pkt* pkt;
	uint32_t fs, fn, ss, sn;
	uint32_t adj;;
	uint8_t th_fin;
	int count;

	/* a situation need to be handled: 
		first segment for second pkt was coverd by first pkt,
		so, a. judgement this situation, b. free the first segment.
		c. adjust the second segment. d. chain first pkt and second
		segment and followers. */
	seq(first, &fs, &fn);
	seq(second, &ss, &sn);
	f = first->mbuf;
	s = second->mbuf;
	adj = 0;
	th_fin = 0;
	count = 0;
	if (fn > ss) {
		uint32_t pkt_len, ss_cur;
		struct rte_mbuf* tmp;				

		/*	fs          fn
			|xxxxxxxxxxx|
			      ss    ^      sn
			      |xxxxxxxxxxxxx|
			         ^  ^
			        ss_cur
			            ^
			           adj
		*/
		ss_cur = ss;
		pkt = (void*)s + sizeof(struct rte_mbuf);
		while (s && ss_cur + pkt->l5_len < fn) {
			/* TODO: logic is not good here, as s->pkt_len and 
			pkt->l5_len depand on each other. */
			pkt_len = s->pkt_len - s->data_len;
			tmp = s;
			ss_cur += pkt->l5_len;
			s = s->next;
			s->pkt_len = pkt_len;
			s->nb_segs = tmp->nb_segs - 1;
			rte_pktmbuf_free(tmp);
			count--;
		}
		assert(s != NULL); /* Caller makes that could be. */
		assert(s->nb_segs >= 1);
		if (!s)
			return count;
		assert(ss_cur >= sn);
		adj = fn - ss_cur;
	}
	assert(!(fn > ss)); /* Caller makes that could be. */
	
	/* IMPORTANT: FIN flag should be reserved.
		any packet droped before, is a duplicate one, of cause 
		FIN is never covered by a pkt with longger sequence
		dnumber.
	 */
	/* TODO: this line could be more beautiful. */
	pkt = (void*)s + sizeof(struct rte_mbuf);
	th_fin = ((struct tcphdr*)(pkt->l4_hdr))->fin;

	/* adjust */
	adj += (s->l2_len + s->l3_len + s->l4_len);
	rte_pktmbuf_adj(s, adj);

	/* For struct pkt:
		update first pkt's l5_len. */
	first->l5_len += (s->pkt_len - pkt->pad_len);
	count += s->nb_segs;
	/* merge FIN flag only. */
	((struct tcphdr*)(first->l4_hdr))->fin |= (th_fin & 0x00000001u);
	/* NOTE: 
		first: 'nb_segs' and 'pkt_len = whole'
		second: 'pkt_len = data_len' ;
	*/
	rte_pktmbuf_chain(f, s);
	
	return count;
}

static void* pull(struct tcp_reassemble* tr, uint32_t seq_expect)
{
	struct list_head* first;
	struct pkt* p;
	struct rte_mbuf* m;
	uint32_t se, nseq;

	if (list_empty(&tr->list)) 
		return NULL;

	first = tr->list.next;
	p = list_entry(first, struct pkt, node);
	seq(p, &se, &nseq);
	if (se <= seq_expect) {
		list_del(first);
		m = p->mbuf;
		tr->count -= m->nb_segs;
		/* update ts, if success pull. */
		tr->ts = time(NULL);
		return p;
	}
	return NULL;
}


/*
 *	We consider the pkt goin there which is already compared to the channel
 *	sequence, so unnecessary to call reassemble(), just insert.
 */
int tcp_reassemble_push(struct tcp_reassemble* tr, struct pkt* pkt)
{
	uint32_t sequence, nseq;
	struct pkt* cur;
	struct list_head* p, *n, *pre;
	bool step_a;

	if (tr->count >= TCP_REASSEMBLE_COUNT_LIMIT)
		return -1;

	/* update ts. */
	tr->ts = time(NULL);
	seq(pkt, &sequence, &nseq);

	if (list_empty(&tr->list)) {
		list_add(&pkt->node, &tr->list);
		tr->count++;
		return 0;
	}
	/*
	   pkt:              A              B         C          D
	             ...--|xxxxxx|-------|xxxxx|----|xxxx|----|xxxxx|--...->
	   situation 1.            |zz|
	   situation 2.            |zzzzzzz|
	   situation 3.            |zzzzzzzzzzzzzzzzzzzzzzz|
	   situation 4.       |zzzzzzz|
	                      s       n
	   pkt:              A 
	             ...--|xxxxxx|          B         C          D
	                       |xxxxxxxxxxxxxxx|----|xxxx|----|xxxxx|--...->
	   situation 5.                              |zz|
	   situation 6.      |zzzzzz|
	                     s      n
	   pkt:              A          B          C
	             ...--|xxxxxx|----|xxxx|----|xxxxxx|    D
	                                            |xxxxxxxxxxxx|-----...->
	   situation 7.                       |zzz|
	   situation 8.                       |zzzzzz|
	   situation 9.                       |zzzzzzzzzz|
	   situation 10.      |zzzzzzzzzz|
	                      s          n

	   step A: use 's' to find the right positon between pkts.
	   step B: use 'n' to decide what to do.
	        1. just insert.   
	        2. We also consider it is never happend, but we do the same
	           things as we discussed in stream_tcp.h:4.1/4.2. 
	           set a mark in packet B and merge A&B together.   
	        3. free B and C then insert.  
		4. same as '2', but set mark in current pkt.
	        5. just free current pkt.
	        6. just free current pkt.
		7/8/9. We already merged C&D together.
		7. so merge pkt&C&D together.
		8. Free C and merge pkt&D together.
		9. Free C and merge pkt&D together.
		10. merge A&pkt&B together.
	 */
	pre = NULL;
	cur = pkt;
	step_a = false;
	list_for_each_safe(p, n, &tr->list) {
		uint32_t s, n;
		struct pkt* packet;
		
		packet = list_entry(p, struct pkt, node);
		seq(packet, &s, &n);

		/* Better: '<' not '<=' . */
		if (!step_a && sequence < s) {
			/* deal with left position. */
			if (pre) {
				uint32_t pre_s, pre_n;
				struct pkt* p_pkt;
		
				p_pkt = list_entry(pre, struct pkt, node);
				seq(p_pkt, &pre_s, &pre_n);
				if (sequence <= pre_n) {
					if (nseq <= pre_n) {
						struct rte_mbuf* m;

						/* 5 */
						m = get_mbuf(cur);
						tr->count -= m->nb_segs;
						rte_pktmbuf_free(m);
						break;
					} else {
						/* 4,6,10 */
						tr->count += chain(p_pkt, cur);
						cur = p_pkt;
					}
				} else {
					/* 1,2,3,7,8,9 */
					list_add(&cur->node, pre);
					tr->count++;
				}
			} else { /* insert first. */
				list_add(&cur->node, &tr->list);
				tr->count++;
			}
			step_a = true;
		}

		/* deal with right position. */
		if (step_a) {
			if (nseq < s) {
				/* work done. for 1 */
				break;
			} else if (nseq < n) {
				/* 2,7,8,9,10 */
				tr->count += chain(cur, packet);
			} else {
				struct rte_mbuf* m;

				/* cur Must bigger then packet. free packet.*/
				/* 3. */
				list_del(&packet->node);
				m = get_mbuf(packet);
				tr->count -= m->nb_segs;
				rte_pktmbuf_free(m);
			}
		}
		pre = p;
	}
	return 0;
}

int tcp_reassemble_pull(struct tcp_reassemble* tr, struct pkt* pkt)
{
	struct pkt* p;
	struct tcphdr* h;

	h = pkt->l4_hdr;

	if ((p = pull(tr, ntohl(h->seq) + pkt->l5_len)) != NULL)
		return chain(pkt, p);
	return 0;
}

int tcp_reassemble_init(struct tcp_reassemble* tr)
{
	INIT_LIST_HEAD(&tr->list);
	tr->count = 0;
	tr->seq = 0;
	tr->ts = 0;
	
	return 0;
}

int tcp_reassemble_destory(struct tcp_reassemble* tr)
{
	struct list_head* p, *n;
	struct rte_mbuf* m;
	struct pkt* pkt;
	tr->count = 0;
	tr->seq = 0;
	tr->ts = 0;

	list_for_each_safe(p, n, &tr->list) {
		pkt = list_entry(p, struct pkt, node);
		m = get_mbuf(pkt);		
		rte_pktmbuf_free(m);
	}
	return 0;
}

bool tcp_reassemble_timeout(struct tcp_reassemble* tr)
{
	/**/
	if (tr->count > 0)
		assert(!list_empty(&tr->list));
		if (tr->ts > TCP_REASSEMBLE_TS_LIMIT)
			/* TODO: add all of them to free list. */
			return true;
	else
		assert(list_empty(&tr->list));

	return false;
}

