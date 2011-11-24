/**
* tranfile.c -- 文件传输处理
* 
* 
* 创建时间: 2010-6-9
* 最后修改时间: 2010-6-9
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/environment.h"
#include "include/debug.h"
#include "include/sys/schedule.h"
#include "include/sys/timeal.h"
#include "include/sys/timer.h"
#include "include/sys/reset.h"
#include "include/lib/align.h"
#include "include/lib/crc.h"
#include "uplink_pkt.h"
#include "uplink_dl.h"
#include "terminfo.h"
#include "svrcomm.h"
#include "include/plcmet/plmdb.h"
#include "include/debug/shellcmd.h"


//#define DWNFILE_SAVEBAK		1

#define DWNFILE_CACHE		TEMP_PATH "dwnfile.tmp"

#define DWNFLAG_WORKFILE	1   //下载标准程序文件
#define DWNFLAG_RAWFILE		2   //下载普通文件

struct dwnfile_ctrl {
	unsigned char flag;

	int timerid;
	unsigned char *pmem;
	FILE *pfcache;
	unsigned int lastlen;

	unsigned short serial;
	unsigned short snmax;
	unsigned int mask;
	unsigned int full_mask;
	unsigned int filesize;
	unsigned short filecrc;

	unsigned short blksize;
	unsigned char winsize;
	unsigned char fileid;
	unsigned char blast;

	unsigned char svr_addr;

	char filename[64];
};
static struct dwnfile_ctrl DownFileCtrl;

struct upfile_ctrl {
	int timerid;
	unsigned char *pmem;
	FILE *pf;

	unsigned int memsize;

	unsigned short blksize;
	unsigned short serial;
};
static struct upfile_ctrl UpFileCtrl;

#define FILECODE_REQ_DWNWORK	0   //请求下载标准文件
#define FILECODE_REQ_DWNRAW		1   //请求下载普通文件
#define FILECODE_REQ_UPRAW		2   //请求上传普通文件
#define FILECODE_DWNDATA		3  //下载文件数据及回应
#define FILECODE_UPDATA			4  //上传文件数据及回应
#define FILECODE_REJECT			5  //否认响应
#define FILECODE_INFO			6  //发送文件信息(断点续传用)
#define FILECODE_MAX			7

#define TRANFILE_REQ_SN    0xb6589031
#define FILEID_PROTO    0
#define FILEID_PROGRAM    1
#define FILEID_DRIVER    2

#define LEN_REQ_DWNWORK		23
struct req_dwnwork {
	unsigned char duid[4];
	unsigned char code;
	unsigned char req[4];
	unsigned char size[4];
	unsigned char fileid;
	unsigned char filecrc[2];
	unsigned char blksize[2];
	unsigned char timeout[4];
	unsigned char winsize; //<=32
};

#define LEN_REQ_DWNRAW		22
struct req_dwnraw {
	unsigned char duid[4];
	unsigned char code;
	unsigned char req[4];
	unsigned char size[4];
	unsigned char filecrc[2];
	unsigned char blksize[2];
	unsigned char timeout[4];
	unsigned char winsize; //<=32
	char filename[1];
};

#define LEN_REQ_UPRAW		21
struct req_upraw {
	unsigned char duid[4];
	unsigned char code;
	unsigned char req[4];
	unsigned char size[4];
	unsigned char filecrc[2];
	unsigned char blksize[2];
	unsigned char timeout[4];
	char filename[1];
};

#define LEN_DWNDATA_HEAD	9
struct down_data {
	unsigned char duid[4];
	unsigned char code;
	unsigned char serial[2];
	unsigned char crc[2];
	unsigned char data[1];
};

struct dfile_info {
	unsigned char duid[4];
	unsigned char code;
	unsigned char filesize[4];
	unsigned char filecrc[2];
	unsigned char serial[2];
	unsigned char mask[4];
	unsigned char blksize[2];
	unsigned char fileid;
	unsigned char winsize;
	unsigned char unuse;
};

static const char *WorkFileName[] = {
	"metproto.xzip",
	"bgjzq.xzip",
	"jzq_drivers.ko",
};

/**
* @brief 清除下载文件控制
*/
static inline void ClearDownFileCtrl(void)
{
	DownFileCtrl.flag = 0;

	if(DownFileCtrl.timerid >= 0) {
		SysStopCTimer(DownFileCtrl.timerid);
		DownFileCtrl.timerid = -1;
	}

	if(NULL != DownFileCtrl.pmem) {
		free(DownFileCtrl.pmem);
		DownFileCtrl.pmem = NULL;
	}

	if(NULL != DownFileCtrl.pfcache) {
		fclose(DownFileCtrl.pfcache);
		DownFileCtrl.pfcache = NULL;
	}

	remove(DWNFILE_CACHE);
}

