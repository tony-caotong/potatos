/**
 * 	Masked potato salad.
 *		by Cao tong <tony.caotong@gmail.com>
 *		@ 2017-02-07
 *
 */

#include <stdio.h>
#include <unistd.h>

#include <time.h>
#include <signal.h>
#include <semaphore.h>

#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_lcore.h>
#include <rte_eal.h>
#include <rte_debug.h>
#include <rte_mbuf.h>
#include <rte_errno.h>
#include <rte_launch.h>
#include <rte_ip.h>

#include "decoder.h"
#include "priv.h"
#include "hw_features.h"
#include "filter.h"
#include "debug.h"

#define P_PKT_SNAPLEN 8192

struct share_block {
	sem_t sem_monitor;
} __attribute__((__packed__));

static int Quit = 0;
static struct share_block Shb[RTE_MAX_LCORE];
static int Pktoff = 0;

struct filter_ipv4_rule filter_rules[] = {
	{IPv4(10,1,0,0), 16, FILTER_DEFAULT_PASS},
	{IPv4(10,2,0,0), 24, FILTER_DEFAULT_PASS},
	{IPv4(10,3,0,0), 24, FILTER_DEFAULT_PASS},
	{IPv4(10,1,1,0), 24, FILTER_DEFAULT_DROP},
	{IPv4(10,2,2,0), 16, FILTER_DEFAULT_DROP},
	{IPv4(10,0,3,0), 24, FILTER_DEFAULT_DROP}
};

void _sig_handle(int sig)
{
	int lcore_id;
	sem_t* sm;

	switch (sig) {
	case SIGINT:
		Quit = 1;
		break;
	case SIGUSR1:
		RTE_LCORE_FOREACH_SLAVE(lcore_id) {
			sm = &(Shb[lcore_id].sem_monitor);
			if (sem_post(sm) < 0) {
				perror("sem_post: ");
			}
		}
		break;
	default:
		printf("_sig_handle: default\n");
		break;
	}
}

void debug_binary_print(char* capital, char* buf, size_t length)
{
	int i = 0;
	
	printf("Capital: [%s]\n", capital);
	while(i < length) {
		if (i%32 == 0)
			printf("0x%016lx --     ", (unsigned long)&buf[i]);
		printf("%02X", (unsigned char)buf[i]);
		i++;
		if (i%32 == 0 && i != length)
			printf("\n");
		else if (i%8 == 0)
			printf(" ");
	}
	printf("\n");
}

/* NOTE: informations for priv data.
	in struct rte_mbuf:
	1. According to API description of func rte_pktmbuf_pool_create().
	   priv_buf posit between struct rte_mbuf and data buffer.
	2. buf->priv_size == buf->buf_addr - (buf + sizeof(*buf)).
	3. the memory map like this:
		.---------------------------------------------------.
		| rte_mbuf | priv buf | headroom | pkt buf |  null  |
		^----------^----------^----------^---------^--------^
		a          b          c          d         e        f
	    buf == a
	    buf->buf_addr == c
	    a + sizeof(struct rte_mbuf) == b
	    b + buf->priv_size == c
	    c + buf->data_off == d
	    d + buf->data_len == e
	    d + buf->data_len == e
	    c + buf->buf_len == f

	"d - c" was defined by MACRO: RTE_PKTMBUF_HEADROOM 
	"f - d" was defined by MACRO: RTE_MBUF_DEFAULT_DATAROOM	
	"f - c" was a parameter passed to rte_pktmbuf_pool_create()
	MUST: "f - d" >= 2KB

	*** headroom size is fixed to 128 bytes (RTE_PKTMBUF_HEADROOM)

*/
void handle_mbuf(struct rte_mbuf* buf, uint32_t sockid)
{
	struct pkt* pkt;
	struct priv* priv;
	char* raw;
	int len;

	priv = (void*)buf + sizeof(struct rte_mbuf);
	pkt = buf->buf_addr + Pktoff;
	raw = rte_pktmbuf_mtod(buf, char*);
	len = buf->data_len;

	/* 1. counters and staters. */
	// TODO

	/* 2. filters. */
	if (RTE_ETH_IS_IPV4_HDR(buf->packet_type)) {
		void* p = raw + sizeof(struct ether_hdr);
		struct iphdr* iph = p;

		/* skip vlans. */
		if (IS_HW_VLAN_PKT(buf->ol_flags)) {
			int i;
			uint16_t ether_type = ETHER_TYPE_VLAN;

			for (i = 0; i < MAX_VLAN_EMBED && ether_type == 
				rte_cpu_to_be_16(ETHER_TYPE_VLAN); i++) {
				struct vlan_hdr *vh = p;
				ether_type = vh->eth_proto;
				p = vh + 1;
			}
		}

		if (!is_filter_ipv4_pass(iph, sockid)) {
			/* ATTENTION: this pack was droped. */
			printf("ATTENTION: this pack was droped.\n");
			return;
		}
		/* BTW: assign predecode values. */
		// TODO:

	} else if (RTE_ETH_IS_IPV6_HDR(buf->packet_type)) {
		;
	} else {
		;
	}

	/* 3. decoders. */
	if (decode_pkt(pkt, raw, len, priv) < 0) {
		;
	}
}

