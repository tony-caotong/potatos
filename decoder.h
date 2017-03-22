/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-02-10
 *
 */

#ifndef __DECODER_H__
#define __DECODER_H__

#include "priv.h"

#define STACK_SIZE 8

struct protocol_stack_item {
	uint32_t protocol_type;
	void* protocol_hdr;
};

typedef struct protocol_stack_item protocol_stack[STACK_SIZE];

struct pkt {
	uint32_t index;
	protocol_stack stack;
};



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

int decode_pkt(struct pkt* pkt, char* raw, int len, struct priv* priv);

#endif /*__DECODER_H__*/