/**
* @brief 清除上传文件控制
*/
static inline void ClearUpFileCtrl(void)
{
	if(UpFileCtrl.timerid >= 0) {
		SysStopCTimer(UpFileCtrl.timerid);
		UpFileCtrl.timerid = -1;
	}

	if(NULL != UpFileCtrl.pmem) {
		free(UpFileCtrl.pmem);
		UpFileCtrl.pmem = NULL;
	}

	if(NULL != UpFileCtrl.pf) {
		fclose(UpFileCtrl.pf);
		UpFileCtrl.pf = NULL;
	}
}

/**
* @brief 下载文件超时定时器
*/
static int CTimerDownFile(unsigned long arg)
{
	PrintLog(LOGTYPE_ALARM, "down file timeout\n");

	DownFileCtrl.timerid = -1;
	ClearDownFileCtrl();

	return 1;
}

/**
* @brief 请求下载标准文件
* @param itf 接口编号
*/
static void ReqDownWork(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	struct req_dwnwork *preq = (struct req_dwnwork *)(prcv->data);
	struct req_dwnwork *pecho = (struct req_dwnwork *)(psnd->data);
	unsigned long timeout, memsize;

	if(UPLINKAPP_LEN(prcv) < LEN_REQ_DWNWORK) return;

	smallcpy(psnd->data, prcv->data, LEN_REQ_DWNWORK);
	UPLINKAPP_LEN(psnd) = LEN_REQ_DWNWORK;

	if(TRANFILE_REQ_SN != MAKE_LONG(preq->req)) goto mark_fail;

	ClearDownFileCtrl();

	DownFileCtrl.flag = DWNFLAG_WORKFILE;
	DownFileCtrl.fileid = preq->fileid;
	DownFileCtrl.filesize = MAKE_LONG(preq->size);
	DownFileCtrl.serial = 0;
	DownFileCtrl.mask = 0;
	DownFileCtrl.winsize = preq->winsize;
	DownFileCtrl.blksize = MAKE_SHORT(preq->blksize);
	DownFileCtrl.filecrc = MAKE_SHORT(preq->filecrc);
	DownFileCtrl.blast = 0;
	DownFileCtrl.lastlen = 0;

	if(DownFileCtrl.blksize < 50 || DownFileCtrl.blksize > (UPLINK_RCVMAX(itf) - LEN_DWNDATA_HEAD)) goto mark_fail;
	if(0 == DownFileCtrl.winsize || DownFileCtrl.winsize > 32) goto mark_fail;
	if(32 == DownFileCtrl.winsize) DownFileCtrl.full_mask = 0xffffffff;
	else DownFileCtrl.full_mask = ((unsigned int)1<<(DownFileCtrl.winsize)) -1;
	if(DownFileCtrl.fileid > FILEID_DRIVER) goto mark_fail;

	memsize = (unsigned long)DownFileCtrl.blksize * (unsigned long)DownFileCtrl.winsize;
	DownFileCtrl.pmem = malloc(memsize);
	if(NULL == DownFileCtrl.pmem) {
		ErrorLog("malloc %d bytes fail\n", memsize);
		goto mark_fail;
	}

	remove(DWNFILE_CACHE);
	DownFileCtrl.pfcache = fopen(DWNFILE_CACHE, "wb");
	if(NULL == DownFileCtrl.pfcache) {
		ErrorLog("open %s fail\n", DWNFILE_CACHE);
		free(DownFileCtrl.pmem);
		DownFileCtrl.pmem = NULL;
		goto mark_fail;
	}

	timeout = MAKE_LONG(preq->timeout);
	DownFileCtrl.timerid = SysAddCTimer(timeout, CTimerDownFile, 0);
	if(DownFileCtrl.timerid < 0) {
		ErrorLog("add ctimer %d sec fail\n", timeout);
		ClearDownFileCtrl();
		goto mark_fail;
	}

	DownFileCtrl.snmax = DownFileCtrl.filesize / DownFileCtrl.blksize;
	if(DownFileCtrl.filesize % DownFileCtrl.blksize) DownFileCtrl.snmax++;
	DownFileCtrl.snmax--;

	DownFileCtrl.svr_addr = prcv->grp & 0xfe;

	PrintLog(LOGTYPE_SHORT, "start download work file: len=%d, timeout=%d, snmax=%d\n", 
			DownFileCtrl.filesize, timeout, DownFileCtrl.snmax);

	pecho->code = FILECODE_REQ_DWNWORK;
	SvrEchoSend(itf, psnd);
	return;

mark_fail:
	pecho->code = FILECODE_REJECT;
	UPLINKAPP_LEN(psnd) = 5;
	SvrEchoSend(itf, psnd);
}

