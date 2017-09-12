/**
 * 	Masked potato salad.
 *		by Cao Tong <tony.caotong@gmail.com>
 *		@ 2017-09-05
 *
 */

#ifndef __STREAM_TCP_H__
#define __STREAM_TCP_H__

enum tcp_status {
	TCP_C_NONE,
	TCP_C_LISTEN,
	TCP_C_SYN_SENT,
	TCP_C_SYN_RECV,
	TCP_C_ESTABLISHED,
	TCP_C_FIN_WAIT1,
	TCP_C_FIN_WAIT2,
	TCP_C_CLOSING,
	TCP_C_CLOSE_WAIT,
	TCP_C_TIME_WAIT,
	TCP_C_LAST_ACK,
	TCP_C_CLOSED
};

struct channel {
	uint32_t last_seq;
	uint32_t next_seq;
	uint32_t acked_seq;
	uint16_t window;
	
	/* it is the sender's status.*/
	enum tcp_status status;
	list_head assemble_cache;
	/* unused. */
	list_head urgent_cache;
}

enum status{
	TCP_S_NONE,
	TCP_S_CONNECTING,
	TCP_S_MIDDLEING,
	TCP_S_CONNECTED,
	TCP_S_CLOSING,
	TCP_S_CLOSED
};

#define STREAM_TCP_FLAG_MID_CREATE 0x00000001
#define STREAM_TCP_FLAG_INVALID 0x00000002
/*
 *	Logicly: there is four sequences in different view.
 *		up/down/logic serial/timeline serial
 *	Invalid: means timeout/invalid pkts/pkt missing.
 *		tcp_reassemble module handles situation: ofo/pkt-miss,
 *		it has a status is_tainted to mark pkt-missing or we
 *		considers pkt is missing.
 */
struct stream_tcp {
	uint8_t up_orient;
	struct channel up;
	struct channel down;

	enum status status;
	uint32_t flags;
}

int stream_tcp_pkt(struct pkt* pkt, struct flow_item* flow);

#endif /*__STREAM_TCP_H__*/
