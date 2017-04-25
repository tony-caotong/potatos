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
	/* we use port nunmber 1/2 to indicate black/white. */
	uint8_t flag;
};

int filter_compile(const char* str, struct filter_ipv4_rule rules[],
		size_t size);

/*
 * filter_init():
 *	1. This init function is not thread-safe, as all cores under one sock
 *	will share the same lpm object. Must use it in master-lcore then
 *	query could be invoked in each processing lcores. 
 *	2. Function is reentrant.
 */
int filter_init(struct filter_ipv4_rule* rules, size_t size, uint32_t sockid);
int is_filter_ipv4_pass(struct iphdr* iph, uint32_t sockid);
int filter_destory(uint32_t sockid);

#endif /*__FILTER_H__*/
