/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-26
 *
 */

#ifndef __DECODE_UDP_H__
#define __DECODE_UDP_H__

#include "pkt.h" 

int decode_udp(char* raw, uint32_t len, struct pkt* pkt);

#endif /*__DECODE_UDP_H__*/
