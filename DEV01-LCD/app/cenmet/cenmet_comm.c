/**
* cenmet_comm.c -- 表计通信
* 
* 
* 创建时间: 2010-5-17
* 最后修改时间: 2010-5-17
*/

#include <stdio.h>
#include <string.h>

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/rs485.h"
#include "include/sys/schedule.h"
#include "include/sys/xin.h"
#include "include/sys/mutex.h"
#include "include/sys/timeal.h"
#include "include/lib/bcd.h"
#include "include/param/capconf.h"
#include "include/param/commport.h"
#include "include/param/meter.h"
#include "include/param/metp.h"
#include "include/cenmet/forward.h"
#include "mdb/mdbconf.h"
#include "mdb/mdbcur.h"
#include "cenmet_proto.h"
#include "cenmet_comm.h"
#include "include/sys/uart.h"
#include "include/param/meter.h"

static sys_mutex_t CenMetMutex;

#if 0
//默认抄读的数据项标识
static const unsigned short DefMetMonItems[] = {
	0x901f, 0x902f, 0x9110, 0x9120, 
	0xb61f, 0xb62f, 0xb63f, 0xb64f, 
	0xb65f,
};
#endif
#if 1
static const unsigned short DefMetMonItems[] = {
	0x9010,
};
#endif
#define NUM_DEFMON_ITEMS	(sizeof(DefMetMonItems)/sizeof(unsigned short))

#define MAX_METMON_ITEMS  31
typedef struct {
	unsigned short num;
	unsigned short items[MAX_METMON_ITEMS];
} metmonitems_t;
static metmonitems_t MetMonItems[MAX_CENMETP];

static void inline CopyDefMetMonItems(unsigned short metid)
{
	MetMonItems[metid].num = NUM_DEFMON_ITEMS;
	memcpy(MetMonItems[metid].items, DefMetMonItems, sizeof(DefMetMonItems));
}


static const unsigned int DefMetMonItems_07[] = {
	0x0001FF00, 0x0002FF00, 0x00030000, 0x00040000, 
	0x0201FF00, 0x0202FF00, 0x0203FF00, 0x0204FF00, 
	0x00206FF00,
};

#define NUM_DEFMON_ITEMS_07	(sizeof(DefMetMonItems_07)/sizeof(unsigned int))

//#define MAX_METMON_ITEMS  31
typedef struct {
	unsigned short num;
	unsigned int items[MAX_METMON_ITEMS];
} metmonitems_t_07;
static metmonitems_t_07 MetMonItems_07[MAX_CENMETP];

static void inline CopyDefMetMonItems_07(unsigned short metid)
{
	MetMonItems_07[metid].num = NUM_DEFMON_ITEMS;
	memcpy(MetMonItems_07[metid].items, DefMetMonItems_07, sizeof(DefMetMonItems_07));
}





void CenMetLock(unsigned char port)
{
	SysLockMutex(&CenMetMutex);
	Rs485Lock(port);
}

void CenMetUnlock(unsigned char port)
{
	Rs485Unlock(port);
	SysUnlockMutex(&CenMetMutex);
}

/**
* @brief 监测表项初始化
*/
static void MetMonInit(void)
{
	unsigned short metid;
	XINREF pf;
	char str[32];
	int i, num;

	for(metid=0; metid<MAX_CENMETP; metid++) {
		sprintf(str, PARAM_PATH "metmon%d.xin", metid);
		pf = XinOpen(str, 'r');
		if(NULL == pf) {
			if(ParaMeter[metid].proto == METTYPE_DL645)
			{
				CopyDefMetMonItems(metid);
			}
			else if(ParaMeter[metid].proto == METTYPE_DL645_2007)
			{
				CopyDefMetMonItems_07(metid);
			}
			continue;
		}

		num = XinReadInt(pf, "num", -1);
		if(num < 0) {
			if(ParaMeter[metid].proto == METTYPE_DL645)
			{
				CopyDefMetMonItems(metid);
			}
			else if(ParaMeter[metid].proto == METTYPE_DL645_2007)
			{
				CopyDefMetMonItems_07(metid);
			}
			XinClose(pf);
			continue;
		}
		DebugPrint(0, "read met %d mon items = %d\n", num);
		if(num > MAX_METMON_ITEMS) num = MAX_METMON_ITEMS;
		if(ParaMeter[metid].proto == METTYPE_DL645)
		{
			MetMonItems[metid].num = num;
		}
		else if(ParaMeter[metid].proto == METTYPE_DL645_2007)
		{
			MetMonItems_07[metid].num = num;
		}

		for(i=0; i<num; i++) {
			sprintf(str, "item%d", i+1);
			if(ParaMeter[metid].proto == METTYPE_DL645)
			{
				MetMonItems[metid].items[i] = XinReadInt(pf, str, 0);
				DebugPrint(0, "mon items = %04X\n", MetMonItems[metid].items[i]);
			}
			else if(ParaMeter[metid].proto == METTYPE_DL645_2007)
			{
				MetMonItems_07[metid].items[i] = XinReadInt(pf, str, 0);
				DebugPrint(0, "mon items = %04X\n", MetMonItems_07[metid].items[i]);
			}
		}

		XinClose(pf);
	}
}

