#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ppp.h"

int lcp_isup = 0;

/*
 * Options.
 */
#define CI_MRU		1	/* Maximum Receive Unit */
#define CI_ASYNCMAP	2	/* Async Control Character Map */
#define CI_AUTHTYPE	3	/* Authentication Type */
#define CI_QUALITY	4	/* Quality Protocol */
#define CI_MAGICNUMBER	5	/* Magic Number */
#define CI_PCOMPRESSION	7	/* Protocol Field Compression */
#define CI_ACCOMPRESSION 8	/* Address/Control Field Compression */

/*
 * Length of each type of configuration option (in octets)
 */
#define CILEN_VOID	2
#define CILEN_SHORT	4	/* CILEN_VOID + sizeof(short) */
#define CILEN_CHAP	5	/* CILEN_VOID + sizeof(short) + 1 */
#define CILEN_LONG	6	/* CILEN_VOID + sizeof(long) */
#define CILEN_LQR	8	/* CILEN_VOID + sizeof(short) + sizeof(long) */

#define CONFREQ		1	/* Configuration Request */
#define CONFACK		2	/* Configuration Ack */
#define CONFNAK		3	/* Configuration Nak */
#define CONFREJ		4	/* Configuration Reject */
#define TERMREQ		5	/* Termination Request */
#define TERMACK		6	/* Termination Ack */
#define CODEREJ		7	/* Code Reject */

const unsigned long ppp_asyncmap = 0xA0000;
#define LCP_RETRY    3
#define LCP_TIMEOUT   3000  //ms

static unsigned long lcp_magic = 0x153a7062;
static unsigned char lcp_id = 0;

void lcp_init(void)
{
	lcp_isup = 0;
	pais.pai_asyncmap = 0xffffffff;
	pais.compress_flag = 0;
}

void fsm_sdata(USHORT protocol, unsigned char code, unsigned char id, unsigned char *data, int datalen)
{
	unsigned char *outp = ppp_buf;
	int outlen;

	if (datalen && (data != (outp+PPP_HDRLEN+4))) memcpy(outp+PPP_HDRLEN+4, data, datalen);

	outlen = datalen + 4;

	MAKEHEADER(outp, protocol);
	PUTCHAR(code, outp);
	PUTCHAR(id, outp);
	PUTSHORT(outlen, outp);

	ppp_output(ppp_buf, outlen+PPP_HDRLEN);
}

static void magic(void)
{
	lcp_magic += 0x10102305;
	lcp_id++;
}

static void lcp_sconfreq(void)
{
	unsigned char *ucp = PPP_BUFDATA;

#define ADDCIVOID(opt) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_VOID, ucp); \
	}
#define ADDCISHORT(opt, val) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_SHORT, ucp); \
	PUTSHORT(val, ucp); \
	}
#define ADDCICHAP(opt, val, digest) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_CHAP, ucp); \
	PUTSHORT(val, ucp); \
	PUTCHAR(digest, ucp); \
	}
#define ADDCILONG(opt, val) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_LONG, ucp); \
	PUTLONG(val, ucp); \
	}
#define ADDCILQR(opt, val) { \
	PUTCHAR(opt, ucp); \
	PUTCHAR(CILEN_LQR, ucp); \
	PUTSHORT(PPP_LQR, ucp); \
	PUTLONG(val, ucp); \
	}

	ADDCISHORT(CI_MRU, PPP_MRU);
	ADDCILONG(CI_ASYNCMAP, ppp_asyncmap);
	ADDCILONG(CI_MAGICNUMBER, lcp_magic);
	ADDCIVOID(CI_PCOMPRESSION);
	ADDCIVOID(CI_ACCOMPRESSION);

	fsm_sdata(PPP_LCP, CONFREQ, lcp_id, PPP_BUFDATA, 20);
}

static const char *code_name[8] = {
	"Empty",
	"CONFREQ",
	"CONFACK",
	"CONFNAK",
	"CONFREJ",
	"TERMREQ",
	"TERMACK",
	"CODEREJ",
};