int lcore_init(uint32_t lcore_id)
{
	sem_t* sm = &(Shb[lcore_id].sem_monitor);

	if (sem_init(sm, 0, 0) < 0) {
		perror("sem_init: ");
		return -1;
	}
	return 0;
}

int lcore_destroy(uint32_t lcore_id)
{
	sem_t* sm = &(Shb[lcore_id].sem_monitor);

	if (sem_destroy(sm) < 0) {
		perror("sem_destroy: ");
		return -1;
	}
	return 0;
}

static int lcore_loop(__attribute__((unused)) void *arg)
{
	struct rte_mbuf** bufs;
	struct rte_mbuf* buf;
	
	uint64_t rx_sum;
	uint16_t nb_rx, buf_size, i;

	uint8_t port_id = 0;
	uint32_t lcore_id = rte_lcore_id();
	uint16_t rx_queue_id = 7 - lcore_id;
	struct share_block* sb = Shb + lcore_id;
	sem_t* sm = &(sb->sem_monitor);

	printf("hello from [core %u] [port %u] [queue %u]\n",
		lcore_id, port_id, rx_queue_id);

	buf_size = 16;
	bufs = malloc(buf_size * sizeof(struct rte_mbuf*));

/* Temporary codes here, FIXME: wrong usage here, 
 *	cannot init in lcore process.
 */
#if 1
	uint32_t socket_id = rte_socket_id();
	int r = filter_init(filter_rules, sizeof(filter_rules), socket_id);
	if (r < 0)
		printf("Errors. filter_init()\n");
#endif

	rx_sum = 0;
	while (!Quit) {
#if 0
		sleep(1);
		printf("heart beat from [core %u] [port %u] [queue %u]\n",
			lcore_id, port_id, rx_queue_id);
#endif
		if (sem_trywait(sm) == 0) {
			printf("thread[core: %d][port: %d][queue: %u]"\
				" recv %lu pkts\n",
				lcore_id, port_id, rx_queue_id, rx_sum);
		} else if (errno == EINVAL) {
			printf("sem_trywait error [core %d]\n", lcore_id);
		}

		nb_rx = rte_eth_rx_burst(port_id, rx_queue_id, bufs, buf_size);
		if (nb_rx <= 0) {
			/* TODO: 
			Just sleep 5 milliseconds as simple, actually epoll
			and interrupt should be used here.
			*/
			usleep(5000);
			continue;
		}
		
		/* handle each pkt. */
		for (i = 0; i < nb_rx; i++) {
			buf = bufs[i];
			debug_print_mbuf_infos(buf);
			handle_mbuf(buf, socket_id);
			rte_pktmbuf_free(buf);
		}

		/* emit infos */
		rx_sum += nb_rx;
	}
	printf("Bye from [core %u] [port %u] [queue %u]\n",
		lcore_id, port_id, rx_queue_id);
	filter_destory(socket_id);
	return 0;
}

#if 0
static uint16_t callback(uint8_t port, uint16_t queue, struct rte_mbuf *pkts[],
			uint16_t nb_pkts, uint16_t max_pkts, void *user_param)
{
}
#endif

int master_logic()
{
	printf("Hey! there is master logic.\n");
	while (!Quit)
		usleep(5000);
	return 0;
}

