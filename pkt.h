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

#define STACK_SIZE 16

struct protocol_stack_item {
	uint16_t protocol_type;
	void* protocol_hdr;
} __attribute__((packed));

typedef struct protocol_stack_item protocol_stack[STACK_SIZE];

struct pkt {
	uint8_t index;
	protocol_stack stack;
	/* timestamp */
	void* platform_wedge;
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

#define RE_SUCC				0
#define RE_PASS	RE_SUCC
#define RE_DROP				1
#define RE_IPV4_FRAGMENT 		2

#endif /*__PKT_H__*/