/**
* @brief 请求下载普通文件
* @param itf 接口编号
*/
static void ReqDownRaw(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	struct req_dwnraw *preq = (struct req_dwnraw *)(prcv->data);
	struct req_dwnraw *pecho = (struct req_dwnraw *)(psnd->data);
	unsigned long timeout, memsize;
	int namelen;

	if(UPLINKAPP_LEN(prcv) <= LEN_REQ_DWNRAW) return;

	smallcpy(psnd->data, prcv->data, LEN_REQ_DWNRAW);
	UPLINKAPP_LEN(psnd) = LEN_REQ_DWNRAW;

	if(TRANFILE_REQ_SN != MAKE_LONG(preq->req)) goto mark_fail;

	ClearDownFileCtrl();

	DownFileCtrl.flag = DWNFLAG_RAWFILE;;
	DownFileCtrl.filesize = MAKE_LONG(preq->size);
	DownFileCtrl.serial = 0;
	DownFileCtrl.mask = 0;
	DownFileCtrl.winsize = preq->winsize;
	DownFileCtrl.blksize = MAKE_SHORT(preq->blksize);
	DownFileCtrl.filecrc = MAKE_SHORT(preq->filecrc);
	DownFileCtrl.blast = 0;
	DownFileCtrl.lastlen = 0;

	if(DownFileCtrl.blksize < 50 || DownFileCtrl.blksize > (UPLINK_RCVMAX(itf) - LEN_DWNDATA_HEAD)) goto mark_fail;
	if(0 == DownFileCtrl.winsize || DownFileCtrl.winsize > 32) goto mark_fail;
	if(32 == DownFileCtrl.winsize) DownFileCtrl.full_mask = 0xffffffff;
	else DownFileCtrl.full_mask = ((unsigned int)1<<(DownFileCtrl.winsize)) - 1;

	*(prcv->data + UPLINKAPP_LEN(prcv)) = 0;
	namelen = strlen(preq->filename) + 1;
	if((namelen+LEN_REQ_DWNRAW) > UPLINKAPP_LEN(prcv) || namelen > 64) goto mark_fail;
	strcpy(DownFileCtrl.filename, (char *)preq->filename);

	memsize = (unsigned long)DownFileCtrl.blksize * (unsigned long)DownFileCtrl.winsize;
	DownFileCtrl.pmem = malloc(memsize);
	if(NULL == DownFileCtrl.pmem) {
		ErrorLog("malloc %d bytes fail\n", memsize);
		goto mark_fail;
	}

	remove(DWNFILE_CACHE);
	DownFileCtrl.pfcache = fopen(DWNFILE_CACHE, "wb");
	if(NULL == DownFileCtrl.pfcache) {
		ErrorLog("open %s fail\n", DWNFILE_CACHE);
		free(DownFileCtrl.pmem);
		DownFileCtrl.pmem = NULL;
		goto mark_fail;
	}

	timeout = MAKE_LONG(preq->timeout);
	//printf("ReqDownRaw.........\n");
	//printf("timeout  = %d\n",timeout);
	timeout = 45;
	DownFileCtrl.timerid = SysAddCTimer(timeout, CTimerDownFile, 0);
	if(DownFileCtrl.timerid < 0) {
		ErrorLog("add ctimer %d sec fail\n", timeout);
		ClearDownFileCtrl();
		goto mark_fail;
	}

	DownFileCtrl.snmax = DownFileCtrl.filesize / DownFileCtrl.blksize;
	if(DownFileCtrl.filesize % DownFileCtrl.blksize) DownFileCtrl.snmax++;
	DownFileCtrl.snmax--;

	DownFileCtrl.svr_addr = prcv->grp & 0xfe;

	PrintLog(LOGTYPE_SHORT, "start download raw file: len=%d, timeout=%d, snmax=%d\n", 
			DownFileCtrl.filesize, timeout, DownFileCtrl.snmax);

	pecho->code = FILECODE_REQ_DWNRAW;
	SvrEchoSend(itf, psnd);
	return;

mark_fail:
	pecho->code = FILECODE_REJECT;
	UPLINKAPP_LEN(psnd) = 5;
	SvrEchoSend(itf, psnd);
}

