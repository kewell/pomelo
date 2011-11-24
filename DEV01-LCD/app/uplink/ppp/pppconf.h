#ifndef __PPPCONF_H
#define __PPPCONF_H

#define MAXPPPUNITS    4          /* Max. # of NI interfaces */
#define NUM_PPP        4          /* # of PPP channels */

#define NPPP           NUM_PPP
#define NPPPHEADERS    30         /* # of PPP headers */
#define NCALLOUTS      5          /* Number of timers */
#define DEBUG          YES        /* turn on debug */
//#define DEBUG          NO

#define OPENTCPIP      NO          /* NO for pNA+, YES for OpEN TCP/IP */
#define CHAPNEEDED     YES         /* YES - need CHAP */
#define UPAPNEEDED     NO         /* YES - need PAP */
//#define UPAPNEEDED     YES         /* YES - need PAP */


/*
 * Timeouts.
 */
//#define DEFTIMEOUT      3   /* Timeout time in seconds */
//#define DEFTIMEOUT      6   /* Timeout time in seconds */
#define DEFTIMEOUT      30   /* Timeout time in seconds */
#define DEFMAXTERMREQS  2   /* Maximum Terminate-Request transmissions */
#define DEFMAXCONFREQS  10  /* Maximum Configure-Request transmissions */
#define DEFMAXNAKLOOPS  5   /* Maximum number of nak loops */

#define DEFMRU  1500        /* Try for this */
#define MINMRU  128         /* No MRUs below this */
#define MAXMRU  1500        /* Normally limit MRU to this */
#define MYMRU   1500        /* our MRU */
#define MINMRU  128         /* No MRUs below this is allowed */

#if (DEFMRU > MYMRU)
#define BUFSIZE DEFMRU      /* use default */
#else
#define BUFSIZE MYMRU       /* whatever user configures */
#endif

/*----------------------------------------------------------------------*/
/*    ERROR NUMBERS FOR PPP                                             */
/*----------------------------------------------------------------------*/
#define EOK      0x10070000 /* no problem */
#define EMIB     0x10070001 /* try to access mib when iface's down */
#define ETIMEOUT 0x10070002 /* exhaust retries */
#define EAUTHFAIL 0x10070003 /* Authentication failed with server */


/*----------------------------------------------------------------------*/
/* OPTION MRU                                                           */
/*----------------------------------------------------------------------*/

#define DIALTIMEOUT     30               /* Timeout seconds for demanddial */
#define MIN2DIALTIMEOUT (60/DIALTIMEOUT) /* Minute multiplier */

/*---------------------------------------------------------------------*/
/* THE 'PPPD' TASK PARAMETERS                                          */
/*---------------------------------------------------------------------*/
#define PPPD_NAME       "PPPD"    /* Name of the task */
#define PPPD_PRIO       254       /* Priority of the client task */
#define PPPD_USRSTACK   0         /* User stack size of pppd task */
#define PPPD_SYSSTACK   2048      /* Supervisor stack size */
#define PPPD_FLAGS      0x00      /* Flags for pppd task */
#define PPPD_INITMODE   T_SUPV    /* Run in superviser mode - int 0 */

#endif /* __PPPCONF_H */