#if 0
/**
* @brief 将表计通信端口号转换为485端口号
* @param port 表计通信端口号
* @return 485端口号0~2
*/
static unsigned char MetPortToRs485Port(unsigned char port)
{
	if(0 == port) port = COMMPORT_RS485_2;
	else port -= 1;
	if(port < COMMPORT_RS485_1 || port > COMMPORT_RS485_3) port = COMMPORT_RS485_2;

	port -= COMMPORT_RS485_1;
	return port;
}
#endif










int make_cen_meter_read_645_pkt(const unsigned char *addr, unsigned int itemid, unsigned char proto,unsigned char *buf)
{
	unsigned char check, *puc;
	int i;


	if(buf == NULL || addr == NULL)	
	{
		PrintLog(LOGTYPE_DOWNLINK, "buf...addr...err\n");
		return -1;
	}
	puc = buf;
	*puc++ = 0x68;
	for(i=0; i<6; i++) *puc++ = addr[i];
	*puc++ = 0x68;
	switch(proto)
	{
		case 1:
			*puc++ = 0x01;
			*puc++ = 0x02;
			*puc++ = (unsigned char)(itemid) + 0x33;
			*puc++ = (unsigned char)(itemid>>8) + 0x33;
			puc = buf;
			check = 0;
			for(i=0; i<12; i++) check += *puc++;
			*puc++ = check;
			*puc = 0x16;
			return 14;
		case 30:
			*puc++ = 0x11;
			*puc++ = 0x04;
			*puc++ = (unsigned char)(itemid) + 0x33;
			*puc++ = (unsigned char)(itemid>>8) + 0x33;
			*puc++ = (unsigned char)(itemid>>16) + 0x33;	
			*puc++ = (unsigned char)(itemid>>24) + 0x33;			
			puc = buf;
			check = 0;
			for(i=0; i<14; i++) check += *puc++;
			*puc++ = check;
			*puc = 0x16;
			return 16;
	}
	return 0;
}


unsigned char cen_meter_485_buffer[256];

int RecvPkt_485(unsigned char *buf,int timeout,unsigned char port_num)
{
	int times, state, recvlen, maxlen,rcv_cnt;
	unsigned char *pbuf = cen_meter_485_buffer;
	char c;
	
	state = 0;
	recvlen = 0;
	maxlen = 0;
	rcv_cnt = 0;
	
	memset(cen_meter_485_buffer,0x00,sizeof(cen_meter_485_buffer));

	for(times=0; times<timeout; times++) 
	{
		while(UartRecv(port_num, &c, 1) > 0) 
		{
			PrintLog(LOGTYPE_DATA, "pbuf = %02x\n",*pbuf);
			switch(state) 
			{
			case 0:
				if(0x68 == c) 
				{
					*pbuf++ = c;
					recvlen++;
					maxlen = 7;
					rcv_cnt = 0;
					state = 1;
					PrintLog(LOGTYPE_DATA, "recvlen = %d\n",recvlen);
				}
				break;
			case 1:
				PrintLog(LOGTYPE_DATA, "pbuf1 = %02x\n",*pbuf);
				PrintLog(LOGTYPE_DATA, "rcv_cnt = %02x\n",rcv_cnt);
				if(rcv_cnt < maxlen)
				{
					*pbuf++ = c;
					recvlen++;
					rcv_cnt++;
				}
				PrintLog(LOGTYPE_DATA, "c = %02x rcv_cnt = %d\n",c,rcv_cnt);
				PrintLog(LOGTYPE_DATA, "rcv_cnt = %02x\n",rcv_cnt);
				if(rcv_cnt == maxlen && c == 0x68)
				{
					maxlen = 2;
					rcv_cnt = 0;
					state = 2;
					PrintLog(LOGTYPE_DATA, "recvlen = %d\n",recvlen);
				}
				
				if(rcv_cnt == maxlen && c != 0x68)
				{
					pbuf = cen_meter_485_buffer;
					recvlen = 0;
					maxlen = 0;
					state = 0;
					break;
				}
				
				break;
			case 2:
				
				PrintLog(LOGTYPE_DATA, "c = %02x\n",c);
				if(rcv_cnt <= maxlen)
				{
					*pbuf++ = c;
					rcv_cnt++;
					recvlen++;
				}
				if(rcv_cnt == maxlen)
				{
					maxlen = c + 2;
					rcv_cnt = 0;
					state = 3;
					PrintLog(LOGTYPE_DATA, "recvlen = %d\n",recvlen);
				}
				break;
			case 3:
				PrintLog(LOGTYPE_DATA, "c = %02x\n",c);
				*pbuf++ = c;
				rcv_cnt++;
				recvlen++;
				if(rcv_cnt >= maxlen) 
				{
					PrintLog(LOGTYPE_DATA, "recvlen = %d\n",recvlen);
					goto mark_rcvend;
				}
				break;
			}
		}

		Sleep(10);
	}
	return -1;
mark_rcvend:
		smallcpy(buf, cen_meter_485_buffer,recvlen);
		return recvlen;
		
}



