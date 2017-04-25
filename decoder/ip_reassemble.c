/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-04-20
 *
 */

#include <rte_ip_frag.h>

#include "ip_reassemble.h"
#include "config.h"
#include "wedge.h"

struct reassem_obj {
	struct rte_ip_frag_death_row death_row;
	struct rte_ip_frag_tbl* tbl;
} __attribute__((__packed__));

static struct reassem_obj Objs[RTE_MAX_LCORE];

int ipv4_reassemble_init(uint32_t lcore_id)
{
	struct rte_ip_frag_tbl* tbl;
	uint32_t bucket_num;
	uint32_t cell_entries;
	uint64_t max_cycles;

	bucket_num = CONFIG_FLOW_NUM_LIMIT;
	cell_entries = CONFIG_IP_FRAG_NUM_LIMIT;	
	/* Rounding up. */
	max_cycles = (rte_get_tsc_hz() + MS_PER_S -1 ) / MS_PER_S * 
		CONFIG_FLOW_TTL_LIMIT;

	tbl = rte_ip_frag_table_create(bucket_num, cell_entries,
		bucket_num * cell_entries, max_cycles, rte_socket_id());
	if (tbl == NULL) {
		fprintf(stderr, "init err!\n");
		return -1;
	}
	Objs[lcore_id].tbl = tbl;
	return 0;
}

void ipv4_reassemble_dpdk_death_row_free(uint32_t lcore_id)
{
	struct rte_ip_frag_death_row* dr;

	dr = &(Objs[lcore_id].death_row);
	rte_ip_frag_free_death_row(dr, CONFIG_DPDK_PREFETCH_OFFSET);
}

int ipv4_reassemble_destroy(uint32_t lcore_id)
{
	struct rte_ip_frag_tbl* tbl;

	tbl = Objs[lcore_id].tbl;
	ipv4_reassemble_dpdk_death_row_free(lcore_id);
	rte_ip_frag_table_destroy(tbl);
	return 0;
}

char* ipv4_reassemble(char* raw, unsigned len, char** out, unsigned* ol,
		struct pkt* pkt)
{
	char* res = NULL;
	struct rte_mbuf *p;
	struct rte_ip_frag_tbl* tbl = NULL;
	struct rte_ip_frag_death_row* dr;
	struct wedge_dpdk* wedge;
	uint32_t lcore_id;

	wedge = pkt->platform_wedge;
	lcore_id = wedge->lcore_id;
	tbl = Objs[lcore_id].tbl;
	dr = &(Objs[lcore_id].death_row);

	p = rte_ipv4_frag_reassemble_packet(tbl, dr, wedge->buf,
		wedge->cur_tsc, (struct ipv4_hdr*)raw);
	if (p != NULL) {
		if (p != wedge->buf)
			wedge->buf = p;
		*ol = p->data_len - p->l2_len - p->l3_len;
		*out = rte_pktmbuf_mtod(p, char*) - p->l2_len - p->l3_len;
		res = *out;
	}
//	rte_ip_frag_table_statistics_dump(stderr, tbl);
	return res;
}


