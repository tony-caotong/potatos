/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-25
 *
 */

#include <net/ethernet.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <rte_hash.h>
#include <rte_hash_crc.h>
#include <rte_mempool.h>

#include "config.h"
#include "flow.h"
#include "wedge.h"

struct flow_obj {
	struct rte_hash* hash;
	struct rte_mempool* pool;
	uint32_t iterate;
	uint32_t flow_count;
};

static struct flow_obj Objs[RTE_MAX_LCORE];

int flow_ipv4_create(uint32_t lcore_id)
{
	int r = 0;
	struct rte_hash* h;
	struct rte_mempool* p;
	struct rte_hash_parameters param = {
		.name = NULL,
		.entries = CONFIG_FLOW_IPV4_HASH_ENTRIES,
		.reserved = 0,
		.key_len = sizeof(((struct flow_item*)0)->key),
		.hash_func = rte_hash_crc,
		.hash_func_init_val = 0,
		.socket_id = rte_socket_id(),
		.extra_flag = 0,
	};
	char name[64];

	sprintf(name, "flow_ipv4_hash_table_%d", lcore_id);
	param.name = name;

	if ((h = rte_hash_create(&param)) == NULL) {
		fprintf(stderr, "rte_hash_create failed: %u\n", lcore_id);
		r = -1;
	}
	Objs[lcore_id].hash = h;

	/* NOTICE: one pool per each lcore or one pool per each socket? */
	sprintf(name, "flow_ipv4_pool_%d", lcore_id);
	if ((p = rte_mempool_create(name, CONFIG_FLOW_IPV4_HASH_ENTRIES,
			sizeof(struct flow_item), 0, 0, NULL, NULL, NULL, NULL,
			rte_socket_id(), MEMPOOL_F_SP_PUT | MEMPOOL_F_SC_GET
			| MEMPOOL_F_NO_PHYS_CONTIG)) == NULL) {
		fprintf(stderr, "rte_mempool_create failed: %u\n", lcore_id);
		r = -1;
	}
	Objs[lcore_id].pool = p;
	Objs[lcore_id].iterate = 0;
	Objs[lcore_id].flow_count = 0;
	return r;
}

struct flow_item* flow_ipv4_new(uint32_t lcore_id, struct ipv4_key* key)
{
	struct rte_hash* h;
	struct rte_mempool* p;
	struct flow_item* item;

	fprintf(stderr, "Go in flow_ipv4_new()\n");
	h = Objs[lcore_id].hash;
	p = Objs[lcore_id].pool;
	if (rte_mempool_get(p, (void**)(&item)) < 0) {
		// TODO DEBUG.
		return NULL;
	}
	memcpy(&(item->key), key, sizeof(((struct flow_item*)0)->key));
	flow_touch(item);
	INIT_LIST_HEAD(&item->upstream);
	INIT_LIST_HEAD(&item->downstream);
	item->up_node_count = 0;
	item->down_node_count = 0;

	if (rte_hash_add_key_data(h, &(item->key), item) < 0) {
		rte_mempool_put(p, (void**)(&item));
		return NULL;
	}

	Objs[lcore_id].flow_count++;
	return item;
}

int flow_ipv4_del(uint32_t lcore_id, struct flow_item* item)
{
	struct rte_hash* h;
	struct rte_mempool* p;
	int r;

	if (item == NULL)
		return -1;

	h = Objs[lcore_id].hash;
	p = Objs[lcore_id].pool;
	if ((r = rte_hash_del_key(h, &(item->key))) < 0) {
		if (r == -EINVAL)
			return -1;
		else if (r == -ENOENT) {
			fprintf(stderr, "DEBUG: flow_ipv4_del(): unfound\n");
		}
	}
	rte_mempool_put(p, item);
	return 0;
}

static int generate_ipv4_key(uint32_t sip, uint16_t sport, uint32_t dip,
	uint16_t dport, uint8_t protocol, struct ipv4_key* key)
{
	uint8_t reversed = 0;
	uint64_t s, d;
	uint64_t ip1, port1, ip2, port2;

	s = sip;
	s = (s << 16) + sport;
	d = dip;
	d = (d << 16) + dport;
	fprintf(stderr, "DEBUG: gen_key: s: %lu d: %lu\n", s, d);
	if (s <= d) {
		reversed = 0;
		ip1 = sip;
		port1 = sport;
		ip2 = dip;
		port2 = dport;
	} else {
		reversed = 1;
		ip1 = dip;
		port1 = dport;
		ip2 = sip;
		port2 = sport;
	}
	key->part1 = (ip1 << 32) + (port1 << 16) + (ip2 >> 16);
	key->part2 = (ip2 << 48) + (port2 << 32) + protocol;
	return reversed;
}

