#ifndef _PPPDEFS_H
#define _PPPDEFS_H

/*
 * The basic PPP frame.
 */
#define PPP_HDRLEN	4	/* octets for standard ppp header */
#define PPP_FCSLEN	2	/* octets for FCS */
#define PPP_MRU		1500	/* default MRU = max length of info field */

#define PPP_ADDRESS(p)	(((UCHAR *)(p))[0])
#define PPP_CONTROL(p)	(((UCHAR *)(p))[1])
#define PPP_PROTOCOL(p)	((((UCHAR *)(p))[2] << 8) + ((UCHAR *)(p))[3])

/*
 * Significant octet values.
 */
#define	PPP_ALLSTATIONS	0xff	/* All-Stations broadcast address */
#define	PPP_UI		0x03	/* Unnumbered Information */
#define	PPP_FLAG	0x7e	/* Flag Sequence */
#define	PPP_ESCAPE	0x7d	/* Asynchronous Control Escape */
#define	PPP_TRANS	0x20	/* Asynchronous transparency modifier */

/*
 * Protocol field values.
 */
#define PPP_IP		0x21	/* Internet Protocol */
#define	PPP_VJC_COMP	0x2d	/* VJ compressed TCP */
#define	PPP_VJC_UNCOMP	0x2f	/* VJ uncompressed TCP */
#define PPP_COMP	0xfd	/* compressed packet */
#define PPP_IPCP	0x8021	/* IP Control Protocol */
#define PPP_CCP		0x80fd	/* Compression Control Protocol */
#define PPP_LCP		0xc021	/* Link Control Protocol */
#define PPP_PAP		0xc023	/* Password Authentication Protocol */
#define PPP_LQR		0xc025	/* Link Quality Report protocol */
#define PPP_CHAP	0xc223	/* Cryptographic Handshake Auth. Protocol */

/*
 * Values for FCS calculations.
 */
#define PPP_INITFCS	0xffff	/* Initial FCS value */
#define PPP_GOODFCS	0xf0b8	/* Good final FCS value */
#define PPP_FCS(fcs, c)	(((fcs) >> 8) ^ fcstab[((fcs) ^ (c)) & 0xff])

/*
 * What to do with network protocol (NP) packets.
 */
enum NPmode {
	NPMODE_PASS,		/* pass the packet through */
	NPMODE_DROP,		/* silently drop the packet */
	NPMODE_ERROR,		/* return an error */
	NPMODE_QUEUE		/* save it up for later. */
};

/*
 * Standard PPP header.
 */
struct ppp_header {
	UCHAR  ph_address;     /* Address Field */
	UCHAR  ph_control;     /* Control Field */
	USHORT ph_protocol;    /* Protocol Field */
};

/*
 * Statistics.
 */
struct pppstat	{
	ULONG	ppp_ibytes;	/* bytes received */
	ULONG	ppp_ipackets;	/* packets received */
	ULONG	ppp_ierrors;	/* receive errors */
	ULONG	ppp_obytes;	/* bytes sent */
	ULONG	ppp_opackets;	/* packets sent */
	ULONG	ppp_oerrors;	/* transmit errors */
};

struct vjstat {
	ULONG	vjs_packets;	/* outbound packets */
	ULONG	vjs_compressed;	/* outbound compressed packets */
	ULONG	vjs_searches;	/* searches for connection state */
	ULONG	vjs_misses;	/* times couldn't find conn. state */
	ULONG	vjs_uncompressedin; /* inbound uncompressed packets */
	ULONG	vjs_compressedin;   /* inbound compressed packets */
	ULONG	vjs_errorin;	/* inbound unknown type packets */
	ULONG	vjs_tossed;	/* inbound packets tossed because of error */
};

struct ppp_stats {
	struct pppstat	p;	/* basic PPP statistics */
	struct vjstat	vj;	/* VJ header compression statistics */
};

struct compstat {
	ULONG	unc_bytes;	/* total uncompressed bytes */
	ULONG	unc_packets;	/* total uncompressed packets */
	ULONG	comp_bytes;	/* compressed bytes */
	ULONG	comp_packets;	/* compressed packets */
	ULONG	inc_bytes;	/* incompressible bytes */
	ULONG	inc_packets;	/* incompressible packets */
	ULONG	ratio;		/* recent compression ratio << 8 */
};

struct ppp_comp_stats {
	struct compstat	c;	/* packet compression statistics */
	struct compstat	d;	/* packet decompression statistics */
};

#endif /*_PPPDEFS_H*/
