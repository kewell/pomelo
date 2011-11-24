#ifndef _PPP_SYS_H
#define _PPP_SYS_H

int establish_ppp(int fd);
int disestablish_ppp(int fd);

void ppp_output (unsigned char *p, int len);
int ppp_input(unsigned char *buf, int maxlen);
int ppp_peek(unsigned short protocol, unsigned char *buf, int maxlen);
int ppp_netup(void);
int ppp_netdown(void);

#endif /*_PPP_SYS_H*/