/**
* @brief 下载文件数据
* @param itf 接口编号
*/
static void DownloadData(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	struct down_data *pdwn = (struct down_data *)(prcv->data);
	struct down_data *pecho = (struct down_data *)(psnd->data);
	unsigned short serial;
	unsigned int len, offset, mask;

	len = (unsigned int)UPLINKAPP_LEN(prcv)&0xffff;
	if(len < LEN_DWNDATA_HEAD) return;

	smallcpy(psnd->data, prcv->data, LEN_DWNDATA_HEAD);
	UPLINKAPP_LEN(psnd) = LEN_DWNDATA_HEAD;
	/*DebugPrint(LOGTYPE_SHORT, "dwn serial=%02X%02X\n", pdwn->serial[1], pdwn->serial[0]);
	DebugPrint(LOGTYPE_SHORT, "echo serial=%02X%02X\n", pecho->serial[1], pecho->serial[0]);
	DebugPrint(0, "rdata=%08X, dwn=%08X; sdata=%08X, echo=%08X\n", prcv->data, pdwn, psnd->data, pecho);*/

	if(NULL == DownFileCtrl.pmem || NULL == DownFileCtrl.pfcache) {
		pecho->code = FILECODE_REJECT;
		UPLINKAPP_LEN(psnd) = 5;
		SvrEchoSend(itf, psnd);
		return;
	}

	len -= LEN_DWNDATA_HEAD;

	serial = MAKE_SHORT(pdwn->serial);
	//DebugPrint(LOGTYPE_SHORT, "serial=%d:%d, len=%d\n", serial, DownFileCtrl.serial, len);
	if(serial > DownFileCtrl.snmax) return;
	if(len > DownFileCtrl.blksize) return;
	if(len < DownFileCtrl.blksize && serial != DownFileCtrl.snmax) return;
	if(serial >= (DownFileCtrl.serial + DownFileCtrl.winsize)) return;

	{
		unsigned short crc1, crc2;

		crc1 = CalculateCRC(pdwn->data, len);
		crc2 = MAKE_SHORT(pdwn->crc);
		if(crc1 != crc2) return;
	}
	//DebugPrint(LOGTYPE_SHORT, "crc ok\n");

	if(serial < DownFileCtrl.serial) {
		SvrEchoSend(itf, psnd);
		return;
	}

	SysClearCTimer(DownFileCtrl.timerid);

	if(serial == DownFileCtrl.snmax) {
		DownFileCtrl.lastlen = len;
		DownFileCtrl.blast = 1;
		offset = serial - DownFileCtrl.serial;
		if(offset < 31) {
			mask = 1<<(offset+1);
			mask -= 1;
			DownFileCtrl.full_mask = mask;
		}
	}

	serial -= DownFileCtrl.serial;
	mask = 1<<serial;
	offset = (unsigned int)DownFileCtrl.blksize * (unsigned int)serial;
	memcpy(DownFileCtrl.pmem + offset, pdwn->data, len);
	DownFileCtrl.mask |= mask;

	if(DownFileCtrl.mask != DownFileCtrl.full_mask) {
		SvrEchoSend(itf, psnd);
		return;
	}

	//DebugPrint(LOGTYPE_SHORT, "full\n");

	if(!DownFileCtrl.blast) len =  (unsigned int)DownFileCtrl.blksize * DownFileCtrl.winsize;
	else len = (unsigned int)DownFileCtrl.blksize*(DownFileCtrl.snmax-DownFileCtrl.serial)+DownFileCtrl.lastlen;
	//else len = (unsigned int)DownFileCtrl.blksize * (DownFileCtrl.winsize-1) + DownFileCtrl.lastlen;
	fwrite(DownFileCtrl.pmem, len, 1, DownFileCtrl.pfcache);

	if(!DownFileCtrl.blast) {
		DownFileCtrl.serial += DownFileCtrl.winsize;
		DownFileCtrl.mask = 0;
		SvrEchoSend(itf, psnd);
		return;
	}

	/**end**/
	//DebugPrint(LOGTYPE_SHORT, "end\n");

	SysStopCTimer(DownFileCtrl.timerid);
	DownFileCtrl.timerid = -1;
	fclose(DownFileCtrl.pfcache);
	DownFileCtrl.pfcache = NULL;

	{
		FILE *pf;
		unsigned short crc;
		int readlen;
		unsigned int memsize;
		char cmd[128];

		pf = fopen(DWNFILE_CACHE, "rb");
		if(NULL == pf) {
			ErrorLog("can not open %s for read\n", DWNFILE_CACHE);
			goto mark_end_fail;
		}

		crc = 0;
		len = 0;
		memsize = (unsigned int)DownFileCtrl.blksize * (unsigned int)DownFileCtrl.winsize;
		while(1) {
			readlen = fread(DownFileCtrl.pmem, 1, memsize, pf);
			if(readlen > 0) {
				len += readlen;
				CalculateCRCStep(DownFileCtrl.pmem, readlen, &crc);
			}
			if(readlen < (int)memsize) break;
		}

		if(len != DownFileCtrl.filesize) {
			PrintLog(LOGTYPE_SHORT, "file size incorrect(%d:%d)\n", len, DownFileCtrl.filesize);
			fclose(pf);
			goto mark_end_fail;
		}

		if(crc != DownFileCtrl.filecrc) {
			PrintLog(LOGTYPE_SHORT, "file crc incorrect(%04X:%04X)\n", crc, DownFileCtrl.filecrc);
			fclose(pf);
			goto mark_end_fail;
		}

		fclose(pf);

		if(DWNFLAG_WORKFILE == DownFileCtrl.flag) 
		{

			//bakup file
			sprintf(cmd, "%s%s.bak", WORK_PATH, WorkFileName[DownFileCtrl.fileid]);
			remove(cmd);
			//DebugPrint(0, "remove %s\n", cmd);
			sprintf(cmd, "mv %s%s %s%s.bak", 
				WORK_PATH, WorkFileName[DownFileCtrl.fileid],
				WORK_PATH, WorkFileName[DownFileCtrl.fileid]);
			//DebugPrint(0, "%s\n", cmd);
			system(cmd);

			//remove
			sprintf(cmd, "%s%s", WORK_PATH, WorkFileName[DownFileCtrl.fileid]);
			//DebugPrint(0, "remove %s\n", cmd);
			remove(cmd);

			//copy
			sprintf(cmd, "cp %s %s%s",  DWNFILE_CACHE, WORK_PATH, WorkFileName[DownFileCtrl.fileid]);
			//DebugPrint(0, "%s\n", cmd);
			system(cmd);

		#ifndef DWNFILE_SAVEBAK
			//del bakup
			sprintf(cmd, "%s%s.bak", WORK_PATH, WorkFileName[DownFileCtrl.fileid]);
			remove(cmd);
		#endif
			SetSoftChange(SOFTCHG_VERSION);
		}
		else if(DWNFLAG_RAWFILE == DownFileCtrl.flag) {

			//remove
			remove(DownFileCtrl.filename);

			//copy
			sprintf(cmd, "cp %s %s",  DWNFILE_CACHE, DownFileCtrl.filename);
			system(cmd);
		}
		else goto mark_end_fail;
	}

	PrintLog(LOGTYPE_SHORT, "down file end ok\n");

	printf("rm -r -f /home/nandflash/work/whxcjzq.out\n");
	system("rm -r -f /home/nandflash/work/whxcjzq.out");
	Sleep(10);
	printf("cp -r -f /home/nandflash/work/dwnfile.tmp /home/nandflash/work/whxcjzq.out\n");
	system("cp -r -f /home/nandflash/work/dwnfile.tmp /home/nandflash/work/whxcjzq.out");
	Sleep(10);
	printf("chmod +x /home/nandflash/work/whxcjzq.out\n");
	system("chmod +x /home/nandflash/work/whxcjzq.out");
	Sleep(10);
	printf("mv /home/param/conf/watchdog.conf /home/param/conf/watchdog.conf.bak\n");
	system("mv /home/param/conf/watchdog.conf /home/param/conf/watchdog.conf.bak");
	Sleep(10);
	printf("rm -r -f  /home/run/watchdog\n");
	system("rm -r -f  /home/run/watchdog");
	Sleep(10);
	printf("mv /home/run/watchdog /home/run/watchdog.bak\n");
	system("mv /home/run/watchdog /home/run/watchdog.bak");
	Sleep(10);
	printf("ln /home/nandflash/work/whxcjzq.out /home/run/watchdog\n");
	system("ln /home/nandflash/work/whxcjzq.out /home/run/watchdog");
	Sleep(10);
	printf("cp -r -f /home/nandflash/work/whxcjzq.out /home/run/watchdog\n");
	system("cp -r -f /home/nandflash/work/whxcjzq.out /home/run/watchdog");
	SavePlMdb();

	ClearDownFileCtrl();
	SvrEchoSend(itf, psnd);

	Sleep(500);
	SysRestart();
	return;

mark_end_fail:
	ClearDownFileCtrl();
	pecho->code = FILECODE_REJECT;
	UPLINKAPP_LEN(psnd) = 5;
	SvrEchoSend(itf, psnd);
}

