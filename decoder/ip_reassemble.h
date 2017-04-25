/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-20
 *
 */

#ifndef __IP_REASSEMBLE_H__
#define __IP_REASSEMBLE_H__

#include <stdint.h>

#include "pkt.h"

#define IPV4_DF(frag_off) (!!(frag_off&~IP_DF))
#define IPV4_MF(frag_off) (!!(frag_off&~IP_MF))
#define IPV4_F_OFF(frag_off) ((frag_off&IP_OFFMASK))
#define IPV4_IS_FRAG(frag_off) (!!(frag_off&(IP_MF|IP_OFFMASK)))

int ipv4_reassemble_init(uint32_t lcore_id);
void ipv4_reassemble_dpdk_death_row_free(uint32_t lcore_id);
int ipv4_reassemble_destroy(uint32_t lcore_id);
char* ipv4_reassemble(char* raw, unsigned len, char** out, unsigned* ol,
		struct pkt* pkt);

#endif /*__IP_REASSEMBLE_H__*/
