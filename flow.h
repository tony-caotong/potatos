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

#define FLOW_NORTH 0
#define FLOW_SOUTH 1
#define FLOW_ORIENT_UNKNOWN 255

struct ipv4_key {
	uint64_t part1;
	uint64_t part2;
};

/**
 *	            North
 *	        ------------>
 *	W(small)  clockwist  E(big)
 *	        <------------
 *	            South
 *	We use IP+PORT to decide smaller or bigger.
 */
struct flow_item {
	/* things about key. */
	struct ipv4_key key;
	uint8_t protocol;
	uint32_t wip;
	uint32_t eip;
	uint16_t wport;
	uint16_t eport;

	time_t atime;
	uint32_t ncount;
	uint32_t scount;

	/* external data. */
	struct stream_tcp* stream;
}__attribute__((packed));

typedef void (*flow_timeout_cb)(uint32_t lcore_id, struct flow_item* flow);

int flow_pkt(struct pkt* pkt, struct flow_item** flowp);
int flow_ipv4_del(uint32_t lcore_id, struct flow_item* item);

int flow_ipv4_create(uint32_t lcore_id);
struct flow_item* flow_ipv4_find_or_add(uint32_t lcore_id,
	uint32_t sip, uint16_t sport, uint32_t dip, uint16_t dport,
	uint8_t protocol, struct pkt* pkt);
int flow_ipv4_timeout(uint32_t lcore_id, uint32_t each, flow_timeout_cb cb);
int flow_ipv4_destroy(uint32_t lcore_id);
void flow_ipv4_state(uint32_t lcore_id);
int flow_orient_decide(struct pkt* pkt, struct flow_item* flow);

static inline void flow_touch(struct flow_item* flow) 
{
	flow->atime = time(NULL);
}

#endif /*__FLOW_H__*/
