#ifndef _PPP_API_H
#define _PPP_API_H

int ppp_start(int ttyfd);
void ppp_end(int ttyfd);
void ppp_proc(void);

extern int ppp_isup;

#endif /*_PPP_API_H*/
