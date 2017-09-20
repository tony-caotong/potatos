/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-19
 *
 */

#include "config.h"
#include "session.h"

int detect_flow(uint32_t sip, uint16_t sport, uint32_t dip, uint16_t dport,
	uint8_t protocol, uint32_t* session_type)
{
	uint32_t t, r;

	r = -1;
	t = TYPE_SESSION_NONE;

#ifdef DEBUG
	t = TYPE_SESSION_TRANSPARENT;
	r = t;
#else
	if (sport == 80 or dport == 80) {
		t = TYPE_SESSION_HTTP;
		r = t;
	}
#endif
	*session_type = t;
	return r;
}