int lcp_start(void)
{
	int retry, timeout;
	int rcvlen;
	unsigned char *rcvp = &ppp_buf[PPP_HDRLEN];
	unsigned char code, id;
	int reqsent, reqrcvd;
	USHORT cilen;

	magic();
	reqsent = reqrcvd = 0;

	for(retry=0; retry<LCP_RETRY; retry++)
	{
		if(!reqsent) lcp_sconfreq();

		for(timeout=0; timeout<LCP_TIMEOUT; timeout+=100)
		{
			usleep(100000);

			rcvlen = ppp_peek(PPP_LCP, ppp_buf, 256);
			if(rcvlen >= (PPP_HDRLEN+4))
			{
				GETCHAR(code, rcvp);
				GETCHAR(id, rcvp);
				GETSHORT(cilen, rcvp);
				ppp_debug(PPPLVL_DATA, "\nlcp recv c=%d, id=%d, cilen=%d", code, id, cilen);
				if(code < 8) ppp_debug(PPPLVL_DATA, ", %s\n", code_name[code]);
				else ppp_debug(PPPLVL_DATA, ", empty\n");
				if(cilen > 4)
				{
					if(CONFACK == code) reqsent = 1;
					else if(CONFREQ == code)
					{
						fsm_sdata(PPP_LCP, CONFACK, id, rcvp, cilen-4);
						reqrcvd = 1;
					}
					else if(CONFREJ == code) return 1;  //fail
				}
				else if(TERMREQ == code) {
					//lcp_isup = 1;
					fsm_sdata(PPP_LCP, TERMACK, id, NULL, 0);
					//lcp_close(1);
					return 1;
				}

				if(reqsent&&reqrcvd)
				{
					pais.pai_asyncmap = ppp_asyncmap;
					pais.compress_flag = COMPRESS_AC|COMPRESS_PR;
					lcp_isup = 1;
					return 0;
				}
			}
			rcvp = &ppp_buf[PPP_HDRLEN];
		}

		//if(!reqsent) lcp_sconfreq();
	}

	return 1;
}

void lcp_close(int flag)
{
	int retry, timeout, rcvlen;
	int reqsent, reqrcvd;
	unsigned char code, id;
	unsigned char *rcvp = &ppp_buf[PPP_HDRLEN];

	if(!lcp_isup) return;

	reqsent = 0;
	if(flag) reqrcvd = 1;
	else reqrcvd = 0;

	lcp_isup = 0;

	for(retry=0; retry<LCP_RETRY; retry++)
	{
		if(!reqsent) fsm_sdata(PPP_LCP, TERMREQ, lcp_id, NULL, 0);

		for(timeout=0; timeout<5; timeout++)  //500ms
		{
			usleep(100000);

			rcvlen = ppp_peek(PPP_LCP, ppp_buf, 256);
			if(rcvlen >= (PPP_HDRLEN+2))
			{
				GETCHAR(code, rcvp);
				GETCHAR(id, rcvp);
				ppp_debug(PPPLVL_DATA, "\nlcp recv c=%d, id=%d", code, id);
				if(code < 8) ppp_debug(PPPLVL_DATA, ", %s\n", code_name[code]);
				else ppp_debug(PPPLVL_DATA, ", empty\n");
				
				if(TERMACK == code) reqsent = 1;
				else if(TERMREQ == code) {
					fsm_sdata(PPP_LCP, TERMACK, id, NULL, 0);
					reqrcvd = 1;
				}

				if(reqsent&&reqrcvd) return;
			}
			rcvp = &ppp_buf[PPP_HDRLEN];
		}
	}

	return;
}

int lcp_input(unsigned char *buf, int len)
{
	unsigned char code, id;

	if(len < (PPP_HDRLEN+2)) return 0;

	code = buf[PPP_HDRLEN];
	id = buf[PPP_HDRLEN+1];
	if(TERMREQ == code) { //end
		fsm_sdata(PPP_LCP, TERMACK, id, NULL, 0);
		lcp_close(1);
		return 1;
	}

	return 0;
}