int check_cen_meter_rcv_pak(unsigned char *buf,int len)
{
	int i = 0,check_sum = 0;
	
	if(len<14)	return 1;
	if(buf[0] != 0x68)	return 1;
	if(buf[7] != 0x68)	return 1;
	for(i=0;i<len-2;i++)
	{
		check_sum += buf[i];
	}
	check_sum = (check_sum&0xFF);
	PrintLog(LOGTYPE_DOWNLINK, "check_sum = %2x buf[i] = %2x\n",check_sum,buf[i]);
	if(check_sum !=buf[i])	return 1;
	return 0;
}


static const int BaudList[8] = {1200, 600, 1200, 2400, 4800, 7200, 9600, 19200};

/**
* @brief 读取485多功能表数据
* @param metid 表计ID
* @param itemid 数据项标识
* @param buf 数据缓存区指针
* @param plen 返回的实际读取长度
* @return 成功0, 否则失败
*/

//int CenMetRead(unsigned short metid, unsigned short itemid, unsigned char *buf, int *plen)
int CenMetRead(unsigned short metid, unsigned int itemid, unsigned char *buf, int *plen)
{
	int i;
	unsigned char port;
	unsigned char pak_645[256];
	unsigned char pak_len;
	unsigned char timeout = 0;
	unsigned char rcv_buf[256];
	int rcv_len = 0;

	PrintLog(LOGTYPE_DOWNLINK, "CenMetRead.........\n");
	port = 0;
	
	CenMetLock(port);
	Sleep(10);
	PrintLog(LOGTYPE_DOWNLINK, "ParaMeter[%d].proto = %d\n",metid,ParaMeter[metid].proto);
	PrintLog(LOGTYPE_DOWNLINK, "userclass = %d\n",ParaMeter[metid].userclass);
	if(ParaMeter[metid].proto == 1)
	{
		if(ParaMeter[metid].userclass == 5) 
		{
			itemid = 0x901F;
		}
		else
		{
			itemid = 0x9010;
		}
	}
	else if(ParaMeter[metid].proto == 30)
	{
		if(ParaMeter[metid].userclass == 5) 
		{
			itemid = 0x0001FF00;
		}
		else
		{
			itemid = 0x00010000;
		}
	}
	pak_len = make_cen_meter_read_645_pkt((unsigned char *)&ParaMeter[metid].addr[0],itemid, ParaMeter[metid].proto,pak_645);
	PrintLog(LOGTYPE_DOWNLINK, "pak_len = %d\n",pak_len);
	memset(rcv_buf,0x00,sizeof(rcv_buf));
	do
	{   
		timeout++;
		rcv_len = 0;
		UartSend(3, pak_645,pak_len);
		rcv_len = RecvPkt_485(rcv_buf,100,COMMPORT_RS485_3);
		PrintLog(LOGTYPE_DOWNLINK, "rcv_len = %d\n",rcv_len);
		if(rcv_len>0)
		{
			PrintLog(LOGTYPE_DOWNLINK, "rcv_buf = ");
			for(i=0;i<rcv_len;i++)
				PrintLog(LOGTYPE_DOWNLINK, "%02X ",rcv_buf[i]);
			PrintLog(LOGTYPE_DOWNLINK, "\n");	
			break;
		}
		else
		{
			PrintLog(LOGTYPE_DOWNLINK, "RecvPktFail.........\n");
		}
		Sleep(10);
	}while(timeout<3);
	if(timeout>=3)
	{
		CenMetUnlock(port);
		return 1;
	}
	//*plen = rcv_len;
	*plen = rcv_len - 14;
	PrintLog(0, "*plen = %d\n",*plen);
	//*plen = rcv_len - 2;

	CenMetUnlock(port);
	if(!check_cen_meter_rcv_pak(rcv_buf,rcv_len))
	{
		if(ParaMeter[metid].proto == 1)
		{
			memcpy(buf,rcv_buf+12,rcv_len-12);
			for(i=0;i<rcv_len-14;i++)
			{
				buf[i] -= 0x33;
				PrintLog(0, "buf[%d] = %x\n",i,buf[i]);
			}
			return 0;	
		}
		else if(ParaMeter[metid].proto == 30)
		{
			memcpy(buf,rcv_buf+14,rcv_len-14);
			for(i=0;i<rcv_len-16;i++)
			{
				buf[i] -= 0x33;
			}
			return 0;	
		}
	}
	return 1;
}



