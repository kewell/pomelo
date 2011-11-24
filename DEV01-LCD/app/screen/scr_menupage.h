/**
* scr_menupage.h -- 显示翻页菜单头文件
* 
* 
* 创建时间: 2010-5-20
* 最后修改时间: 2010-7-10
*/

#ifndef _SCR_MENUPAGE_H
#define _SCR_MENUPAGE_H

#define UCHAR     unsigned char
#define ULONG     unsigned long
void ScrpCheckState(unsigned short mid, ULONG arg, UCHAR key);
void ScrpPlmMetDayData(unsigned short mid, ULONG arg, UCHAR key);
void ScrpImportantUserData(unsigned short mid, ULONG arg, UCHAR key);
void ScrpPlmInfo(unsigned short mid, ULONG arg, UCHAR key);
void ScrpVesionInfo(unsigned short mid, ULONG arg, UCHAR key);
void ScrpPortCfg(unsigned short mid, ULONG arg, UCHAR key);
void ScrpOtherCfg(unsigned short mid, ULONG arg, UCHAR key);
void ScrpCascadePara(unsigned short mid, ULONG arg, UCHAR key);
void ScrpPortPara(unsigned short mid, ULONG arg, UCHAR key);
void ScrpClass1DataTask(unsigned short mid, ULONG arg, UCHAR key);
void ScrpClass2DataTask(unsigned short mid, ULONG arg, UCHAR key);
void ScrpUserMetPara (unsigned short mid, ULONG arg, UCHAR key);
void ScrpMajorMetBasePara(unsigned short mid, ULONG arg, UCHAR key);
void ScrpMajorMetLmtPara(unsigned short mid, ULONG arg, UCHAR key);
void ScrpReadMetPara(unsigned short mid, ULONG arg, UCHAR key);
void ScrpTermParaCom(unsigned short mid, ULONG arg, UCHAR key);
void ScrpTermParaMix(unsigned short mid, ULONG arg, UCHAR key);
void ScrpEventReadCfg(unsigned short mid, ULONG arg, UCHAR key);
void ScrpHead(unsigned short mid, ULONG arg, UCHAR key);
void ScrpMetDataEne(unsigned short mid, ULONG arg, UCHAR key);
void ScrpMetDataDmn(unsigned short mid, ULONG arg, UCHAR key);
void ScrpMetDataImm(unsigned short mid, ULONG arg, UCHAR key);
void ScrpTermParaPort(unsigned short mid, ULONG arg, UCHAR key);
void ScrpTermParaMetProto(unsigned short mid, ULONG arg, UCHAR key);
void ScrpMetParaBase(unsigned short mid, ULONG arg, UCHAR key);
void ScrpMetParaAlm(unsigned short mid, ULONG arg, UCHAR key);

void ScrpLdParaBase(unsigned short mid, ULONG arg, UCHAR key);
void ScrpLdParaPc(unsigned short mid, ULONG arg, UCHAR key);
void ScrpLdParaEc(unsigned short mid, ULONG arg, UCHAR key);
void ScrpLdParaPrd(unsigned short mid, ULONG arg, UCHAR key);

void ScrpStatTerm(unsigned short mid, ULONG arg, UCHAR key);
void ScrpStatSvrcomm(unsigned short mid, ULONG arg, UCHAR key);
void ScrpStatMetComm(unsigned short mid, ULONG arg, UCHAR key);
void ScrpStatLd(unsigned short mid, ULONG arg, UCHAR key);
void ScrpStatPower(unsigned short mid, ULONG arg, UCHAR key);
void ScrpStatAlarm(unsigned short mid, ULONG arg, UCHAR key);
void ScrpTermTime(unsigned short mid, ULONG arg, UCHAR key);
void ScrpBatShow(unsigned short mid,ULONG arg,UCHAR key);
void ScrpClrenp(unsigned short mid,ULONG arg,UCHAR key);
void ScrpCwPercent(unsigned short mid,ULONG arg,UCHAR key);
void ScrpCwPercentPet(unsigned short mid,ULONG arg,UCHAR key);
void ScrpDmClr(unsigned short mid ,ULONG arg,UCHAR key);
void ScrpPro(unsigned short mid ,ULONG arg,UCHAR key);
void ScrpLosVol(unsigned short mid,ULONG arg,UCHAR key);
void ScrpLosAmp(unsigned short mid,ULONG arg,UCHAR key);
void ScrpTerminal(unsigned short mid,ULONG arg,UCHAR key);
void ScrpSetTerminalCfg(unsigned short mid,ULONG arg,UCHAR key);
void ScrpSetMetCfg(unsigned short mid,ULONG arg,UCHAR key);
void ScrpTempratureShow(unsigned short mid,ULONG arg,UCHAR key);
void ScrpPauniset(unsigned short mid,ULONG arg,UCHAR key);
void ScrpSetLoop(unsigned short mid,ULONG arg,UCHAR key);
void ScrpLoopMes(unsigned short mid,ULONG arg,UCHAR key);
void ScrpRunState(unsigned short mid, ULONG arg, UCHAR key);
void ScrpMeterCfg(unsigned short mid, ULONG arg, UCHAR key);
void ScrpReadMeterStat(unsigned short mid, ULONG arg, UCHAR key);
void ScrpRestartTerm(unsigned short mid, ULONG arg, UCHAR key);
void ScrShowTermPara(unsigned short mid, ULONG arg, UCHAR key);
void ScrpPlmMetDay(unsigned short mid, ULONG arg, UCHAR key);
//--------------------------------RS485_H----------------------------------------------//
#define RS485_BITMASK    0x03
#define RS485_BIT5   0x00
#define RS485_BIT6   0x01
#define RS485_BIT7   0x02
#define RS485_BIT8   0x03

#define RS485_STOPMASK    0x04
#define RS485_STOP1    0x00
#define RS485_STOP2    0x04

#define RS485_PARITYMASK    0x38
#define RS485_NOPARITY   0x00
#define RS485_ODDPARITY    0x08
#define RS485_EVENPARITY    0x18
#define RS485_1PARITY    0x30
#define RS485_0PARITY    0x38

#define MAXNUM_RS485    4

//--------------------------------主站通信接口----------------------------------------------//
//主站通信接口
#define SVRADDR_SMS    1    //短信
#define SVRADDR_GPRS    2   //GPRS/CDMA
#define SVRADDR_DTMF    3    //DTMF
#define SVRADDR_ETHERNET    4   //以太网
#define SVRADDR_IR    5    //红外
#define SVRADDR_RS232    6   //串口
#define SVRADDR_CSD    7    //GSM CSD
#define SVRADDR_RADIO    8    //无线
#define SVRADDR_SMSWAKE    9    //短信唤醒
#define SVRADDR_485LINK 10 //485级连(从)
#define SVRADDR_METCHK		11   //485校表


#define CEN_METER_NUMBER	0x01


#endif /*_SCR_PAGE_H*/

