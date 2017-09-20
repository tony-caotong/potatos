/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-19
 *
 */

#include <stdio.h>

#include "debug.h"
#include "session.h"

#include "session_http.h"

typedef int (*func_session_open)(struct pkt* pkt, struct flow_item* flow);
typedef int (*func_session_process)(struct pkt* pkt, struct flow_item* flow);
typedef int (*func_session_close)(struct pkt* pkt, struct flow_item* flow);

struct session_adapter {
	uint32_t session_type;
	func_session_open open;
	func_session_process process;
	func_session_close close;
};

static struct session_adapter store[];

int session_init(uint32_t lcore_id)
{
	return 0;
}

int session_destroy(uint32_t lcore_id)
{
	return 0;
}

int session_pkt(struct pkt* pkt, struct flow_item* flow, char** data, int* len)
{
	int r;
	uint32_t type;
	func_session_open open;
	func_session_process process;
	func_session_close close;

	type = flow->session_type;

	if (type == TYPE_SESSION_NONE || type >= TYPE_SESSION_MAX)
		return -1;

	open = store[type].open;
	process = store[type].process;
	close = store[type].close;

	if (flow->event == EVENT_OPEN) {
		if (open(pkt, flow) < 0)
			return -1;
	}

	r = process(pkt, flow);

	if (flow->event == EVENT_CLOSE) {
		if (close(pkt, flow) < 0)
			return -1;
	}

	return r;
}

int session_transparent_open(struct pkt* pkt, struct flow_item* flow)
{
	fprintf(stderr, "open....\n");
	return 0;
}

int session_transparent_process(struct pkt* pkt, struct flow_item* flow)
{
	struct rte_mbuf* m;
	int i, segs, len, totlen, totpad;
	char cap[16];
	void* p;
	
	m = pkt->mbuf;
	p = pkt->l5_hdr;
	len = m->data_len - m->l2_len - m->l3_len - m->l4_len - pkt->pad_len;
	segs = m->nb_segs;
	totlen = 0;
	totpad = 0;

	fprintf(stdout, "transparent:\n");
	for (i = 0; i < segs; i++) {
		struct pkt* tmppkt;
		tmppkt = (void*)m + sizeof(struct rte_mbuf);

		sprintf(cap, "seg:[%d]", i);
		totlen += len;
		totpad += tmppkt->pad_len;
		debug_print_binary(cap, p, len);
		if (m->next == NULL)
			break;
		m = m->next;
		p = m->buf_addr + m->data_off;
		len = m->data_len - tmppkt->pad_len;
	}
	m = pkt->mbuf;
	fprintf(stdout, "transparent len: %d l5_len: %lu pkt_len: %d \n"
		"l2: %d, l3: %d, l4: %d totpad: %d \n",
		totlen, pkt->l5_len, m->pkt_len, m->l2_len,
		m->l3_len, m->l4_len, totpad);
	assert(totlen == (m->pkt_len - m->l2_len
		- m->l3_len - m->l4_len - totpad));
	return 0;
}

int session_transparent_close(struct pkt* pkt, struct flow_item* flow)
{
	fprintf(stderr, "close....\n");
	fprintf(stdout, "EOF\n");
	return 0;
}

static struct session_adapter store[TYPE_SESSION_MAX] = {
	{TYPE_SESSION_NONE, NULL, NULL, NULL},
	{TYPE_SESSION_HTTP,
		session_http_open, session_http_process, session_http_close},
	{TYPE_SESSION_TRANSPARENT,
		session_transparent_open,
		session_transparent_process,
		session_transparent_close},
};