/**
* @brief 上传文件超时定时器
*/
static int CTimerUpFile(unsigned long arg)
{
	PrintLog(LOGTYPE_ALARM, "up file timeout\n");

	UpFileCtrl.timerid = -1;
	ClearUpFileCtrl();

	return 1;
}

/**
* @brief 请求上传普通文件
* @param itf 接口编号
*/
static void ReqReadRaw(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	struct req_upraw *preq = (struct req_upraw *)(prcv->data);
	struct req_upraw *pecho = (struct req_upraw *)(psnd->data);
	unsigned long timeout;
	int len, i;
	unsigned short crc;

	if(UPLINKAPP_LEN(prcv) <= LEN_REQ_UPRAW) return;

	smallcpy(psnd->data, prcv->data, LEN_REQ_UPRAW);
	UPLINKAPP_LEN(psnd) = LEN_REQ_UPRAW;

	if(TRANFILE_REQ_SN != MAKE_LONG(preq->req)) goto mark_fail;

	ClearUpFileCtrl();

	UpFileCtrl.serial = 0;
	UpFileCtrl.blksize = MAKE_SHORT(preq->blksize);
	UpFileCtrl.memsize = UpFileCtrl.blksize;

	if(UpFileCtrl.blksize < 50 || UpFileCtrl.blksize > (UPLINK_RCVMAX(itf) - LEN_DWNDATA_HEAD)) goto mark_fail;

	*(prcv->data + UPLINKAPP_LEN(prcv)) = 0;
	len = strlen(preq->filename) + 1;
	if((len+LEN_REQ_UPRAW) > UPLINKAPP_LEN(prcv)) goto mark_fail;

	UpFileCtrl.pmem = malloc((unsigned long)UpFileCtrl.blksize);
	if(NULL == UpFileCtrl.pmem) {
		ErrorLog("malloc %d bytes fail\n", UpFileCtrl.blksize);
		goto mark_fail;
	}

	UpFileCtrl.pf = fopen(preq->filename, "rb");
	if(NULL == UpFileCtrl.pf) {
		ErrorLog("open %s fail\n", preq->filename);
		free(UpFileCtrl.pmem);
		UpFileCtrl.pmem = NULL;
		goto mark_fail;
	}

	crc = 0;
	len = 0;
	while(1) {
		i = fread(UpFileCtrl.pmem, 1, UpFileCtrl.blksize, UpFileCtrl.pf);
		if(i > 0) {
			CalculateCRCStep(UpFileCtrl.pmem, i, &crc);
			len += i;
		}
		if(i < UpFileCtrl.blksize) break;
	}
	DEPART_LONG(len, pecho->size);
	DEPART_SHORT(crc, pecho->filecrc);
	fseek(UpFileCtrl.pf, 0, SEEK_SET);

	timeout = MAKE_LONG(preq->timeout);
	UpFileCtrl.timerid = SysAddCTimer(timeout, CTimerUpFile, 0);
	if(UpFileCtrl.timerid < 0) {
		ErrorLog("add ctimer %d sec fail\n", timeout);
		ClearUpFileCtrl();
		goto mark_fail;
	}

	PrintLog(LOGTYPE_SHORT, "start upload file: len=%d, timeout=%d\n", len, timeout);

	pecho->code = FILECODE_REQ_UPRAW;
	SvrEchoSend(itf, psnd);
	return;

mark_fail:
	pecho->code = FILECODE_REJECT;
	UPLINKAPP_LEN(psnd) = 5;
	SvrEchoSend(itf, psnd);
}