/*
int CenMetRead(unsigned short metid, unsigned short itemid, unsigned char *buf, int *plen)
{
	metinfo_t metinfo;
	metfunc_t * pfun;
	int rtn;
	unsigned char port;

	*plen =0;

	if(metid >= MAX_CENMETP) {
		*plen = GetCMetStdLen(itemid);
		return 1;
	}

	pfun = GetCMetFunc(metid, FUNCTYPE_READ);
	if(NULL == pfun) {
		*plen = GetCMetStdLen(itemid);
		return 1;
	}

	if(0 == (ParaMeter[metid].portcfg&METBAUD_MASK)) metinfo.baudrate = pfun->speed;
	else metinfo.baudrate = BaudList[ParaMeter[metid].portcfg>>5];
	metinfo.databits = pfun->databits;
	metinfo.stopbits = pfun->stopbits;
	metinfo.parity = pfun->parity;
	metinfo.password = ParaMeter[metid].pwd;
	metinfo.usrname = NULL;

	port = COMMPORT_CENMET;
	metinfo.port = port;
	metinfo.metid = ParaMeter[metid].metp_id;
	metinfo.itemid = itemid;
	for(rtn=0; rtn<6; rtn++) metinfo.addr[rtn] = ParaMeter[metid].addr[rtn];
	metinfo.rbuf = buf;

	CenMetLock(port);

	Sleep(10);

	while(Rs485Recv(metinfo.port, buf, 1) > 0);

	rtn = (*pfun->read)(&metinfo);
	*plen = metinfo.rlen;

	CenMetUnlock(port);

	return rtn;	
}
*/
/**
* @brief 读取485多功能表时间
* @param metid 表计ID
* @param pclk 返回时钟变量指针
* @return 成功返回0, 失败返回1
*/
int CenMetReadTime(unsigned short metid, sysclock_t *pclk)
{
	metfunc_t *pfunc;
	int i, j, len;
	unsigned char buf[8];
	sysclock_t clk[2];

	pfunc = GetCMetFunc(metid, FUNCTYPE_READ);
	if(NULL == pfunc) return 1;

	if(METATTR_DL645LIKE != pfunc->attr) return 1;

	for(i=0; i<2; i++) {
		for(j=0; j<3; j++) {
			if(!CenMetRead(metid, 0xc010, buf, &len)) break;

			Sleep(20);
		}
		if(j >= 3) return 1;

		BcdToHex(buf, 4);
		clk[i].year = buf[3];
		clk[i].month = buf[2];
		clk[i].day = buf[1];

		Sleep(30);

		for(j=0; j<3; j++) {
			if(!CenMetRead(metid, 0xc011, buf, &len)) break;

			Sleep(20);
		}
		if(j >= 3) return 1;

		BcdToHex(buf, 3);
		clk[i].hour = buf[2];
		clk[i].minute = buf[1];
		clk[i].second = buf[0];

		if(0 == i) Sleep(50);
	}	

	i = SysClockDifference(&clk[1], &clk[0]);
	if(i < 0) i *= -1;
	i /= 60;
	if(i > 3) return 1;

	*pclk = clk[1];
	return 0;
}

