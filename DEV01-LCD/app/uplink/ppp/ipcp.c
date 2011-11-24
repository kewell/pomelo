#include <stdio.h>
#include <stdlib.h>
#include "ppp.h"

#define CONFREQ		1	/* Configuration Request */
#define CONFACK		2	/* Configuration Ack */
#define CONFNAK		3	/* Configuration Nak */
#define CONFREJ		4	/* Configuration Reject */
#define TERMREQ		5	/* Termination Request */
#define TERMACK		6	/* Termination Ack */
#define CODEREJ		7	/* Code Reject */

#define CILEN_VOID	2
#define CILEN_COMPRESS	4	/* min length for compression protocol opt. */
#define CILEN_VJ	6	/* length for RFC1332 Van-Jacobson opt. */
#define CILEN_ADDR	6	/* new-style single address option */
#define CILEN_ADDRS	10	/* old-style dual address option */

#define CI_ADDRS	1	/* IP Addresses */
#define CI_COMPRESSTYPE	2	/* Compression Type */
#define	CI_ADDR		3

#define MAX_STATES 16

#define IPCP_VJMODE_OLD 1	/* "old" mode (option # = 0x0037) */
#define IPCP_VJMODE_RFC1172 2	/* "old-rfc"mode (option # = 0x002d) */
#define IPCP_VJMODE_RFC1332 3	/* "new-rfc"mode (option # = 0x002d, */
                                /*  maxslot and slot number compression) */

#define IPCP_VJ_COMP 0x002d	/* current value for VJ compression option*/
#define IPCP_VJ_COMP_OLD 0x0037	/* "old" (i.e, broken) value for VJ */

#define IPCP_MAXRETRY    10
#define IPCP_TIMEOUT    8000   //ms

static UCHAR ipcp_id = 0;
static ULONG tcp_addr = 0;

void ipcp_init(void)
{
	pais.maxslotindex = MAX_STATES-1;   //really max index
	pais.vj_flag = 1;
	tcp_addr = 0;
}

ULONG ipcp_get_addr(void)
{
	return tcp_addr;
}

static void ipcp_sendreq(void)
{
	UCHAR *outp = PPP_BUFDATA;

	PUTCHAR(CI_ADDR, outp);
	PUTCHAR(CILEN_ADDR, outp);
	PUTLONG(tcp_addr, outp);

	/*PUTCHAR(CI_COMPRESSTYPE, outp);
	PUTCHAR(CILEN_VJ, outp);
	PUTSHORT(IPCP_VJ_COMP, outp);
	PUTCHAR(pais.maxslotindex, outp);
	PUTCHAR(pais.vj_flag, outp);*/

	fsm_sdata(PPP_IPCP, CONFREQ, ++ipcp_id, PPP_BUFDATA, CILEN_ADDR);
}


//unsigned char gprs_ip[4];
static int ipcp_checkci(UCHAR cid, UCHAR cilen, UCHAR *cidata)
{
	ppp_debug(PPPLVL_DATA, "cid=%d, cilen=%d:\n", cid, cilen);
	//printHexString(cidata, cilen-2);

	if(CI_ADDR == cid)
	{
		ppp_debug(PPPLVL_ALM, "IP Address = %d.%d.%d.%d\n", cidata[0], cidata[1], cidata[2], cidata[3]);
		GETLONG(tcp_addr, cidata);

		return 0;
	}

	return 1;
}

int ipcp_start(void)
{
	int retry, timeout;
	int rcvlen;
	UCHAR *rcvp = &ppp_buf[PPP_HDRLEN];
	UCHAR code, id;
	int reqsent, reqrcvd;
	USHORT datalen;
	UCHAR cid, cilen;

	reqsent = reqrcvd = 0;

	for(retry=0; retry<IPCP_MAXRETRY; retry++)
	{
		if(!reqsent) ipcp_sendreq();

		for(timeout=0; timeout<IPCP_TIMEOUT; timeout+=100)
		{
			usleep(100000);

			rcvlen = ppp_peek(PPP_IPCP, ppp_buf, 256);
			if(rcvlen < 0) return 1;  //end
			if(rcvlen >= (PPP_HDRLEN+4))
			{
				GETCHAR(code, rcvp);
				GETCHAR(id, rcvp);
				GETSHORT(datalen, rcvp);

				ppp_debug(PPPLVL_DATA, "\nIPCP recv l=%d c=%d, id=%d, dl=%d\n", rcvlen, code, id, datalen);

				if(datalen >= 4)
				{
					if(CONFACK == code) reqsent = 1;
					else if(CONFREQ == code)
					{
						int baklen = datalen;

						datalen -= 4;
						while(datalen)
						{
							GETCHAR(cid, rcvp);
							GETCHAR(cilen, rcvp);
							if((cilen <= datalen) && (cilen >= 2))
							{
								ipcp_checkci(cid, cilen, rcvp);
								rcvp += (cilen-2);
							}
							else if(cilen > datalen) cilen = datalen;

							datalen -= cilen;
						}

						datalen = baklen;
						fsm_sdata(PPP_IPCP, CONFACK, id, rcvp, datalen-4);
						reqrcvd = 1;
					}
					else if(CONFREJ == code) return 1;  //fail
					else if(TERMREQ == code) return 1;  //fail
					else if(CONFNAK == code)
					{
						int brsnd = 0;

						datalen -= 4;
						while(datalen)
						{
							GETCHAR(cid, rcvp);
							GETCHAR(cilen, rcvp);
							if((cilen <= datalen) && (cilen >= 2))
							{
								if(!ipcp_checkci(cid, cilen, rcvp)) brsnd = 1;
								rcvp += (cilen-2);
							}
							else if(cilen > datalen) cilen = datalen;

							datalen -= cilen;
						}

						if(brsnd)
						{
							ipcp_sendreq();
							timeout = 0;
							retry++;
						}
					}
				}

				if(reqsent&&reqrcvd)
				{
					return 0;
				}
			}
			rcvp = &ppp_buf[PPP_HDRLEN];
		}
	}

	return 1;
}
