/**
* unique.c -- 自定义参数
* 
* 
* 创建时间: 2010-5-18
* 最后修改时间: 2010-5-18
*/

#include <stdio.h>
#include <string.h>

#define DEFINE_PARAUNI

#include "param_config.h"
#include "include/debug.h"
#include "include/param/unique.h"
#include "include/sys/xin.h"

para_uni_t ParaUni;

static void LoadDefParaUni(void)
{
	ParaUni.uplink = 2;
	ParaUni.addr_area[0] = 0x55;
	ParaUni.addr_area[1] = 0x07;
	ParaUni.addr_sn[0] = 1;
	ParaUni.addr_sn[1] = 0;

	ParaUni.addr_mac[0] = 0x56;
	ParaUni.addr_mac[1] = 0x34;
	ParaUni.addr_mac[2] = 0x12;
	ParaUni.addr_mac[3] = 0x48;
	ParaUni.addr_mac[4] = 0x80;
	ParaUni.addr_mac[5] = 0x00;
}

int LoadParaUni(void)
{
	XINREF pf;
	unsigned char uc;

	DebugPrint(0, "  load param unique...");

	LoadDefParaUni();

	pf = XinOpen(UNIQUE_SAVE_PATH "unique.xin", 'r');
	if(NULL == pf) {
		DebugPrint(0, "no file\n");
		goto mark_read2;
	}

	ParaUni.uplink = XinReadInt(pf, "uplink", 2);
	ParaUni.gprs_mod_baud = XinReadInt(pf, "gprsmodbaud", 15200);

	DebugPrint(0, "ok\n");
	XinClose(pf);

mark_read2:

	DebugPrint(0, "  load param manuf...");

	pf = XinOpen(UNIQUE_SAVE_PATH "manuf.xin", 'r');
	if(NULL == pf) {
		DebugPrint(0, "no file\n");
		return 1;
	}
	XinReadString(pf, "cmply", ParaUni.cmply,2);
	if(XinReadHex(pf, "addr", ParaUni.addr_area, 4) > 0) {
		uc = ParaUni.addr_area[0];
		ParaUni.addr_area[0] = ParaUni.addr_area[1];
		ParaUni.addr_area[1] = uc;
		uc = ParaUni.addr_sn[0];
		ParaUni.addr_sn[0] = ParaUni.addr_sn[1];
		ParaUni.addr_sn[1] = uc;
	}

	if(XinReadHex(pf, "mac", ParaUni.addr_mac, 6) > 0) {
		ParaUni.addr_mac[0] = 0x00;
		ParaUni.addr_mac[1] = 0x80;
		ParaUni.addr_mac[2] = 0x48;
	}

	XinReadString(pf, "manuno", ParaUni.manuno, 14);
	XinReadString(pf, "manuin", ParaUni.manuno_inner, 12);

	DebugPrint(0, "ok\n");
	XinClose(pf);

	return 0;
}



/**
* @brief 保存终端参数
* @return 0成功, 否则失败
*/
int SaveParaManuf(void)
{
	XINREF pf;
	int bakup = 0;
	//char str[24];
	unsigned char addr_area_sn[4];

	printf("SaveParaManuf.......\n");
	pf = XinOpen(UNIQUE_SAVE_PATH "manuf.xin", 'w');
	if(NULL == pf) goto mark_bakup;

mark_save:
	XinWriteString(pf, "cmply", ParaUni.cmply);
	addr_area_sn[0] = ParaUni.addr_area[1];
	addr_area_sn[1] = ParaUni.addr_area[0];
	addr_area_sn[2] = ParaUni.addr_sn[1];
	addr_area_sn[3] = ParaUni.addr_sn[0];
	XinWriteHex(pf, "addr", addr_area_sn, 4);
	XinWriteHex(pf, "mac", ParaUni.addr_mac, 6);
	XinWriteString(pf, "manuno", ParaUni.manuno);
	XinWriteString(pf, "manuin", ParaUni.manuno_inner);
	XinClose(pf);

mark_bakup:
	if(bakup) return 0;
	bakup = 1;
	pf = XinOpen(PARAM_BAK_PATH "manuf.xin", 'w');
	if(NULL == pf) return 1;
	goto mark_save;
}

/**
* @brief 终端参数F85操作
*/
int ParaOperationF85(int flag, unsigned short metpid, unsigned char *buf, int len, int *actlen)
{
	printf("ParaOperationF85...................\n");
	if(0 == flag) 
	{
		smallcpy(buf, ParaUni.cmply,6);
		//smallcpy(buf, ParaUni.addr_area,2);
		//smallcpy(buf + 2, ParaUni.addr_sn,2);
	}
	else 
	{
		smallcpy( ParaUni.cmply,buf,6);
		//smallcpy(ParaUni.addr_area, buf, 2);
		//smallcpy(ParaUni.addr_sn, buf + 2, 2);
	}

	return 0;
}