/**
* @brief 上传文件传输数据
* @param itf 接口编号
*/
static void UploadData(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	struct down_data *pdwn = (struct down_data *)(prcv->data);
	struct down_data *pecho = (struct down_data *)(psnd->data);
	unsigned short serial, crc;
	int len;

	len = (unsigned int)UPLINKAPP_LEN(prcv)&0xffff;
	if(len < LEN_DWNDATA_HEAD) return;

	smallcpy(psnd->data, prcv->data, LEN_DWNDATA_HEAD);
	UPLINKAPP_LEN(psnd) = LEN_DWNDATA_HEAD;

	if(NULL == UpFileCtrl.pmem || NULL == UpFileCtrl.pf) goto mark_fail;

	serial = MAKE_SHORT(pdwn->serial);
	if(serial == (UpFileCtrl.serial+1)) {
		if(UpFileCtrl.memsize < UpFileCtrl.blksize) { //end
			PrintLog(LOGTYPE_SHORT, "upload file end\n");
			ClearUpFileCtrl();
			return;
		}

		len = fread(UpFileCtrl.pmem, 1, UpFileCtrl.blksize, UpFileCtrl.pf);
		if(len <= 0) goto mark_end_fail;

		UpFileCtrl.memsize = len;
		UpFileCtrl.serial++;
	}

	if(serial != UpFileCtrl.serial) goto mark_end_fail;

	SysClearCTimer(UpFileCtrl.timerid);

	memcpy(pecho->data, UpFileCtrl.pmem, UpFileCtrl.memsize);
	UPLINKAPP_LEN(psnd) = UpFileCtrl.memsize + LEN_DWNDATA_HEAD;
	crc = CalculateCRC(UpFileCtrl.pmem, UpFileCtrl.memsize);
	DEPART_SHORT(crc, pecho->crc);

	SvrEchoSend(itf, psnd);
	return;

mark_end_fail:
	ClearUpFileCtrl();
mark_fail:
	pecho->code = FILECODE_REJECT;
	UPLINKAPP_LEN(psnd) = 5;
	SvrEchoSend(itf, psnd);
	return;
}

