#include <stdio.h>
#include <stdlib.h>
#include "ppp.h"
#include "ppp_sys.h"

PAI pais;                  /* async control structures */
mib_stat ppp_mibstat;
UCHAR ppp_buf[256];

int ppp_isup = 0;

static int tty_fd = -1;

void ppp_end(int ttyfd)
{
	ppp_isup = 0;

	ppp_netdown();

	if(ttyfd < 0) return;

	disestablish_ppp(ttyfd);
}

int ppp_start(int ttyfd)
{
	//ppp_debug(PPPLVL_NOTE, "start ppp...\n");
	printf("start ppp1...\n");
	printf("start ppp ttyfd = %d\n",ttyfd);

	ppp_isup = 0;
	tty_fd = ttyfd;
	//printf("establish_ppp succ\n");
	printf("start ppp ttyfd = %d\n",ttyfd);
	if(establish_ppp(ttyfd))
	{
		printf("establish_ppp_fail\n");
		return 1;
	}
	else
	{
		printf("establish_ppp succ\n");
	}
	printf("start ppp2...\n");
	lcp_init();
	printf("start ppp3...\n");
	if(lcp_start()) {
		ppp_debug(PPPLVL_NOTE, "lcp start fail\n");
		goto err_close;
	}
	printf("lcp start succ\n");
	ppp_debug(PPPLVL_NOTE, "lcp start OK\n");
	printf("start ppp4...\n");
	if(chap_start()) {
		ppp_debug(PPPLVL_NOTE, "chap start fail\n");
		goto err_close;
	}
	ppp_debug(PPPLVL_NOTE, "chap start OK\n");
	printf("start ppp5...\n");
	ipcp_init();
	if(ipcp_start()) {
		ppp_debug(PPPLVL_NOTE, "ipcp start fail\n");
		goto err_close;
	}
	printf("start ppp6...\n");
	ppp_debug(PPPLVL_NOTE, "ipcp start OK\n");

	if(ppp_netup()) {
		ppp_debug(PPPLVL_NOTE, "ppp up failed!\n");
		goto err_close;
	}
	ppp_debug(PPPLVL_ALM, "ppp up!\n");
	printf("start ppp7...\n");
	ppp_isup = 1;

	return 0;

err_close:
	ppp_end(ttyfd);
	return 1;
}

void ppp_proc(void)
{
	int len;

	len = ppp_peek(PPP_LCP, ppp_buf, 256);
	if(len > 0) lcp_input(ppp_buf, len);

	if(!lcp_isup) {
		ppp_end(tty_fd);
	}
}
