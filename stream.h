/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-14
 *
 */

#ifndef __STREAM_H__
#define __STREAM_H__

#include <stdint.h>

#include "pkt.h"
#include "flow.h"

int stream_create(uint32_t lcore_id);
int stream_destroy(uint32_t lcore_id);

int stream_pkt(struct pkt* pkt, struct flow_item* flow);

#endif /*__STREAM_H__*/
