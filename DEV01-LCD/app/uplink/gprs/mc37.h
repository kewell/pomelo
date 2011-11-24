#ifndef _MC37_H
#define _MC37_H

int M590CheckDev(void);
int M590Restart(const char *apn, const char *sms, const char *usrname, const char *pwd);
void M590Init(void);
int M590Dail(void);
int M590DialOff(void);
int M590Connect(unsigned long ip, unsigned short port, unsigned char proto);
void M590Disconnect(void);
int M590LineState(void);
void M590Ring(void);
int M590GetChar(unsigned char *buf);
int M590RawSend(const unsigned char *buf, int len);
void M590DetectLine(unsigned char uc);

#endif /*_MC37_H*/

