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

#include "list.h"

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

#define PKT_TYPE_NONE 0
#define PKT_TYPE_REASSEMBLED 1
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

	/* ip reassemble */
	uint16_t type;
//	uint16_t num;
//	struct pkt* next;
} __attribute__((packed));

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

#define RE_DECODER_SUCC				0
#define RE_DECODER_DROP				1
#define RE_DECODER_CACHED			2
#define RE_DECODER_FLOWED			3

#endif /*__PKT_H__*/
