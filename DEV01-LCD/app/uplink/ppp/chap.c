#include <stdio.h>
#include <stdlib.h>

#include "ppp.h"
#include "md5.h"

#define CHAP_TIMEOUT    6000   //ms

#define CHAP_CHALLENGE		1
#define CHAP_RESPONSE		2
#define CHAP_SUCCESS		3
#define CHAP_FAILURE    	4

#define MD5_SIGNATURE_SIZE    16	/* 16 bytes in a MD5 message digest */

static char chap_user[48] = "gprs";
static char chap_pwd[48] = "gprs";

//static char chap_user[48] = "CMNET";
//static char chap_pwd[48] = "*";
//static char chap_user[] = "	*";
//static char chap_pwd[] = "";

void chap_set_usrpwd(char *user, char *pwd)
{
	if(strlen(user) < 48) strcpy(chap_user, user);
	if(strlen(pwd) < 48) strcpy(chap_pwd, pwd);


	
}

static void chap_rcvchallenge(UCHAR id, UCHAR *rbuf, USHORT rlen)
{
	static MD5_CTX mdContext;

	UCHAR *pchallenge, challenge_len;
	UCHAR *outp = ppp_buf;
	int outlen;

	pchallenge = &rbuf[1];
	challenge_len = rbuf[0];
	if((challenge_len+1) > rlen) return;

	MD5Init(&mdContext);
	MD5Update(&mdContext, &id, 1);
	MD5Update(&mdContext, (UCHAR *)chap_pwd, strlen(chap_pwd));
	MD5Update(&mdContext, pchallenge, challenge_len);
	MD5Final(&mdContext);

	outlen = MD5_SIGNATURE_SIZE+strlen(chap_user)+1+4;
	MAKEHEADER(outp, PPP_CHAP);
	PUTCHAR(CHAP_RESPONSE, outp);
	PUTCHAR(id, outp);
	PUTSHORT(outlen, outp);

	PUTCHAR(MD5_SIGNATURE_SIZE, outp);
	memcpy(outp, mdContext.digest, MD5_SIGNATURE_SIZE);
	outp += MD5_SIGNATURE_SIZE;
	strcpy((char *)outp, chap_user);
	ppp_output(ppp_buf, outlen+PPP_HDRLEN);
}

int chap_start(void)
{
	int timeout, rcvlen;
	UCHAR code, id;
	UCHAR *rcvp = &ppp_buf[PPP_HDRLEN];
	USHORT datalen;

	for(timeout=0; timeout<CHAP_TIMEOUT; timeout+=100)
	{
		usleep(100000);

		rcvlen = ppp_peek(PPP_CHAP, ppp_buf, 256);
		if(rcvlen < 0) return 1;   //end;
		//printf("\nchap_start1\n");
		//printf("\nrcvlen = %d\n",rcvlen);
		if(rcvlen >= (PPP_HDRLEN+4))
		{
			GETCHAR(code, rcvp);
			GETCHAR(id, rcvp);
			GETSHORT(datalen, rcvp);
			
			ppp_debug(PPPLVL_DATA, "\nCHAP recv l=%d c=%d, id=%d, dl=%d\n", rcvlen, code, id, datalen);
			//printf("\nCHAP recv l=%d c=%d, id=%d, dl=%d\n", rcvlen, code, id, datalen);
			if((datalen+PPP_HDRLEN) > rcvlen) continue;

			switch(code)
			{
			case CHAP_CHALLENGE:
				if(datalen < 6) break;
				datalen -= 4;
				timeout = 0;
				chap_rcvchallenge(id, rcvp, datalen);
				break;
			case CHAP_SUCCESS:
				return 0;  //OK
				break;
			case CHAP_FAILURE:
				return 1; //fail
				break;
			default: break;
			}
			
			rcvp = &ppp_buf[PPP_HDRLEN];
		}
	}

	return 1;
}
