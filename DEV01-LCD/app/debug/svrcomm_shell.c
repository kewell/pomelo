/**
* svrcomm_shell.c -- 上行通信命令行调试
* 
* 
* 创建时间: 2010-9-17
* 最后修改时间: 2010-9-17
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/task.h"
#include "shell.h"

#include "uplink/uplink_pkt.h"
#include "uplink/uplink_dl.h"

#define SHELLARG_NUM	12
static char CmdLineArgBuf[SHELLARG_NUM][128];
static char *CmdLineArgV[SHELLARG_NUM];

static unsigned char UpInterface = 0;
static unsigned char ServerAddress = 0;
static unsigned char ServerSeq = 0;

static int TimerIdSvrShell = -1;
static int CTimerSvrShell(unsigned long arg)
{
	TimerIdSvrShell = -1;
	SetLogType(0);
	SetLogInterface(0);
	return 1;
}

void SvrShellProc(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	char *pcommand;
	int len;
	shell_func pfunc;
	int argc;

	len = UPLINKAPP_LEN(prcv) & 0xffff;
	if(len <= 4 || len > 260) return;
	pcommand = (char *)prcv->data;
	pcommand[len] = 0;
	pcommand += 4;
	len -= 4;

	argc = ShellParseArg(pcommand, CmdLineArgV, SHELLARG_NUM);
	if(argc > 0) {
		pfunc = FindShellFunc(CmdLineArgV[0]);
		if(NULL != pfunc) {
			SetLogInterface(LOGITF_SVRCOMM);
			UpInterface = itf;
			ServerAddress = prcv->grp&0xfe;
			ServerSeq = prcv->seq & 0x0f;

			if(TimerIdSvrShell < 0) TimerIdSvrShell = SysAddCTimer(120, CTimerSvrShell, 0);
			else SysClearCTimer(TimerIdSvrShell);

			//Sleep(5);
			(*pfunc)(argc, CmdLineArgV);
		}
	}
}

static char SvrShellPrintBuf[1536];
char *GetSvrShellBuffer(void)
{
	return (char *)(((uplink_pkt_t *)SvrShellPrintBuf)->data+4);
}

void SvrShellPrint(const char *str)
{
	uplink_pkt_t *pkt = (uplink_pkt_t *)SvrShellPrintBuf;

	pkt->ctrl = UPECHO_USRDATA|UPCTRL_DIR;
	pkt->afn = UPAFN_TRANFILE;
	pkt->seq = ServerSeq;
	pkt->seq |= UPSEQ_FIR|UPSEQ_FIN;
	pkt->grp = ServerAddress;
	UPLINKAPP_LEN(pkt) = strlen((char *)(pkt->data+4))+5;
	pkt->data[0] = pkt->data[1] = 0;
	pkt->data[2] = 4;
	pkt->data[3] = 1;
	UplinkSendPkt(UpInterface, pkt);
}

void SvrShellQuit(void)
{
	if(TimerIdSvrShell >= 0) {
		SysStopCTimer(TimerIdSvrShell);
		TimerIdSvrShell = -1;
	}

	return;
}

DECLARE_INIT_FUNC(SvrShellInit);
int SvrShellInit(void)
{
	int argc;

	for(argc=0; argc<SHELLARG_NUM; argc++) {
		CmdLineArgV[argc] = CmdLineArgBuf[argc];
	}

	SET_INIT_FLAG(SvrShellInit);

	return 0;
}



