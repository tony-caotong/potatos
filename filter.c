/**
 *	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-03-27
 *
 */

#include <stdbool.h>
#include <string.h>
#include <arpa/inet.h>

#include <rte_lpm.h>
#include <rte_ip.h>
#include "filter.h"

#define NB_SOCKETS 8
#define IPV4_L3FWD_LPM_MAX_RULES 32
#define IPV4_L3FWD_LPM_NUMBER_TBL8S (1<<8)

struct rule_box {
	struct rte_lpm* white_list;
	struct rte_lpm* black_list;
	int8_t is_init;
};

static struct rule_box rule_box[NB_SOCKETS];

#define VSIZE 5
#define MASKX (VSIZE - 1)
static int format_ipv4(const char* str, struct filter_ipv4_rule* rule)
{
	size_t n, i, j;
	const char *p = str;
	char *p1, *q, *save, *sub;
	int tmp;
	unsigned v[VSIZE];
	int f;

	/* 1. trim and dup. */
	while (isspace(*p))
		p++;
	n = strlen(p);
	while (n > 0 && isspace(p[n-1]))
		n--;
	if (n == 0)
		return 0;

	/* 2. white or black. */
	f = FILTER_DEFAULT_PASS;
	if (p[0] == '!') {
		f = FILTER_DEFAULT_DROP;
		p++;
		n--;
	}

	/* 1.1 yes! it is dup. */
	if ((q = strndup(p, n)) == NULL)
		return -1;

	/* 3. split and check.*/
	for (p1 = q, i = 0; ; p1 = NULL, i++) {
		sub = strtok_r(p1, "/.",  &save);
		if (sub == NULL)
			break;
		for (j = 0, tmp = 0; j < strlen(sub); j++) {
			if (!isdigit(sub[j]))
				goto err;
			tmp = tmp * 10 + (sub[j] - '0');
		}
		if (i >= VSIZE)
			goto err;
		if (i == MASKX && *(p + (n-strlen(sub)-1)) != '/')
			/* invaild token. */
			goto err;
		if (i == MASKX && tmp > 32)
			/* invaild mask. */
			goto err;
		if (tmp > 255)
			/* invaild mask. */
			goto err;
		v[i] = tmp;
	}
	if (i != VSIZE)
		goto err;

	/* 4. success and return. */
	rule->ip = IPv4(v[0], v[1], v[2], v[3]);
	rule->depth = v[MASKX];
	rule->flag = f;

	/* 1.99 don't forget to release. */
	free(q);
	return 1;
err:
	free(q);
	return -1;
}

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

int filter_compile(const char* str, struct filter_ipv4_rule rules[],
		size_t size) {
	char *p, *substr, *saveptr = NULL;
	int count, r;
	struct filter_ipv4_rule* rule;

	count = 0;
	rule = rules;
	for (p = (char*)str; ; p = NULL) {
		substr = strtok_r(p, ",", &saveptr);
		if (substr == NULL)
			break;
		if (count >= size)
			/* "filter items are too more." */
			return -1;
		if ((r = format_ipv4(substr, rule + count)) >= 0)
			count+=r;
		else
			/* "Invalid filter string." */
			return -1;
	}
	return count;
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
	int i;
	struct rule_box* b;
	struct filter_ipv4_rule* r;
	struct rte_lpm** lpmp;

	b = &(rule_box[sockid]);
	if (b->is_init)
		/* It's not a error, we support reentrant. */
		return 0;
	/* We do not initialize LPM table here, insteading of let them NULL,
		so if there is no white/block rules, we can easily skip that
		checking.
	*/
	b->is_init = true;
	b->white_list = NULL;
	b->black_list = NULL;

	/* populate the LPM table */
	for (i = 0; i < size; i++) {
		r = rules + i;

		if (r->flag == FILTER_DEFAULT_PASS)
			lpmp = &(b->white_list);
		else if (r->flag == FILTER_DEFAULT_DROP)
			lpmp = &(b->black_list);
		else
			goto err;

		/* create LPM table when first use. */
		if (*lpmp == NULL)
			if (lpm_alloc(lpmp, r->flag, sockid) < 0)
				goto err;
		if (rte_lpm_add(*lpmp, r->ip, r->depth, r->flag) < 0)
			goto err;
#if 1
		struct in_addr in;
		in.s_addr = htonl(r->ip);
		printf("Add rule: %s/%d %s\n", inet_ntoa(in), r->depth, 
			r->flag == FILTER_DEFAULT_PASS ? "white":"black");
#endif
	}
	return 0;
err:
	filter_destory(sockid);
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
	b->is_init = false;
	return 0;
}

int is_filter_ipv4_pass(struct iphdr* iph, uint32_t sockid)
{
	uint32_t saddr, daddr;
	bool in_white, in_black;
	/* next_hop is flag. */
	uint32_t next_hop;
	struct rule_box* b = &(rule_box[sockid]);
	int r; 

	saddr = ntohl(iph->saddr);
	daddr = ntohl(iph->daddr);
	
	in_white = false;
	in_black = false;

	/* 1. handle white list, if both of saddr and daddr are not in
		list, `in_white` will be set false; if white_list is NULL
		PASSed.
	*/
	if (b->white_list) {
		r = rte_lpm_lookup(b->white_list, saddr, &next_hop);
		if (r == -EINVAL)
			goto err;
		/*found, and whatever next_hop is. */
		if (r == 0)
			in_white |= true;
		r = rte_lpm_lookup(b->white_list, daddr, &next_hop);
		if (r == -EINVAL)
			goto err;
		/*found, and whatever next_hop is. */
		if (r == 0)
			in_white |= true;
	} else {
		/* NOTE: Empty white list MEANS everything is white. */
		in_white = true;
	}

	/* 2. handle black list, either saddr or daddr was blacked,
		this packet was blacked.
	*/
	if (b->black_list) {
		r = rte_lpm_lookup(b->black_list, saddr, &next_hop);
		if (r == -EINVAL)
			goto err;
		if (r == 0)
			in_black |= true;
		r = rte_lpm_lookup(b->black_list, daddr, &next_hop);
		if (r == -EINVAL)
			goto err;
		if (r == 0)
			in_black |= true;
		/* Empty black list MEANS nothing is black.
	} else {
		in_black = false;
		*/
	}

	/* 3. combine black and white. */
	return (!in_black && in_white);
err:
	abort();
}

