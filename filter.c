/**
 *	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-03-27
 *
 */

#include <stdbool.h>
#include <rte_lpm.h>
#include "filter.h"

#define NB_SOCKETS 8
#define IPV4_L3FWD_LPM_MAX_RULES 32
#define IPV4_L3FWD_LPM_NUMBER_TBL8S (1<<8)

struct rule_box {
	struct rte_lpm* white_list;
	struct rte_lpm* black_list;
};

static struct rule_box rule_box[NB_SOCKETS];

static int lpm_alloc(struct rte_lpm** lpmp, uint8_t flag, uint32_t sockid)
{
	struct rte_lpm_config cfg;
	char name[64];
	struct rte_lpm* lpm;

	cfg.max_rules = IPV4_L3FWD_LPM_MAX_RULES;
	cfg.number_tbl8s = IPV4_L3FWD_LPM_NUMBER_TBL8S;

	if (flag == FILTER_DEFAULT_PASS)
		snprintf(name, sizeof(name), "IPV4_WHITE_LPM_%d", sockid);
	else if (flag == FILTER_DEFAULT_DROP)
		snprintf(name, sizeof(name), "IPV4_BLACK_LPM_%d", sockid);
	else
		return -1;

	lpm = rte_lpm_create(name, sockid, &cfg);
	if (lpm == NULL)
		return -1;
	*lpmp = lpm;

	return 0;
}

/*
 *	Default:
 *		A: all people is in White List.
 *		B: No one is in Black list.
 *	if (true == (!item_in_black(it) && item_in_white(it)))
 *		passed;
 *	else
 *		droped;
 *
 *	Implement:
 *		One LPM table can not finish this logic, we MUST use
 *		two of them.
 */
int filter_init(struct filter_ipv4_rule* rules, size_t size, uint32_t sockid)
{
	struct rule_box* b;
	int i;

	/* We do not initialize LPM table here, insteading of let them NULL,
		so if there is no white/block rules, we can easily skip that
		checking.
	*/
	b = &(rule_box[sockid]);
	b->white_list = NULL;
	b->black_list = NULL;

	/* populate the LPM table */
	for (i = 0; i < size; i++) {
		struct rte_lpm* lpm;
		struct filter_ipv4_rule* r = rules + i;

		if (r->flag == FILTER_DEFAULT_PASS) {
			if (b->white_list == NULL) {
				/* create the white LPM table */
				if (lpm_alloc(&(b->white_list), r->flag,
						sockid) < 0)
					goto err;
			}
			lpm = b->white_list;
		} else if (r->flag == FILTER_DEFAULT_DROP) {
			if (b->black_list == NULL) {
				/* create the black LPM table */
				if (lpm_alloc(&(b->black_list), r->flag,
						sockid) < 0)
					goto err;
			}
			lpm = b->black_list;
		} else {
			continue;
		}

		if (rte_lpm_add(lpm, r->ip, r->depth, r->flag) < 0) {
			/*TODO: DEBUG*/
		}
		/*TODO: INFO*/
	}
	return 0;
err:
	return -1;
}

int filter_destory(uint32_t sockid)
{
	struct rule_box* b = &(rule_box[sockid]);

	if (b->white_list) {
		rte_lpm_free(b->white_list);
		b->white_list = NULL;
	}
	if (b->black_list) {
		rte_lpm_free(b->black_list);
		b->black_list = NULL;
	}
	return 0;
}

int is_filter_ipv4_pass(struct iphdr* iph, uint32_t sockid)
{
	uint32_t saddr, daddr;
	bool is_white, is_black;
	/* next_hop is flag. */
	uint32_t next_hop;
	struct rule_box* b = &(rule_box[sockid]);
	int r; 

	saddr = ntohl(iph->saddr);
	daddr = ntohl(iph->daddr);
	
	is_white = true;
	is_black = false;

	/* 1. handle white list, if both of saddr and daddr are not in
		list, `is_white` will be set false; if white_list is NULL
		PASSed.
	*/
	if (b->white_list) {
		r = rte_lpm_lookup(b->white_list, saddr, &next_hop);
		if (r == -EINVAL)
			abort();
		 /*found, and whatever next_hop is. */
		if (r != 0)
			is_white = false;
		r = rte_lpm_lookup(b->white_list, daddr, &next_hop);
		if (r == -EINVAL)
			abort();
		 /*found, and whatever next_hop is. */
		if (r != 0)
			is_white |= false;
	}

	/* 2. handle black list, either saddr or daddr was blacked,
		this packet was blacked.
	*/
	if (b->black_list) {
		r = rte_lpm_lookup(b->black_list, saddr, &next_hop);
		if (r == -EINVAL)
			abort();
		if (r == 0)
			is_black = true;
		r = rte_lpm_lookup(b->black_list, daddr, &next_hop);
		if (r == -EINVAL)
			abort();
		if (r == 0)
			is_black = true;
	}

	/* 3. combine black and white. */
	return (!is_black && is_white);
}

