/**
 * 	Masked potato salad.
 *		by Cao tong <tony_caotong@gmail.com>
 *		@ 2017-02-10
 *
 */

#ifndef __DECODER_H__
#define __DECODER_H__

typedef struct decoder_t DECODER;

#ifdef __cplusplus
extern "C" {
#endif

int hally_hdrlen();
DECODER* init_decoder();
int decode_pkt(DECODER* dec, void* priv, void* buf, int len);
void destory_decoder(DECODER* dec);

#ifdef __cplusplus
}
#endif

#endif /*__DECODER_H__*/