/**
* @brief 数据透明转发
* @param pcmd 命令缓存区指针
* @param pecho 返回数据缓存区指针
*/
void CenMetForward(const metfrwd_t *pcmd, metfrwd_echo_t *pecho)
{
	int len, time, timeout;
	int baud, databits, stopbits;
	unsigned char port, uc;
	unsigned char *puc;
	char parity;

	pecho->len[0] = pecho->len[1] = 0;

	port = COMMPORT_CENMET;

	uc = (pcmd->flag>>5)&0x07;
	baud = BaudList[uc];

	databits = (int)((pcmd->flag)&0x03) + 5;
	if(pcmd->flag & 0x08) {
		if(pcmd->flag & 0x04) parity = 'O';
		else parity = 'E';
	}
	else parity = 'N';
	if(pcmd->flag & 0x10) stopbits = 2;
	else stopbits = 1;

	Rs485Lock(port);

	Rs485Set(port, baud, databits, stopbits, parity);
	while(Rs485Recv(port, &uc, 1) > 0);

	len = (int)((unsigned short)pcmd->len[0] + ((unsigned short)pcmd->len[1]<<8))&0xffff;
	Rs485Send(port, pcmd->data, len);

	len = 0;
	pecho->port = pcmd->port;
	timeout = (int)(pcmd->timeout1)&0x7f;
	if(pcmd->timeout1&0x80) timeout *= 100;
	time = 0;
	puc = pecho->data;

	while(time < timeout) {
		while(Rs485Recv(port, puc, 1) > 0) {
			timeout = (int)(pcmd->timeout2)&0xff;
			time = 0;

			puc++;
			len++;
			if(len >= 800) break;
		}

		time += 10;
		Sleep(10);
		if(len >= 800) break;
	}

	Rs485Unlock(port);

	pecho->len[0] = (unsigned char)len;
	pecho->len[1] = (unsigned char)(len>>8);
}

/**
* @brief 表计数据自动读取
*/
#define ERR_COM_TIME	3


//根据抄表时间间隔自动读取多功能表数据
void CenMetProc(void)
{
	static int poll = 0;

	unsigned char metc_ibuf[128];
	unsigned short metid, metpid;
	//unsigned char metattr;
	int read_step, step_num, retry, len;
	unsigned short *pnode;
	unsigned int *pnode_07;

	for(metid=0; metid<MAX_CENMETP; metid++) 
	{
		//if(0 == ParaMeter[metid].metp_id || ParaMeter[metid].metp_id > MAX_CENMETP) continue;
		if(0 == ParaMeter[metid].metp_id) continue;
		if(METTYPE_ACSAMP == ParaMeter[metid].proto || METTYPE_PLC == ParaMeter[metid].proto) continue;
		if((ParaMeter[metid].portcfg&0x1F) != CEN_METER_PORT)		continue;
		metpid = ParaMeter[metid].metp_id;//测量点号
		if(ParaMeter[metid].proto == METTYPE_DL645)
		{
			step_num = MetMonItems[metid].num;//数据标识个数
			pnode = &MetMonItems[metid].items[0];//数据项标识
			for(read_step=0; read_step<step_num; read_step++,pnode++) 
			{
				for(retry=0; retry <ERR_COM_TIME; retry++) //重读次数
				{
					if(!CenMetRead(metid, *pnode, metc_ibuf, &len)) 
					{
						UpdateMdbCurrent(metpid, *pnode, metc_ibuf, len, UPCURFLAG_645);//更新读取正确的数据项
						break;
					}
					Sleep(10);
				}
				if(retry >= ERR_COM_TIME)
				{
					UpdateMdbCurrent(metpid, *pnode, metc_ibuf, len, UPCURFLAG_ERROR);//更新读取错误数据项
				}
			}
		}
		else if(ParaMeter[metid].proto == METTYPE_DL645_2007)
		{
			step_num = MetMonItems_07[metid].num;//数据标识个数
			pnode_07 = &MetMonItems_07[metid].items[0];//数据项标识

			for(read_step=0; read_step<step_num; read_step++,pnode_07++) 
			{
				for(retry=0; retry <ERR_COM_TIME; retry++) //重读次数
				{
					if(!CenMetRead(metid, *pnode_07, metc_ibuf, &len)) 
					{
						UpdateMdbCurrent(metpid, *pnode_07, metc_ibuf, len, UPCURFLAG_645);//更新读取正确的数据项
						break;
					}

					Sleep(10);
				}

				if(retry >= ERR_COM_TIME)
				{
					UpdateMdbCurrent(metpid, *pnode_07, metc_ibuf, len, UPCURFLAG_ERROR);//更新读取错误数据项
				}
			}
		}				
	}
	poll++;
	if(poll >= 50000) poll = 0;

	return;
}

