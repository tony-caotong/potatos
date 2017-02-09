/**
 * 	Masked potato salad.
 *		by Cao tong <tony_caotong@gmail.com>
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

struct priv{
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint8_t e;
	uint8_t f;
	uint8_t g;
	uint8_t h;
};

struct share_block {
	sem_t sem_monitor;
} __attribute__((__packed__));

static int Quit = 0;
static struct share_block Shb[RTE_MAX_LCORE];

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

void binary_print(char* capital, char* buf, size_t length)
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
		a          b          c          d         e       f
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

*/
void handle_mbuf(struct rte_mbuf* buf)
{
	char captial[32];
	char* p = buf->buf_addr + buf->data_off;
	int l = buf->pkt_len;

#if 0
	snprintf(captial, sizeof(captial), "packet length: %d", l);
	if (l > 1024) {
		printf("%s\n", captial);
	}
	binary_print(captial, p, l);
#endif
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
			handle_mbuf(buf);
			rte_pktmbuf_free(buf);
		}

		/* emit infos */
		rx_sum += nb_rx;
	}
	printf("Bye from [core %u] [port %u] [queue %u]\n",
		lcore_id, port_id, rx_queue_id);
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

	struct rte_eth_conf eth_conf;
	struct rte_mempool* mpool;

	port_id = 0;
	printf("Hello Potato!\n");
	ret = rte_eal_init(argc, argv);
	if (ret < 0) {
		rte_panic("Cannot init EAL\n");
	}
	socket_id = rte_socket_id();
	lcore_count =  rte_lcore_count();
	printf("rte_lcore_count: %d\n", lcore_count);

	/* 1. mem pool create */
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
		sizeof(struct priv), RTE_MBUF_DEFAULT_BUF_SIZE, socket_id);
	if (NULL == mpool) {
		printf("rte_pktmbuf_pool_create: %s\n",
			rte_strerror(rte_errno));
		return -1;
	}

	/* 2. Initialize Port. */
	nb_rx_queue = lcore_count - 1;
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

	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (lcore_init(lcore_id) < 0)
			return -1;
	}
	printf("init done!\n");

	/* call lcore_hello() on every slave lcore */
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		rte_eal_remote_launch(lcore_loop, NULL, lcore_id);
	}

	/* call things on master lcore*/
	signal(SIGINT, _sig_handle);
	signal(SIGUSR1, _sig_handle);
	master_logic();
	rte_eal_mp_wait_lcore();

	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		lcore_destroy(lcore_id);
	}

	/*TODO: release resources. */

	printf("ByeBye!\n");
	return 0;
}

