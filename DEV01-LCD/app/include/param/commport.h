/**
* commport.h -- Í¨ĞÅ¶Ë¿Ú²ÎÊıÍ·ÎÄ¼ş
* 
* 
* ´´½¨Ê±¼ä: 2010-5-8
* ×îºóĞŞ¸ÄÊ±¼ä: 2010-5-8
*/

#ifndef _PARAM_COMMPORT_H
#define _PARAM_COMMPORT_H

#define COMMFRAME_BAUD_300		0
#define COMMFRAME_BAUD_600		0x20
#define COMMFRAME_BAUD_1200		0x40
#define COMMFRAME_BAUD_2400		0x60
#define COMMFRAME_BAUD_4800		0x80
#define COMMFRAME_BAUD_7200		0xa0
#define COMMFRAME_BAUD_9600		0xc0
#define COMMFRAME_BAUD_19200	0xe0

#define COMMFRAME_STOPBIT_1		0
#define COMMFRAME_STOPBIT_2		0x10

#define COMMFRAME_NOCHECK		0
#define COMMFRAME_HAVECHECK		0x08

#define COMMFRAME_ODDCHECK		0
#define COMMFRAME_EVENCHECK		0x04

#define COMMFRAME_DATA_5		0
#define COMMFRAME_DATA_6		1
#define COMMFRAME_DATA_7		2
#define COMMFRAME_DATA_8		3

//DL698¹æÔ¼Ó³Éä¶Ë¿Ú
//#define COMMPORT_PLC		0  //ÔØ²¨¶Ë¿Ú//F33ÉèÖÃÊ±¶ÔÓ¦¶Ë¿Ú1
#define COMMPORT_PLC		30  //ÔØ²¨¶Ë¿Ú//F33ÉèÖÃÊ±¶ÔÓ¦¶Ë¿Ú1
#define COMMPORT_RS485_1	1  //¼¶Áª¶Ë¿Ú 
#define COMMPORT_RS485_2	2  //³­×Ü±í¶Ë¿Ú//F33ÉèÖÃÊ±¶ÔÓ¦¶Ë¿Ú3
#define COMMPORT_RS485_3	3  //³­±í¶Ë¿Ú

#define PLC_PORT				(COMMPORT_PLC + 1)
//#define PLC_PORT				31
//#define CEN_METER_PORT		(COMMPORT_RS485_2 + 1)
#define CEN_METER_PORT		2


//#define MAX_COMMPORT		4
#define MAX_COMMPORT		31

//Êµ¼ÊÓ²¼ş485¶Ë¿Ú
#define COMMPORT_CASCADE	(COMMPORT_RS485_1-1)
#define COMMPORT_CENMET		(COMMPORT_RS485_2-1)
#define COMMPORT_485BUS		(COMMPORT_RS485_3-1)

#define RDMETFLAG_ENABLE		0x0001  //ÖÃ"1"²»ÔÊĞí×Ô¶¯³­±í£¬ÖÃ"0" ÒªÇóÖÕ¶Ë¸ù¾İ³­±íÊ±¶Î×Ô¶¯³­±í
#define RDMETFLAG_ALL			0x0002  //ÖÃ"1"ÒªÇóÖÕ¶ËÖ»³­ÖØµã±í£¬ÖÃ"0"ÒªÇóÖÕ¶Ë³­ËùÓĞ±í
#define RDMETFLAG_FREZ			0x0004  //ÒªÇóÖÕ¶Ë²ÉÓÃ¹ã²¥¶³½á³­±í£¬ÖÃ"0"²»ÒªÇó
#define RDMETFLAG_CHECKTIME		0x0008  //ÖÃ"1"ÒªÇóÖÕ¶Ë¶¨Ê±¶Ôµç±í¹ã²¥Ğ£Ê±£¬ÖÃ"0"²»ÒªÇó
#define RDMETFLAG_FINDMET		0x0010  //ÖÃ"1"ÒªÇóÖÕ¶ËËÑÑ°ĞÂÔö»ò¸ü»»µÄµç±í£¬ÖÃ"0"²»ÒªÇó
#define RDMETFLAG_RDSTATUS		0x0020  //ÖÃ"1"ÒªÇóÖÕ¶Ë³­¶Á"µç±í×´Ì¬×Ö"£¬ÖÃ"0"²»ÒªÇó

//F33 ÖÕ¶Ë³­±íÔËĞĞ²ÎÊıÉèÖÃ
//F34 ÓëÖÕ¶Ë½Ó¿ÚµÄÍ¨ĞÅÄ£¿éµÄ²ÎÊıÉèÖÃ
#define MAXNUM_PERIOD	24
typedef struct {
	unsigned char hour_start;
	unsigned char min_start;
	unsigned char hour_end;
	unsigned char min_end;
}cfg_period_t;

typedef struct {
	unsigned short flag;  //Ì¨Çø¼¯ÖĞ³­±íÔËĞĞ¿ØÖÆ×Ö
	unsigned char time_hour;	//³­±íÈÕ-Ê±¼ä:Ê±
	unsigned char time_minute;  //³­±íÈÕ-Ê±¼ä:·Ö
	unsigned int dateflag;  //³­±íÈÕ-ÈÕÆÚ   ×Ü±í
	unsigned char cycle;  //³­±í¼ä¸ôÊ±¼ä, ·Ö, 1~60  ×Ü±í
	unsigned char chktime_day;     //¶Ôµç±í¹ã²¥Ğ£Ê±¶¨Ê±Ê±¼ä:ÈÕ
	unsigned char chktime_hour;    //¶Ôµç±í¹ã²¥Ğ£Ê±¶¨Ê±Ê±¼ä:Ê±
	unsigned char chktime_minute;  //¶Ôµç±í¹ã²¥Ğ£Ê±¶¨Ê±Ê±¼ä:·Ö
	unsigned char periodnum;	//ÔÊĞí³­±íÊ±¶ÎÊı
	unsigned char reserv;
	cfg_period_t period[MAXNUM_PERIOD];  //ÔÊĞí³­±íÊ±¶ÎÊ  ×Ü±í
	
	unsigned int baudrate;  //ÓëÖÕ¶Ë½Ó¿Ú¶ÔÓ¦¶ËµÄÍ¨ĞÅËÙÂÊ£¨bps£©
	unsigned char frame;  //ÓëÖÕ¶Ë½Ó¿Ú¶ËµÄÍ¨ĞÅ¿ØÖÆ×Ö
} para_commport_t;

const para_commport_t *GetParaCommPort(unsigned int port);

#endif /*_PARAM_COMMPORT_H*/

