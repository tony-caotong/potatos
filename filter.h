/**
 *	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-03-27
 *
 */

#ifndef __FILTER_H__
#define __FILTER_H__

#include <stdint.h>
#include <netinet/ip.h>

#define FILTER_DEFAULT_PASS 1
#define FILTER_DEFAULT_DROP 2

struct filter_ipv4_rule {
	uint32_t ip;
	uint8_t depth;
	/* we use */
	uint8_t flag;
};

int filter_init(struct filter_ipv4_rule* rules, size_t size, uint32_t sockid);
int is_filter_ipv4_pass(struct iphdr* iph, uint32_t sockid);
int filter_destory(uint32_t sockid);

#endif /*__FILTER_H__*/