/**
* @brief 上传文件传输中止
* @param itf 接口编号
*/
static void UpLoadReject(unsigned char itf)
{
	ClearUpFileCtrl();
}

/**
* @brief 复制文件下载进程信息
* @param pifno 复制缓存区指针
*/
static inline void CopyFileInfo(struct dfile_info *pinfo)
{
	pinfo->duid[0] = pinfo->duid[1] = 0;
	pinfo->duid[2] = 0x02;  //F10
	pinfo->duid[3] = 0x01;
	pinfo->code = FILECODE_INFO;

	DEPART_LONG(DownFileCtrl.filesize, pinfo->filesize);
	DEPART_SHORT(DownFileCtrl.serial, pinfo->serial);
	DEPART_LONG(DownFileCtrl.mask, pinfo->mask);
	DEPART_SHORT(DownFileCtrl.blksize, pinfo->blksize);
	DEPART_SHORT(DownFileCtrl.filecrc, pinfo->filecrc);
	pinfo->fileid = DownFileCtrl.fileid;
	pinfo->winsize = DownFileCtrl.winsize;
}

/**
* @brief 主动发送文件下载进程信息(登录后发送)
* @param itf 接口编号
*/
void SvrSendFileInfo(unsigned char itf)
{
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	struct dfile_info *pinfo = (struct dfile_info *)(psnd->data);

	if(NULL == DownFileCtrl.pmem || NULL == DownFileCtrl.pfcache) return;

	psnd->ctrl = UPCMD_USRDATA|UPCTRL_PRM;
	psnd->afn = UPAFN_TRANFILE;
	UPLINKAPP_LEN(psnd) = sizeof(struct dfile_info);

	CopyFileInfo(pinfo);
	psnd->seq = UPSEQ_SPKT;
	psnd->grp = DownFileCtrl.svr_addr;

	UplinkSendPkt(itf, psnd);
}

