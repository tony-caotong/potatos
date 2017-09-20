/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-19
 *
 */

#ifndef __DETECT_H__
#define __DETECT_H__

int detect_flow(uint32_t sip, uint16_t sport, uint32_t dip, uint16_t dport,
	uint8_t protocol, uint32_t* session_type);

#endif /*__DETECT_H__*/
