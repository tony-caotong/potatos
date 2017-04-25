/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-18
 *
 */

#ifndef __DECODE_IPV4_H__
#define __DECODE_IPV4_H__

#include "pkt.h"

int decode_ipv4(char* raw, int len, struct pkt* pkt);

#endif /*__DECODE_IPV4_H__*/