/**
* @brief 读取多个表计数据并更新数据库
* @param mid 测量点id, 0~
* @param pitems 数据标识列表指针
* @param num 数据标识数目
*/
void ReadImmMdbCur(unsigned short mid, const unsigned short *pitems, unsigned short num)
{
	unsigned char metc_ibuf[128];
	unsigned short metid;
	//unsigned char attr;
	unsigned short itemid, i;
	int retry, len;

	PrintLog(0, "ReadImmMdbCur1\n");
	if(CenMetpMap[mid].type != METP_METER) return;
	PrintLog(0, "ReadImmMdbCur2\n");
	metid = CenMetpMap[mid].metid;
	
	if(METTYPE_ACSAMP == ParaMeter[metid].proto) return;
	PrintLog(0, "ReadImmMdbCur3\n");
	//GetCMetAttr(mid, &attr);
	//if((METATTR_EMPTY == attr) || (METATTR_DL645LIKE != attr)) return;
	//PrintLog(0, "ReadImmMdbCur4.............................\n");
	UpdateMdbCurRdTime(mid);//更新抄表时间
	PrintLog(0, "ReadImmMdbCur4\n");
	//mid++;

	for(i=0; i<num; i++) {
		itemid = *pitems++;

		for(retry=0; retry <ERR_COM_TIME; retry++) {
			if(!CenMetRead(mid, itemid, metc_ibuf, &len)) {
				UpdateMdbCurrent(mid, itemid, metc_ibuf, len, UPCURFLAG_645);
				break;
			}

			Sleep(10);
		}
	}
	PrintLog(0, "ReadImmMdbCur5\n");
}

DECLARE_INIT_FUNC(CMetCommInit);


void cen_meter_port_param_init(void)
{
	#if 1
	//const para_commport_t *cen_meter_port;
	const para_commport_t *cen_meter_port = GetParaCommPort(CEN_METER_PORT - 1);
	int baudrate;
	char parity;
	int stopbits;
	int databits;

	PrintLog(0, "cen_meter_port_param_init1\n");

	//cen_meter_port = GetParaCommPort(CEN_METER_PORT - 1);

	baudrate = cen_meter_port->baudrate;
	PrintLog(0, "cen_meter_port_param_init2\n");
	databits = (cen_meter_port->frame & 0x03) + 5;
	PrintLog(0, "cen_meter_port_param_init3\n");
	if(cen_meter_port->frame & 0x10)	stopbits = 2;
	else		stopbits = 1;
	PrintLog(0, "cen_meter_port_param_init4\n");
	if(cen_meter_port->frame & 0x08)
	{
		if(cen_meter_port->frame & 0x04)
			parity = 'O';
		else
			parity = 'E';
		PrintLog(0, "cen_meter_port_param_init5\n");
	}
	else
	{
		parity = 'N';
		PrintLog(0, "cen_meter_port_param_init6\n");
	}
	PrintLog(0, "cen_meter_port_param_init7\n");
	PrintLog(0, "baudrate = %d\n",baudrate);
	PrintLog(0, "databits = %d\n",databits);
	PrintLog(0, "stopbits = %d\n",stopbits);
	//PrintLog(0, "parity = %s\n",parity);
	//PrintLog(0, "baudrate = %d databits = %d stopbits = %d parity = %s\n",baudrate,databits,stopbits,parity);
	PrintLog(0, "cen_meter_port_param_init8\n");
	UartOpen(COMMPORT_RS485_3);
	UartSet(COMMPORT_RS485_3, baudrate, databits, stopbits, parity);
	//UartSet(COMMPORT_RS485_3, 1200, 8, 1, 'E');
	//UartSet(COMMPORT_RS485_3, 1200, 8, 1, 'O');
	#endif

	#if 0
	UartOpen(COMMPORT_RS485_3);
	UartSet(COMMPORT_RS485_3, 1200, 8, 1, 'E');
	#endif
}

int CMetCommInit(void)
{
	SysInitMutex(&CenMetMutex);
	MetMonInit();
	cen_meter_port_param_init();
	SET_INIT_FLAG(CMetCommInit);

	return 0;
}