/**
* @brief 处理读取文件下载进程信息请求
* @param itf 接口编号
*/
static void SvrDownFileInfo(unsigned char itf)
{
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	struct dfile_info *pinfo = (struct dfile_info *)(psnd->data);

	if(NULL == DownFileCtrl.pmem || NULL == DownFileCtrl.pfcache) return;

	psnd->ctrl = UPCMD_USRDATA;
	psnd->afn = UPAFN_TRANFILE;
	UPLINKAPP_LEN(psnd) = sizeof(struct dfile_info);

	CopyFileInfo(pinfo);

	SvrEchoSend(itf, psnd);
}

typedef void (*dealfunc_t)(unsigned char itf);
static const dealfunc_t DealFunc[FILECODE_MAX] = {
	ReqDownWork,	/*FILECODE_REQ_DWNWORK*/
	ReqDownRaw,		/*FILECODE_REQ_DWNRAW*/
	ReqReadRaw,		/*FILECODE_REQ_UPRAW*/
	DownloadData,	/*FILECODE_DWNDATA*/
	UploadData,		/*FILECODE_UPDATA*/
	UpLoadReject,	/*FILECODE_REJECT*/
	SvrDownFileInfo, /*FILECODE_INFO*/
};

/**
* @brief 处理自定义文件传输
* @param itf 接口编号
*/
void SvrSelfTranFile(unsigned char itf)
{
	uplink_pkt_t *prcv = (uplink_pkt_t *)UPLINK_RCVBUF(itf);
	uplink_pkt_t *psnd = (uplink_pkt_t *)UPLINK_SNDBUF(itf);
	unsigned char code;

	if(UPLINKAPP_LEN(prcv) < 5) return;

	code = *(prcv->data + 4);
	if(code >= FILECODE_MAX) return;

	psnd->ctrl = UPECHO_USRDATA;
	psnd->afn = UPAFN_TRANFILE;

	if(NULL != DealFunc[code]) (DealFunc[code])(itf);
}

/**
* @brief 自定义文件传输初始化
*/
DECLARE_INIT_FUNC(SvrSelfTranFileInit);
int SvrSelfTranFileInit(void)
{
	DownFileCtrl.timerid = -1;
	DownFileCtrl.pmem = NULL;
	DownFileCtrl.pfcache = NULL;

	UpFileCtrl.timerid = -1;
	UpFileCtrl.pmem = NULL;
	UpFileCtrl.pf = NULL;

	SET_INIT_FLAG(SvrSelfTranFileInit);

	return 0;
}

static int  shell_ftp_tranfile(int argc, char *argv[])
{
	printf("rm -r -f /home/nandflash/work/whxcjzq.out\n");
	system("rm -r -f /home/nandflash/work/whxcjzq.out");
	Sleep(10);
	printf("mv /home/nandflash/work/whxcjzq.out1 /home/nandflash/work/whxcjzq.out\n");
	system("mv /home/nandflash/work/whxcjzq.out1 /home/nandflash/work/whxcjzq.out");
	Sleep(10);
	printf("chmod +x /home/nandflash/work/whxcjzq.out\n");
	system("chmod +x /home/nandflash/work/whxcjzq.out");
	Sleep(10);
	printf("mv /home/param/conf/watchdog.conf /home/param/conf/watchdog.conf.bak\n");
	system("mv /home/param/conf/watchdog.conf /home/param/conf/watchdog.conf.bak");
	Sleep(10);
	printf("rm -r -f  /home/run/watchdog\n");
	system("rm -r -f  /home/run/watchdog");
	Sleep(10);
	printf("mv /home/run/watchdog /home/run/watchdog.bak\n");
	system("mv /home/run/watchdog /home/run/watchdog.bak");
	Sleep(10);
	printf("ln /home/nandflash/work/whxcjzq.out /home/run/watchdog\n");
	system("ln /home/nandflash/work/whxcjzq.out /home/run/watchdog");
	Sleep(10);
	printf("cp -r -f /home/nandflash/work/whxcjzq.out /home/run/watchdog\n");
	system("cp -r -f /home/nandflash/work/whxcjzq.out /home/run/watchdog");
	SavePlMdb();
	Sleep(500);
	SysRestart();

	return 0;
}

DECLARE_SHELL_CMD("plftpshell", shell_ftp_tranfile, "FTP传输完成批量执行shell");

