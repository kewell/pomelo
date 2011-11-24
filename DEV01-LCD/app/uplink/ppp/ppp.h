#ifndef _PPP_H
#define _PPP_H

#ifndef ULONG
#define ULONG unsigned long
#endif
#ifndef USHORT
#define USHORT unsigned short
#endif
#ifndef UCHAR
#define UCHAR unsigned char
#endif

#include <unistd.h>
#include <string.h>

#include "pppdefs.h"
#include "pppconf.h"

#include "ppp_sys.h"

#include "lcp.h"
#include "chap.h"
#include "ipcp.h"

#include "ppp_debug.h"

#define MAXNAMELEN      256 /* max length of hostname or name for auth */
#define MAXSECRETLEN    256 /* max length of password or secret */

typedef struct {
	char   *ifDescr;
	long   ifadminstatus;
	long   ifoperstatus;
	long   starttime;
	long   mtu;
	long   speed;
	long   inoctets;
	long   inucastpkts;
	long   innucastpkts;
	long   indiscards;
	long   inerrors;
	long   inunknownprotos;
	long   outoctets;
	long   outucastpkts;
	long   outnucastpkts;
	long   outdiscards;
	long   outerrors;
	long   ifoutqlen;
	long   badaddress;
	long   badcontrols;
	long   packettoolongs;
	long   badfcs;
	long   localmru;
	long   remotemru;
	long   sendaccmap;
	long   recvaccmap;
	long   sendpcomp;
	long   recvpcomp;
	long   sendaccomp;
	long   recvaccomp;
} mib_stat;
extern mib_stat ppp_mibstat;

/*----------------------------------------------------------------------*/
/* Configuration structure for PPP                                      */
/*----------------------------------------------------------------------*/
typedef struct {
	long pppmode;                /* Mode for the PPP channel  */
	long dialmode;               /* Dial mode for the channel */
	long xfermode;               /* SYNC or ASYNC */
	char *user;                  /* User */
	char *passwd;                /* Password */
	unsigned long mru;           /* MRU of the channel */
	unsigned long asyncmap;      /*             */
	unsigned long lcp_options;   /* Various LCP options */
	unsigned long auth_options;  /* Various Auth Options */
	unsigned long local_ip;      /* Local IP address of the channel */
	unsigned long peer_ip;       /* Peer IP address */
	unsigned long ipcp_options;  /* Various IPCP options */
} ppp_cfg;

#define NEGMRU          0x1     /* Negotiate MRU */
#define NEGASYNCMAP     0x2     /* Negotiate async map */
#define NEGMAGIC        0x4     /* Negotiate magic number */
#define NEGPROTOCOMP    0x8     /* Negotiate protocol comp */
#define NEGACCOMP       0x10    /* Negotiate address/control comp */

#define REQUPAP         0x1    /* Negotiate PAP */
#define REQCHAP         0x2    /* Negotiate CHAP */
#define NOUPAP          0x4    /* Dont allow PAP authentication */
#define NOCHAP          0x8    /* Dont allow CHAP authentication */

#define NEGADDR         0x1    /* Negotiate IPCP addr compression */
#define NEGIPCOMP       0x2    /* Negotiate IPCP compression */

#define NEGADDR_OLD     0x1    /* Neg. OLD addr comp. (RFC1172)*/
#define NEGADDR_NEW     0x2    /* Neg. New addr. comp. (RFC1332) */
#define IPCPADDR        NEGADDR_NEW

#define VJMODE_RFC1172_TYPO 0x1    /* Neg. 1172 compression with typo */
#define VJMODE_RFC1172      0x2    /* Neg. RFC 1172 compression */
#define VJMODE_RFC1332      0x3    /* Neg. RFC 1332 comrpession */
#define IPCPVJCOMPMODE      VJMODE_RFC1332

#define PPP_DIRECT      0x1
#define PPP_DIALUP      0x2
#define PPP_DEMANDDIAL  0x3

#define PPPMODE_ACTIVE  1          /* We start the negotiation */
#define PPPMODE_PASSIVE 0

#define PPP_ASYNC       0          /* Compute & check FCS      */
#define PPP_SYNC        1          /* Disregard FCS            */

/*----------------------------------------------------------------------*/
/* Async control structure                                              */
/*----------------------------------------------------------------------*/
#define COMPRESS_AC    0x01
#define COMPRESS_PR    0x02
#define COMPRESS_VJ     0x04
typedef struct  ppp_async_info 
{
	ULONG pai_flags;
	#define PAI_FLAGS_ESCAPED   0x1
	ULONG pai_asyncmap;        /* current outgoing asyncmap */
	USHORT compress_flag;
	UCHAR maxslotindex;
	UCHAR vj_flag;
} PAI;
extern PAI pais;

/*----------------------------------------------------------------------*/
/* Per channel control block                                            */
/*----------------------------------------------------------------------*/
#define PPP_CANON_MODE       0x1 /* Link in canonical mode */
#define PPP_ANNOUNCE_FLAG    0x2 /* Announce the packet up */

typedef struct  ppp_control_info 
{
	ULONG pci_flags;
	#define PCI_FLAGS_VJCOMP    0x1      /* VJ compression */
	#define PCI_FLAGS_CID       0x2      /* Cid compressed */
	#define PCI_FLAGS_COMPPROT  0x4      /* protocol field compression */
	#define PCI_FLAGS_COMPAC    0x8      /* a/c fields compresion */
} PCI;

/*
 * Inline versions of get/put char/short/long.
 * Pointer is advanced; we assume that both arguments
 * are lvalues and will already be in registers.
 * cp MUST be u_char *.
 */
#define GETCHAR(c, cp) { (c) = *(cp)++; }
#define PUTCHAR(c, cp) { *(cp)++ = (UCHAR) (c); }
#define GETSHORT(s, cp) { (s) = *(cp)++ << 8; (s) |= *(cp)++; }
#define PUTSHORT(s, cp) { *(cp)++ = (UCHAR) ((s) >> 8); *(cp)++ = (UCHAR) (s); }
#define GETLONG(l, cp) { \
	(l) = *(cp)++ << 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; (l) <<= 8; \
	(l) |= *(cp)++; }
#define PUTLONG(l, cp) { \
	*(cp)++ = (UCHAR) ((l) >> 24); \
	*(cp)++ = (UCHAR) ((l) >> 16); \
	*(cp)++ = (UCHAR) ((l) >> 8); \
	*(cp)++ = (UCHAR) (l); }

/*
 * MAKEHEADER - Add Header fields to a packet.
 */
#define MAKEHEADER(p, t) { \
	PUTCHAR(PPP_ALLSTATIONS, p); \
	PUTCHAR(PPP_UI, p); \
	PUTSHORT(t, p); }

/*”√”⁄LCP,CHAPµ»*/
extern UCHAR ppp_buf[256];
#define PPP_BUFDATA    (&ppp_buf[8])

#endif /*_PPP_H*/
