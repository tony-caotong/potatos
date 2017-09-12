/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-08
 *
 */

#include "list.h"
#include "tcp_reassemble.h"

int tcp_reassemble_init(struct tcp_reassemble* tr)
{
	INIT_LIST_HEAD(tr->list);
	tr->count = 0;
	tr->seq = 0;
	tr->ts = 0;
	tr->ready =NULL;
	
	return 0;
}

/*
 *	Better to learn from ipv4_frag_reassemble():
 *		dpdk_root/lib/librte_ip_frag/rte_ipv4_reassembly.c 
 */
static int chain()
{
	rte_pktmbuf_chain
	rte_pktmbuf_adj
}

static int reassemble(struct list_head, uint32_t seq, struct mbuf** out)
{

}

static void seq(struct pkt* pkt, uint32_t* seq, uint32_t* nseq)
{
	struct tcphdr* h;

	/* TODO: check if this packet is a merged one. */
	h = pkt->l4_hdr;
	*seq = ntohl(h->seq);
	*nseq = *seq + pkt->l5_len;
}

/*
 *	We consider the pkt goin there which is already compared to the channel
 *	sequence, so unnecessary to call reassemble(), just insert.
 */
int tcp_reassemble_push(struct tcp_reassemble* tr, struct pkt* pkt)
{
	uint32_t seq, nseq;
	struct list_head* p, *pre, *cur, *post;

	if (tr->count >= TCP_REASSEMBLE_COUNT_LIMIT)
		return -1;

	/* update ts. */
	tr->ts = time();
	seq(pkt, &seq, &nseq);

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
	post = NULL;
	list_for_each(p, tr->list) {
		uint32_t s, n;
		struct pkt* packet;
		
		packet = list_entry(p);
		seq(packet, &s, &n);
		if (!post) {/*A*/
			if (seq < s) {
				post = packet;
				continue;
			}
			pre = packet;
		} else { /*B*/
			/* deal with left position. */
			if (pre) {
				uint32_t pre_s, pre_n;
				seq(pre, &pre_s, &pre_n);
				if (seq <= pre_n) {
					if (nseq <= pre_n) {
						/* 5 */
						/*TODO duplicata */
						break;
					}
					/* 4,6,10 */
					cur = chain(pre, cur);
				} else 
					/* 1,2,3,7,8,9 */
					insert(cur, pre);
			} else { /* insert first. */
				insert(cur, tr->list);
			}

			/* deal with right position. */
			
			/* a. free any node who need to be freed. */
		}
	}
}

void* tcp_reassemble_pull(struct tcp_reassemble* tr, uint32_t seq)
{
	/* update ts, if success pull. */
}

int tcp_reassemble_destory(struct tcp_reassemble* tr)
{

}

bool tcp_reassemble_timeout(struct tcp_reassemble* tr)
{
	/* tr->ts > TS_LIMIT. && tr->count > 0. */
}

