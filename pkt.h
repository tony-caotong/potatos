/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-21
 *
 */

#ifndef __PKT_H__
#define __PKT_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include <rte_mbuf.h>

#include "list.h"
#include "wedge.h"

#define STACK_SIZE 16

struct protocol_stack_item {
	uint16_t protocol_type;
	void* protocol_hdr;
} __attribute__((packed));

typedef struct protocol_stack_item protocol_stack[STACK_SIZE];

struct ipv4_5tuple{
	uint32_t sip;
	uint32_t dip;
	uint16_t sport;
	uint16_t dport;
	uint8_t l4_proto;
};

#define PKT_TYPE_NONE 0x0000
#define PKT_TYPE_REASSEMBLED 0x0001
#define PKT_TYPE_DULPLICATE_L5DATA 0x0002
struct pkt {
	struct list_head node;
	/* stack */
	uint8_t index;
	protocol_stack stack;
	uint16_t l3_proto;
	struct ipv4_5tuple tuple5;
//	uint8_t l5_proto;

	/* timestamp */
	time_t ctime;

	/* platform things */
	void* platform_wedge;
	void* mbuf;

	/* ip reassemble */
	uint16_t type;
//	uint16_t num;
//	struct pkt* next;

	void* l3_hdr;
	void* l4_hdr;
	void* l5_hdr;
	uint64_t l5_len;
	/* valid only when 'PKT_TYPE_DULPLICATE_L5DATA' was set. */
	void* app_begin;

} __attribute__((packed));

static inline struct rte_mbuf* get_mbuf(struct pkt* pkt)
{
	struct rte_mbuf* m;
	struct wedge_dpdk* w;

	w = (struct wedge_dpdk*)(pkt->platform_wedge);
	m = w->buf;

	return m;
}

#define add_proto_item(item, string) item
enum proto_t {
	PROTO_NONE,
#include "proto.h"
	PROTO_SIZE
};
#undef add_proto_item

#define add_proto_item(item, string) string
static const char* proto_str[] __attribute__((unused)) = {
	"PROTO_NONE",
#include "proto.h"
	"PROTO_SIZE"
};
#undef additem

/**
 * Ethernet VLAN Header.
 * Contains the 16-bit VLAN Tag Control Identifier and the Ethernet type
 * of the encapsulated frame.
 */
struct vlan_header {
	uint16_t vlan_tci; /**< Priority (3) + CFI (1) + Identifier Code (12) */
	uint16_t eth_proto;/**< Ethernet type of encapsulated frame. */
} __attribute__((__packed__));

static inline bool proto_push(struct pkt* pkt, void* p, enum proto_t protocol) 
{
	if (pkt->index >= STACK_SIZE)
		return false;
	pkt->stack[pkt->index].protocol_hdr = p;
	pkt->stack[pkt->index].protocol_type = protocol;
	pkt->index++;
	return true;
}

#define RE_STREAM_TAINTED		-2
#define RE_COMMON_ERR			-1
#define RE_PKT_SUCC			0
#define RE_PKT_DROP			1
#define RE_PKT_CACHED			2
#define RE_STREAM_CLOSED		3

#endif /*__PKT_H__*/