struct flow_item* flow_ipv4_find_or_add(uint32_t lcore_id,
	uint32_t sip, uint16_t sport, uint32_t dip, uint16_t dport,
	uint8_t protocol, struct pkt* pkt, uint8_t direction)
{
	int r;
	struct rte_hash* h;
	struct flow_item* item;
	struct ipv4_key key;
	uint8_t reversed;
	struct list_head* list;

	h = Objs[lcore_id].hash;

	/* adjust key order. */
	reversed = generate_ipv4_key(sip, sport, dip, dport, protocol, &key);

	fprintf(stderr, "Go in flow_ipv4_find_or_add()\n");
	r = rte_hash_lookup_data(h, &key, (void**)(&item));
	if (r == -EINVAL)
		return NULL;
	else if (r == -ENOENT) {
		item = flow_ipv4_new(lcore_id, &key);
		item->protocol = protocol;
		item->key_salt = direction ^ reversed;
		if (direction == FLOW_DIR_UP) {
			item->client_ip = sip;
			item->client_port = sport;
			item->server_ip = dip;
			item->server_port = dport;
		}
		else { /* FLOW_DIR_DOWN */
			item->client_ip = dip;
			item->client_port = dport;
			item->server_ip = sip;
			item->server_port = sport;
		}
	}

	/* Things about: direction / reversed / stream.
	 *	1. upstream means: those packets were sent from client to
	 *		server.
	 *	   downstream means: sent from server to client.
	 *	2. our Key was ordered by ip+port, so each packet has a 
	 *		featrue that is 'original' or 'reversed'.
	 *	3. the orignal packet was sorted in the 'direction' named
	 *		stream.
	 *	4. the variable 'dir_salt' was sortd the state at the begining
	 *		of flow created.
	 */
	reversed = item->key_salt ^ reversed;
	if (reversed) {
		list = &item->downstream;
		item->down_node_count++;
	} else {
		list = &item->upstream;
		item->up_node_count++;
	}
	list_add_tail(&pkt->node, list);
	return item;
}

int flow_ipv4_timeout(uint32_t lcore_id, uint32_t each, flow_timeout_cb cb)
{
	struct ipv4_key* key;
	struct flow_item* flow;
	struct rte_hash* h;
//	struct rte_mempool* p;
	uint32_t iterate;
	int i, r, count;
	time_t cur;

	h = Objs[lcore_id].hash;
//	p = Objs[lcore_id].pool;
	iterate = Objs[lcore_id].iterate;
	cur = time(NULL);
	count = 0;

	fprintf(stderr, "Go in flow_ipv4_timeout()\n");
	for (i = 0; i < each; i++) {
		if ((r = rte_hash_iterate(h, (const void**)(&key),
				(void**)(&flow), &iterate)) < 0) {
			if (r == -EINVAL)
				return -1;
			else if (r == -ENOENT) {
				iterate = 0;
				break;
			}
		}
		if ((cur - flow->atime) > CONFIG_FLOW_IPV4_TIMEOUT_SECS) {
			fprintf(stderr, "flow_ipv4_timeout(): time out.\n");
			cb(lcore_id, flow);
			count++;
		}
	}
	Objs[lcore_id].iterate = iterate;
	return count;
}

int flow_ipv4_destroy(uint32_t lcore_id)
{
	struct rte_hash* h;
	struct rte_mempool* p;

	h = Objs[lcore_id].hash;
	rte_hash_free(h);
	p = Objs[lcore_id].pool;
	rte_mempool_free(p);

	return 0;
}

