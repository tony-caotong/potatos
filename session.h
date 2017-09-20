/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-19
 *
 */

#ifndef __APP_STORE_H__
#define __APP_STORE_H__

#include "pkt.h"
#include "flow.h"

/*
	We have a session type called 'transparent', that means
	emit all datas to app layer.
*/
enum session_type {
	TYPE_SESSION_NONE = 0,
	TYPE_SESSION_HTTP,
	TYPE_SESSION_TRANSPARENT,
	TYPE_SESSION_MAX,
};

int session_init(uint32_t lcore_id);
int session_pkt(struct pkt* pkt, struct flow_item* flow,
	char** data, int* len);
int session_destroy(uint32_t lcore_id);

#endif /*__APP_STORE_H__*/
