#ifndef _LCP_H
#define _LCP_H

void lcp_init(void);
void fsm_sdata(unsigned short protocol, unsigned char code, unsigned char id, unsigned char *data, int datalen);
int lcp_start(void);
int lcp_input(unsigned char *buf, int len);
void lcp_close(int flag);

extern int lcp_isup;

#endif /*_LCP_H*/

