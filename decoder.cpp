/**
 * 	Hey, I am Hally.
 *		by Cao tong <tony_caotong@gmail.com>
 *		@ 2017-02-14
 *
 */

#include <pkt.h>
#include <PsDecode.h>
#include <TcpAnalyze.h>
#include <PrETHERNET.h>
#include <PrTCP.h>

#include "decoder.h"
#include "priv.h"

typedef struct decoder_t {
	PsDecode* hl_dec;
	TcpAnalyze* hl_tcp;
} DECODER;

/* unused, just for compiling of daplibs. */
void *__lgwr__handle = NULL;
void *__timer_mgr_handle = NULL;

int hally_hdrlen()
{
	/*
	this is a magic value 2 for ethernet.
	./sdk/hally_root/dapcomm/lch/util/pkt.c:433 */
	return sizeof(pkt_hdr) + 2;
}

DECODER* init_decoder()
{
	PsDecode* hl_dec;
	TcpAnalyze* hl_tcp;
	DECODER* dec;

	hl_dec = new PsDecode();
	if (hl_dec == NULL) {
		return NULL;
	}

	hl_tcp = new TcpAnalyze(NULL);
	if (hl_tcp == NULL) {
		delete hl_dec;
		return NULL;
	}
	
	dec = (DECODER*)malloc(sizeof(DECODER));
	if (dec == NULL) {
		delete hl_tcp;
		delete hl_dec;
		return NULL;
	}
	dec->hl_dec = hl_dec;
	dec->hl_tcp = hl_tcp;
	return dec;
}

int decode_pkt(DECODER* dec, void* pkt, void* buf, int len)
{
	PsResult* hl_res;
	PsDecode* hl_dec;
	TcpAnalyze* hl_tcp;
	pkt_hdr* hl_hdr;
	int i;

	if (dec == NULL || pkt == NULL || buf == NULL || len <= 0) {
		return -1;
	}

	/* ./dapcomm/lch/util/pkttypes.h */
	hl_hdr = (pkt_hdr*)buf;
	hl_hdr->sc = 0;
	pkthdr_set_sync(hl_hdr);
	/* As magic number in func hally_hdrlen(), this it is too. */
	pkthdr_set_dlen(hl_hdr, len + 2);
	pkthdr_set_type(hl_hdr, PKTTYPE_DATA);
	pkthdr_set_subtype(hl_hdr, PKTTYPE_DATA_ETHERNET);
	pkthdr_set_protocol(hl_hdr, PKTPROTOCOL_AUTO);
	pkthdr_set_device(hl_hdr, 0);
	pkthdr_set_channel(hl_hdr, 0);
//	pkthdr_set_ts(hl_hdr, pcaphdr->ts.tv_sec, pcaphdr->ts.tv_usec * 1000);

	/* We presume hl_dec must be valid. */
	hl_dec = dec->hl_dec;
	hl_tcp = dec->hl_tcp;
	hl_res = hl_dec->Decode(hl_hdr, 0);

	for (i = 0; i < SS_SIZE; i++) {
		PrBase *p = hl_res->GetPr((protocol_t)i);
		if (p != NULL) {
			printf("Base: %d %s\n", i, protocol_t_str[i]);
		} else {
			continue;
		}
		if (i == SS_ETHERNET) {
			PrETHERNET* ep = dynamic_cast<PrETHERNET*>(p);
			int j;

			printf(" >>>> ether dst addr: ");
			for (j = 0; j<6; j++) {
				printf("%x ", ep->m_ucDestAddr[j]);
			}
			printf("\n");
			printf(" >>>> ether src addr: ");
			for (j = 0; j<6; j++) {
				printf("%x ", ep->m_ucSrcAddr[j]);
			}
			printf("\n");
			printf(" >>>> ether protocol: %x\n", ep->m_nProtocol);
			printf(" >>>> ether vlan_id: %d\n", ep->m_nVLanID);
		} else if (i == SS_TCP) {
			PrTCP *tcp = dynamic_cast<PrTCP*>(p);
			if (tcp && tcp->m_nPayloadLen > 0) {
				hl_tcp->Analyze(hl_res, tcp);
				pkt_hdr *ph;
				while ((ph = hl_tcp->Split()) != NULL) {
						printf(">>>>>>>>>>>>>>>>>>>>"
							"one ordered pkt.\n");
						free(ph);
				}
			}
		}
	}
	return 0;
}

void destory_decoder(DECODER* dec)
{
	if (dec == NULL) {
		return;
	}
	delete dec->hl_dec;
	dec->hl_dec = NULL;
	delete dec->hl_tcp;
	dec->hl_tcp = NULL;
	free(dec);
}