void flow_ipv4_state(uint32_t lcore_id)
{
	uint32_t it = Objs[lcore_id].iterate;
	uint32_t flow_count = Objs[lcore_id].flow_count;
	struct rte_hash* h = Objs[lcore_id].hash;
	size_t count, pkt_count;

	fprintf(stderr, "FLOW OBJ: lcore_id: %u\n", lcore_id);
	fprintf(stderr, "FLOW OBJ: iterate: %u\n", it);
	fprintf(stderr, "FLOW OBJ: flow count: %u\n", flow_count);

	it = 0;
	count = 0;
	pkt_count = 0;
	while (1) {
		struct ipv4_key* key;
		struct flow_item* flow;
		struct in_addr ias;
		struct in_addr iad;
		int r;
		struct list_head* i;

		if ((r = rte_hash_iterate(h, (const void**)(&key),
				(void**)(&flow), &it)) < 0) {
			if (r == -EINVAL) {
				fprintf(stderr, "rte_hash_iterate "
					"ERROR: %d\n", r);
				return;
			} else if (r == -ENOENT) {
				break;
			}
		}
	
		ias.s_addr = flow->client_ip;
		fprintf(stderr, "FLOW: client ip: %s\n", inet_ntoa(ias));
		fprintf(stderr, "FLOW: client port: %u\n",
			ntohs(flow->client_port));
		iad.s_addr = flow->server_ip;
		fprintf(stderr, "FLOW: server ip: %s\n", inet_ntoa(iad));
		fprintf(stderr, "FLOW: server port: %u\n",
			ntohs(flow->server_port));
		fprintf(stderr, "FLOW: protocol: %u\n", flow->protocol);
		fprintf(stderr, "FLOW: atime: %lu\n", flow->atime);
		fprintf(stderr, "FLOW: key: 0x%016lx%016lx  salt: %u\n",
			flow->key.part1, flow->key.part2, flow->key_salt);
		count++;
		fprintf(stderr, "FLOW: up node count: %u\n",
			flow->up_node_count);
		list_for_each(i, &(flow->upstream)) {
			struct pkt* p = list_entry(i, struct pkt, node);

			ias.s_addr = p->tuple5.sip;
			iad.s_addr = p->tuple5.dip;
			fprintf(stderr, "\t[%s:%u]->[%s:%u][%u]\n",
				inet_ntoa(ias), ntohs(p->tuple5.sport),
				inet_ntoa(iad), ntohs(p->tuple5.dport),
				p->tuple5.l4_proto);
			pkt_count++;
		}
		fprintf(stderr, "FLOW: down node count: %u\n",
			flow->down_node_count);
		list_for_each(i, &(flow->downstream)) {
			struct pkt* p = list_entry(i, struct pkt, node);

			ias.s_addr = p->tuple5.sip;
			iad.s_addr = p->tuple5.dip;
			fprintf(stderr, "\t[%s:%u]->[%s:%u][%u]\n",
				inet_ntoa(ias), ntohs(p->tuple5.sport),
				inet_ntoa(iad), ntohs(p->tuple5.dport),
				p->tuple5.l4_proto);
			pkt_count++;
		}
	}
	fprintf(stderr, "FLOW OBJ: total flow found: %zu"
		" total pkts found: %zu\n", count, pkt_count);
}

static int flow_ipv4_reassemble(struct pkt* pkt, struct flow_item** flowp)
{
	struct wedge_dpdk* wedge;
	uint32_t lcore_id;
	uint8_t protocol;
	uint32_t sip, dip;
	uint16_t sport, dport;
	struct flow_item* flow;
	uint8_t direction;
	
	wedge = pkt->platform_wedge;
	lcore_id = wedge->lcore_id;
	sip = pkt->tuple5.sip;
	dip = pkt->tuple5.dip;
	protocol = pkt->tuple5.l4_proto;

	fprintf(stderr, "Go in flow_ipv4_reassemble()\n");
	/* 1. decide 5 tuples or 3 tuples. */
	if (protocol == IPPROTO_TCP || protocol == IPPROTO_UDP) {
		sport = pkt->tuple5.sport;
		dport = pkt->tuple5.dport;
	} else {
		/* both '0' in sport and dport means it's a 3 tuples flow.*/
		sport = 0;
		dport = 0;
	}
	
	/* 2. skip. means we do not care of this pack. */
	// TODO
	
	/* 3. direction determinating. */
	// TODO
	direction = 0;

	/* 4. find which flow belonged to or create a new one. */
	flow = flow_ipv4_find_or_add(lcore_id, sip, sport,
			dip, dport, protocol, pkt, direction);
	if (flow == NULL)
		return -1;
	*flowp = flow;
	return 0;
}

int flow_pkt(struct pkt* pkt, struct flow_item** flowp)
{
	uint16_t l3_proto;
	int r = 0;

	l3_proto = pkt->l3_proto;

	fprintf(stderr, "Go in flow_pkt()\n");

	switch (l3_proto) {
	case ETHERTYPE_IP:
		r = flow_ipv4_reassemble(pkt, flowp);
		break;
	case ETHERTYPE_IPV6:
		/*r = decode_ipv6(p, l, pkt);*/
		break;
	case ETHERTYPE_ARP:
		break;
	default:
		break;
	}
	return r;
}

