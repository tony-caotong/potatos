/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-19
 *
 */

#ifndef __APP_HTTP_H__
#define __APP_HTTP_H__

#include "flow.h"

int session_http_open(struct pkt* pkt, struct flow_item* flow);
int session_http_process(struct pkt* pkt, struct flow_item* flow);
int session_http_close(struct pkt* pkt, struct flow_item* flow);

#endif /*__APP_HTTP_H__*/
