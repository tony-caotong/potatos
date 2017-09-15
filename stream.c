/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-14
 *
 */

#include <net/ethernet.h>
#include <netinet/in.h>

#include <rte_mempool.h>

#include "config.h"
#include "wedge.h"
#include "stream.h"
#include "stream_tcp.h"

struct stream_obj {
	struct rte_mempool* pool;
	uint64_t stream_count;
};

static struct stream_obj Objs[RTE_MAX_LCORE];

int stream_create(uint32_t lcore_id)
{
	struct rte_mempool* p;
	char name[64];

	sprintf(name, "stream_pool_%d", lcore_id);
	if ((p = rte_mempool_create(name, CONFIG_TCP_STREAM_ENTRIES,
		sizeof(struct stream_tcp), 0, 0, NULL, NULL, NULL, NULL,
		rte_socket_id(), MEMPOOL_F_SP_PUT | MEMPOOL_F_SC_GET
			)) == NULL) {
		return -1;
	}

	Objs[lcore_id].pool = p;
	Objs[lcore_id].stream_count = 0;

	return 0;
}

int stream_destroy(uint32_t lcore_id)
{
	struct rte_mempool* p;
	
	p = Objs[lcore_id].pool;
	rte_mempool_free(p);
	return 0;
}

int stream_pkt(struct pkt* pkt, struct flow_item* flow)
{
	int r;
	uint16_t l3_proto;
	uint8_t l4_proto;
	struct rte_mempool* p;
	struct stream_tcp* s;
	struct wedge_dpdk* wedge;
	uint32_t lcore_id;
	int32_t pkt_orient;

	wedge = pkt->platform_wedge;
	lcore_id = wedge->lcore_id;

	r = 0;
	l3_proto = pkt->l3_proto;
	l4_proto = pkt->tuple5.l4_proto;
	if (!flow || l3_proto != ETHERTYPE_IP || !l4_proto == IPPROTO_TCP) {
		return r;
	}

	p = Objs[lcore_id].pool;
	s = flow->stream;

	if (!s) {
		if (rte_mempool_get(p, (void**)(&s)) < 0) {
			return -1;
		}
		stream_tcp_init(s);
		flow->stream = s;
	}

	if (s->flags & STREAM_TCP_FLAG_INVALID)
		return RE_STREAM_TAINTED;

	pkt_orient = flow_orient_decide(pkt, flow);
	r = stream_tcp_pkt(pkt, pkt_orient, s);
	if (r == RE_STREAM_CLOSED || r < 0) {
		stream_tcp_destroy(s);
		rte_mempool_put(p, s);
		flow->stream = NULL;
	}
	return r;
}

