#ifndef _SCR_SHOW_H
#define _SCR_SHOW_H

#define SCRHEAD_FLAG_SIG		1
#define SCRHEAD_FLAG_CHN		2
#define SCRHEAD_FLAG_ALM		3
#define SCRHEAD_FLAG_CLRALM		4
#define SCRHEAD_FLAG_METID		5
#define SCRHEAD_FLAG_LOS		6
#define SCRHEAD_FLAG_PROG		7
#define SCRHEAD_FLAG_BAT		8

void ScreenSetHeadFlag(unsigned char flag,unsigned int arg);
void ScreenDisplayAlarm(const char *str);

#endif /*_SCR_SHOW_H*/

