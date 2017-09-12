/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-26
 *
 */

#ifndef __DECODE_TCP_H__
#define __DECODE_TCP_H__

#include "pkt.h" 

int decode_tcp(char* raw, uint32_t len, struct pkt* pkt, uint32_t plen);

#endif /*__DECODE_TCP_H__*/