int main(int argc, char** argv)
{
	int ret;
	int i;
	int nb_rx_queue;
	int socket_id;
	int port_id;
	int lcore_id;
	int lcore_count;
	int buf_size;
	int pkthdr_size;
	int priv_len;

	struct rte_eth_conf eth_conf;
	struct rte_mempool* mpool;

	fprintf(stderr, "Welcome Potatos!\n");

	/* 0. Initialize DPDK environment. */
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_panic("Cannot init EAL\n");
	}

	/* 0.0 Initialize global values. */
	socket_id = rte_socket_id();
	lcore_count =  rte_lcore_count();

	/* 1. mem pool create */

	/* TODO:
		1. Config Data Room size.
		2. Check MTU configuration.
	*/
	buf_size = RTE_PKTMBUF_HEADROOM + P_PKT_SNAPLEN;
	pkthdr_size = sizeof(struct pkt);
	Pktoff = RTE_PKTMBUF_HEADROOM - pkthdr_size;
	priv_len = sizeof(struct priv);
	if (Pktoff < 0) {
		priv_len += -Pktoff;
	}

	/* Note:
		n: n = (2^q - 1)

		cache_size:
		1. must be lower or equal to CONFIG_RTE_MEMPOOL_CACHE_MAX_SIZE
			and n / 1.5
		2. n modulo cache_size == 0

		* Value of CONFIG_RTE_MEMPOOL_CACHE_MAX_SIZE is 512 which is
		defined in file '$RTE_SDK/config/common_base'.

		2^14 - 1 = 16383 = 3 * 43 * 127
		3 * 127 = 381 < 512
	*/
	mpool = rte_pktmbuf_pool_create("MBUF_POOL", (1<<14)-1, 3*127,
		priv_len, buf_size, socket_id);
	if (NULL == mpool) {
		printf("rte_pktmbuf_pool_create: %s\n",
			rte_strerror(rte_errno));
		return -1;
	}

#if 0
	int data_room_size = rte_pktmbuf_data_room_size(mpool);
	printf("data_room_size: %d\n", data_room_size);
#endif

	/* 2. Initialize Port. */
	/* TODO: deal with port_id and rx_queue */
	port_id = 0;
	nb_rx_queue = lcore_count - 1;
	memset(&eth_conf, 0, sizeof(eth_conf));
	eth_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
	eth_conf.link_speeds = ETH_LINK_SPEED_1G;      
	eth_conf.rxmode.max_rx_pkt_len = ETHER_MAX_LEN;
	eth_conf.rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IPV4;

	ret = rte_eth_dev_configure(port_id, nb_rx_queue, 0, &eth_conf);
	if (ret < 0) {
		printf("Error rte_dev_eth_configure(): %d\n", ret);
		return -1;
	}
	
	/* 3. Initialize receive queue. */
	uint16_t nb_rx_desc = 32;
	//TODO int is_hw_parse = is_hw_parse_ptype_ipv4(port_id);
	int is_hw_parse = 0;
	if (is_hw_parse)
		printf("HW parse is supporting.\n");
	else
		printf("HW parse is not supporting. enable SW parse.\n");
	struct rte_eth_rxconf* rx_conf = NULL;
	for (i = 0; i < nb_rx_queue; i++) {
		ret = rte_eth_rx_queue_setup(port_id, i, nb_rx_desc, socket_id,
			rx_conf, mpool);
		if (ret != 0) {
			perror("rte_dev_rx_queue_setup: ");
			return -1;
		}
		if (!is_hw_parse) {
			void* cb_handler = rte_eth_add_rx_callback(port_id, i,
				sw_ptype_parse_callback, NULL);
			if (!cb_handler) {
				perror("rte_eth_add_rx_callback: ");
				return -1;
			}
		}
	}

	/* 3.1. Set packet filter. */
//	rte_lpm_create(port_id, );

	/* 4. Initialize slave lcore things. */
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (lcore_init(lcore_id) < 0)
			return -1;
	}
	fprintf(stderr, "init done!\n");

	/* 5.1 start ports. */
	ret = rte_eth_dev_start(port_id);
	if (ret < 0) {
		perror("rte_dev_rx_queue_setup: ");
		return -1;
	}
	rte_eth_promiscuous_enable(port_id);

	/* 5.2 start slave lcores. */
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		rte_eal_remote_launch(lcore_loop, NULL, lcore_id);
	}

	/* 5.3 start master lcores. */
	signal(SIGINT, _sig_handle);
	signal(SIGUSR1, _sig_handle);
	master_logic();
	/* 5.3.1 waiting for dead of slaves. */
	rte_eal_mp_wait_lcore();

	/* 6.1 release slave resources. */
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		lcore_destroy(lcore_id);
	}
	/* 6.2 TODO: release master resources. */
//	rte_lpm_destory();
	/* 6.3 TODO: release global resources. */

	fprintf(stderr, "ByeBye!\n");
	return 0;
}

