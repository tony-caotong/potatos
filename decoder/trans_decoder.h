/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-02-10
 *
 */

#ifndef __TRANS_DECODER_H__
#define __TRANS_DECODER_H__

#include "pkt.h"
int decode_pkt(char* raw, int len, struct pkt* pkt);

#endif /*__TRANS_DECODER_H__*/
