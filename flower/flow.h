/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-25
 *
 */

#ifndef __FLOW_H__
#define __FLOW_H__

#include <time.h>
#include <assert.h>

#include "pkt.h"

#define FLOW_DIR_UP	0
#define FLOW_DIR_DOWN	1

struct ipv4_key {
	uint64_t part1;
	uint64_t part2;
};

struct flow_item {
	struct ipv4_key key;
	/* It is value of direction ^ (reversed value of first pkt) */
	uint8_t key_salt;

	struct list_head upstream;	/* client to server. */
	struct list_head downstream;	/* server to client. */
	uint32_t client_ip;
	uint32_t server_ip;
	uint16_t client_port;
	uint16_t server_port;
	uint8_t protocol;
	uint32_t up_node_count;
	uint32_t down_node_count;

	time_t atime;
}__attribute__((packed));

typedef void (*flow_timeout_cb)(uint32_t lcore_id, struct flow_item* flow);

int flow_pkt(struct pkt* pkt, struct flow_item** flowp);

int flow_ipv4_create(uint32_t lcore_id);
struct flow_item* flow_ipv4_find_or_add(uint32_t lcore_id,
	uint32_t sip, uint16_t sport, uint32_t dip, uint16_t dport,
	uint8_t protocol, struct pkt* pkt, uint8_t direction);
int flow_ipv4_timeout(uint32_t lcore_id, uint32_t each, flow_timeout_cb cb);
int flow_ipv4_destroy(uint32_t lcore_id);
void flow_ipv4_state(uint32_t lcore_id);

static inline void flow_touch(struct flow_item* flow) 
{
	/* TODO: learn about assert's behavior. */
	assert(time(&(flow->atime)) != EOVERFLOW);
}

#endif /*__FLOW_H__*/
