/**
* 	Masked potato salad.
*		By Cao tong <tony_caotong@gmail.com>
*		AT 2017-02-07
*
*/

#include <stdio.h>

#include <time.h>
#include <unistd.h>

#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_lcore.h>
#include <rte_eal.h>
#include <rte_debug.h>
#include <rte_mbuf.h>
#include <rte_errno.h>
#include <rte_launch.h>

struct priv{
};


void binary_print(char* capital, char* buf, size_t length)
{
	int i = 0;
	
	printf("Capital: [%s]\n", capital);
	while(i < length) {
		if (i%16 == 0)
			printf("0x%016lx -- ", (unsigned long)&buf[i]);
		printf("%02X", (unsigned char)buf[i]);
		i++;
		if (i%16 == 0 && i != length)
			printf("\n");
		else if (i%8 == 0)
			printf("\t");
	}
	printf("\n");
}

void handle_mbuf(struct rte_mbuf* buf)
{
	char captial[32];
	char* p = buf->buf_addr + buf->data_off;
	int l = buf->pkt_len;

	snprintf(captial, sizeof(captial), "packet length: %d", l);
	if (l > 1024) {
		printf("%s\n", captial);
	}
//	binary_print(captial, p, l);
}

static int lcore_hello(__attribute__((unused)) void *arg)
{
	struct rte_mbuf** bufs;
	struct rte_mbuf* buf;
	
	time_t old, new;
	uint16_t rx_sum, nb_rx, buf_size, i;

	uint8_t port_id = 0;
	uint32_t lcore_id = rte_lcore_id();
	uint16_t rx_queue_id = 7 - lcore_id;

	printf("hello from [core %u] [port %u] [queue %u]\n",
		lcore_id, port_id, rx_queue_id);

	buf_size = 16;
	bufs = malloc(buf_size * sizeof(struct rte_mbuf*));

	old = time(NULL);
	rx_sum = 0;
	while (1) {
#if 0
		sleep(1);
		printf("heart beat from [core %u] [port %u] [queue %u]\n",
			lcore_id, port_id, rx_queue_id);
#endif
		nb_rx = rte_eth_rx_burst(port_id, rx_queue_id, bufs, buf_size);
		if (nb_rx <= 0) {
			continue;
		}
		
		/* handle each pkt. */
		for (i = 0; i < nb_rx; i++) {
			buf = bufs[i];
			handle_mbuf(buf);
		}

		/* emit infos */
		rx_sum += nb_rx;
		new = time(NULL);
		if (new > old + 5) {
			printf("thread[core: %d][port: %d][queue: %u]"\
				" recv %u pkts\n",
				lcore_id, port_id, rx_queue_id, rx_sum);
			rx_sum = 0;
			old = new;
		}
	}
	return 0;
}

#if 0
static uint16_t callback(uint8_t port, uint16_t queue, struct rte_mbuf *pkts[],
			uint16_t nb_pkts, uint16_t max_pkts, void *user_param)
{
}
#endif


int main(int argc, char** argv)
{
	int ret;
	int i;
	int nb_rx_queue;
	int socket_id;
	int port_id;
	int lcore_id;

	struct rte_eth_conf eth_conf;
	struct rte_mempool* mpool;

	port_id = 0;
	nb_rx_queue = 4;
	socket_id = rte_socket_id();
	printf("Hello Potato!\n");
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_panic("Cannot init EAL\n");
	}

	/* 1. mem pool create */
	mpool = rte_pktmbuf_pool_create("MBUF_POOL", 8192-1, 250,
		sizeof(struct priv), RTE_MBUF_DEFAULT_BUF_SIZE, socket_id);
	if (NULL == mpool) {
		printf("rte_pktmbuf_pool_create: %s\n",
			rte_strerror(rte_errno));
		return -1;
	}

	/* 2. Initialize Port. */
	eth_conf.rxmode.mq_mode = ETH_MQ_RX_RSS;
	eth_conf.link_speeds = ETH_LINK_SPEED_1G;      
	eth_conf.rxmode.max_rx_pkt_len = ETHER_MAX_LEN;
	eth_conf.rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IPV4;

	ret = rte_eth_dev_configure(port_id, nb_rx_queue, 0, &eth_conf);
	if (ret < 0) {
		perror("rte_dev_eth_configure: ");
		return -1;
	}
	
	/* 3. Initialize receive queue. */
	uint16_t nb_rx_desc = 16;
	struct rte_eth_rxconf* rx_conf = NULL;
	for (i = 0; i < nb_rx_queue; i++) {
		ret = rte_eth_rx_queue_setup(port_id, i, nb_rx_desc, socket_id,
			rx_conf, mpool);
		if (ret != 0) {
			perror("rte_dev_rx_queue_setup: ");
			return -1;
		}
#if 0
		void* cb_handler;
		cb_handler = rte_eth_add_rx_callback(port_id, i,
			callback, NULL);
		if (!cb_handler) {
			perror("rte_eth_add_rx_callback: ");
			return -1;
		}
#endif
	}

	ret = rte_eth_dev_start(port_id);
	if (ret < 0) {
		perror("rte_dev_rx_queue_setup: ");
		return -1;
	}
		
	rte_eth_promiscuous_enable(port_id);
	printf("init done!\n");

	/* call lcore_hello() on every slave lcore */
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
	}

	/* call it on master lcore too */
	lcore_hello(NULL);

	rte_eal_mp_wait_lcore();

	return 0;
}

