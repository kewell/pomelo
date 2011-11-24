#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/sysmacros.h>

#if __GLIBC__ >= 2
#include <asm/types.h>		/* glibc 2 conflicts with linux/types.h */
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#else
#include <linux/types.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/route.h>
#include <linux/if_ether.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>

#include "lcp.h"
#include "ppp_sys.h"
#include "ppp_debug.h"

#define _PPP_DEBUG_

static int ppp_fd = -1;
static int ppp_dev_fd = -1;
static int ppp_sock = -1;

static char ifname[32];
static int ifunit;

static int sif_isup = 0;

#define ok_error(num)	((num) == EIO)

int establish_ppp(int fd)
{
	int chindex = 0;
	int flags;
	int ppp_disc = N_PPP;
	int ttyfd = fd;
	//int ppp_fd_fd;

	printf("establish_ppp...\n");
	printf("establish_ppp fd = %d\n",fd);
	if(ppp_sock >= 0) close(ppp_sock);
	ppp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(ppp_sock < 0) {
		ppp_debug(PPPLVL_DATA, "Create socket error.\n");
		printf("Create socket error...\n");
		return 1;
	}
	else
	{
		printf("Create socket succ...\n");
	}
	

/*
 * Ensure that the tty device is in exclusive mode.
 */
 	printf("establish_ppp1...\n");
	printf("establish_ppp1...fd = %d\n",fd);
    if (ioctl(fd, TIOCEXCL, 0) < 0) {
		if (!ok_error ( errno ))
			ppp_debug(PPPLVL_DATA, "Couldn't make tty exclusive: %d", errno);
    }
 	printf("establish_ppp2...\n");
	printf("establish_ppp2..fd = %d\n",fd);
	if (ioctl(fd, TIOCSETD, &ppp_disc) < 0) {
		if (!ok_error (errno) ) {
			ppp_debug(PPPLVL_DATA, "Couldn't set tty to PPP discipline: %d", errno);
			return 1;
		}
    }
 	printf("establish_ppp3...\n");
	printf("establish_ppp3...fd = %d\n",fd);
	if (ioctl(fd, PPPIOCGCHAN, &chindex) == -1) {
		ppp_debug(PPPLVL_DATA, "Couldn't get channel number: %d\n", errno);
		return 1;
	}
	ppp_debug(PPPLVL_DATA, "using channed %d\n", chindex);
	 	printf("establish_ppp4...\n");
		printf("establish_ppp4...fd = %d\n",fd);
	fd = open("/dev/ppp", O_RDWR);
	//ppp_fd_fd = open("/dev/ppp", O_RDWR);
	if (fd < 0) {
		ppp_debug(PPPLVL_DATA, "Couldn't open /dev/ppp: %d\n", errno);
		goto err_close;
	}
	 	printf("establish_ppp5...\n");
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	if (ioctl(fd, PPPIOCATTCHAN, &chindex) < 0) {
		ppp_debug(PPPLVL_DATA, "Couldn't attach to channel %d: %d\n", chindex, errno);
		goto err_close;
	}
	 	printf("establish_ppp6...\n");
	flags = fcntl(fd, F_GETFL);
	if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		ppp_debug(PPPLVL_DATA, "Couldn't set /dev/ppp (channel) to nonblock\n");
	ppp_fd = fd;
	//ppp_fd = ppp_fd_fd;
	printf("establish_ppp7...ppp_fd = %d\n",ppp_fd);
	 	printf("establish_ppp7...\n");
	ppp_dev_fd = open("/dev/ppp", O_RDWR);
	if (ppp_dev_fd < 0) {
		ppp_debug(PPPLVL_DATA, "Couldn't open /dev/ppp: %d\n", errno);
	}
	flags = fcntl(ppp_dev_fd, F_GETFL);
	if (flags == -1
		|| fcntl(ppp_dev_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		ppp_debug(PPPLVL_DATA, "Couldn't set /dev/ppp to nonblock: %d", errno);

	ifunit = -1;
	if(ioctl(ppp_dev_fd, PPPIOCNEWUNIT, &ifunit) < 0) {
		ppp_debug(PPPLVL_DATA, "Couldn't create new ppp unit: %d\n", errno);
		goto err_close;
	}
	ppp_debug(PPPLVL_DATA, "allocat PPP unit %d\n", ifunit);

	sprintf(ifname, "ppp%d", ifunit);

	if (ioctl(fd, PPPIOCCONNECT, &ifunit) < 0) {
		ppp_debug(PPPLVL_DATA, "Couldn't attach to PPP unit %d: %d\n", ifunit, errno);
		goto err_close;
	}

	ppp_debug(PPPLVL_DATA, "ppp established OK.\n");
	return 0;

err_close:
	if(ppp_fd >= 0) close(ppp_fd);
	if(ppp_dev_fd >= 0) close(ppp_dev_fd);
	if(ppp_sock >= 0) close(ppp_sock);
	ppp_fd = ppp_dev_fd = -1;
	ppp_sock = -1;
	disestablish_ppp(ttyfd);
	return 1;
}

int disestablish_ppp(int fd)
{
	int tty_disc = N_TTY;

	/*
	* Flush the tty output buffer so that the TIOCSETD doesn't hang.
	*/
	if(tcflush(fd, TCIOFLUSH) < 0) {
		ppp_debug(PPPLVL_DATA, "tcflush failed: %d\n", errno);
		return 1;
	}
	/*
	* Restore the previous line discipline
	*/
	if (ioctl(fd, TIOCSETD, &tty_disc) < 0) {
		if (!ok_error (errno))
			ppp_debug(PPPLVL_DATA, "ioctl(TIOCSETD, N_TTY): %d (line %d)", errno, __LINE__);
	}

	if (ioctl(fd, TIOCNXCL, 0) < 0) {
		if (!ok_error (errno))
			ppp_debug(PPPLVL_DATA, "ioctl(TIOCNXCL): %d (line %d)", errno, __LINE__);
	}

	return 0;
}

static void dump_packet(char *prompt, unsigned char *p, int len)
{
//#ifndef _PPP_DEBUG_
//	return;
//#else
	int i;
	char str[64];
	char *pstr = str;

	str[0] = 0;
	ppp_debug(PPPLVL_DATA, "%s:\n", prompt);
	for(i=1; i<=len; i++) {
		sprintf(pstr, "%02X ", *p++);
		pstr += 3;
		if(0 == (i&0x0f)) {
			*pstr++ = '\n';
			*pstr = 0;
			pstr = str;
			ppp_debug(PPPLVL_DATA, "%s", str);
			*pstr = 0;
		}
		else if(0 == (i&0x07)) {
			*pstr++ = ':';
			*pstr++ = ' ';
			*pstr = 0;
		}
	}

	if(0 != str[0]) ppp_debug(PPPLVL_DATA, "%s\n", str);
//#endif
}

void ppp_output (unsigned char *p, int len)
{
	int fd = ppp_fd;
	int proto;

	dump_packet("sent", p, len);

	if (len < PPP_HDRLEN) return;

	p += 2;
	len -= 2;
	proto = (p[0] << 8) + p[1];
	if (ppp_dev_fd >= 0 && !(proto >= 0xc000 || proto == PPP_CCPFRAG)) 
		fd = ppp_dev_fd;

	if (write(fd, p, len) < 0) {
		if (errno == EWOULDBLOCK || errno == EAGAIN || errno == ENOBUFS
			|| errno == ENXIO || errno == EIO || errno == EINTR)
			ppp_debug(PPPLVL_DATA, "write: warning: (%d)\n", errno);
		else
			ppp_debug(PPPLVL_DATA, "write: (%d)\n", errno);
	}
}

int ppp_input(unsigned char *buf, int maxlen)
{
	int len, nr;

	if(0 == maxlen) len = PPP_MRU + PPP_HDRLEN;
	else len = maxlen;

	*buf++ = PPP_ALLSTATIONS;
	*buf++ = PPP_UI;
	len -= 2;

	nr = -1;
	//printf("ppp_fd = %d\n",ppp_fd);
	if (ppp_fd >= 0) {
		nr = read(ppp_fd, buf, len);
		//printf("ppp_input1  nr = %d\n",nr);
		if (nr < 0 && errno != EWOULDBLOCK && errno != EAGAIN
			&& errno != EIO && errno != EINTR)
			ppp_debug(PPPLVL_DATA, "read: %d\n", errno);
		if (nr < 0 && errno == ENXIO)
			return 0;
	}

	if (nr < 0 && ppp_dev_fd >= 0) {
		/* N.B. we read ppp_fd first since LCP packets come in there. */
		nr = read(ppp_dev_fd, buf, len);
		//printf("ppp_input2  nr = %d\n",nr);
		if (nr < 0 && errno != EWOULDBLOCK && errno != EAGAIN
			&& errno != EIO && errno != EINTR)
			ppp_debug(PPPLVL_DATA, "read /dev/ppp: %d", errno);
		if (nr < 0 && errno == ENXIO)
			nr = 0;
	}

	if(nr > 0) {
		//printf("ppp_input3  nr = %d\n",nr);
		nr += 2;
		dump_packet("recv", buf, nr);
	}
	//printf("ppp_input4  nr = %d\n",nr);
	return nr;
}

int ppp_peek(unsigned short protocol, unsigned char *buf, int maxlen)
{
	int rl;
	unsigned short proto;

	rl = ppp_input(buf, maxlen);
	//printf("ppp_peek1\n");
	if(rl < PPP_HDRLEN) return 0;
	//printf("ppp_peek2\n");
	proto = ((unsigned short)buf[2]<<8) + buf[3];
	if(proto != protocol)
	{
		//printf("ppp_peek3\n");
		if(PPP_LCP == proto)
		{
			//printf("ppp_peek4\n");
			if(lcp_input(buf, rl)) return -1;
		}
		//printf("ppp_peek5\n");
		return 0;
	}
	//printf("ppp_peek6\n");
	return rl;
}

#define SET_SA_FAMILY(addr, family)			\
	memset ((char *) &(addr), '\0', sizeof(addr));	\
	addr.sa_family = (family);

#define SIN_ADDR(x)	(((struct sockaddr_in *) (&(x)))->sin_addr.s_addr)

static int sifaddr (unsigned long our_adr, unsigned long his_adr)
{
	struct ifreq   ifr;
	unsigned long net_mask;

	memset (&ifr, '\0', sizeof (ifr));

	SET_SA_FAMILY (ifr.ifr_addr,    AF_INET);
	SET_SA_FAMILY (ifr.ifr_dstaddr, AF_INET);
	SET_SA_FAMILY (ifr.ifr_netmask, AF_INET);

	strcpy (ifr.ifr_name, ifname);
	/*
	*  Set our IP address
	*/
	SIN_ADDR(ifr.ifr_addr) = our_adr;
	if (ioctl(ppp_sock, SIOCSIFADDR, (caddr_t) &ifr) < 0) {
		if (errno != EEXIST) {
			if (! ok_error (errno))
			ppp_debug(PPPLVL_DATA, "ioctl(SIOCSIFADDR): %d (line %d)\n", errno, __LINE__);
		}
		else {
			ppp_debug(PPPLVL_DATA, "ioctl(SIOCSIFADDR): Address already exists\n");
		}
		return 1;
	}
	/*
	*  Set the gateway address
	*/
	SIN_ADDR(ifr.ifr_dstaddr) = his_adr;
	if (ioctl(ppp_sock, SIOCSIFDSTADDR, (caddr_t) &ifr) < 0) {
		if (! ok_error (errno))
			ppp_debug(PPPLVL_DATA, "ioctl(SIOCSIFDSTADDR): %d (line %d)\n", errno, __LINE__);
		return 1;
	}
	/*
	*  Set the netmask.
	*  For recent kernels, force the netmask to 255.255.255.255.
	*/
	net_mask = ~0L;

	if (net_mask != 0) {
		SIN_ADDR(ifr.ifr_netmask) = net_mask;
		if (ioctl(ppp_sock, SIOCSIFNETMASK, (caddr_t) &ifr) < 0) {
			if (! ok_error (errno))
				ppp_debug(PPPLVL_DATA, "ioctl(SIOCSIFNETMASK): %d (line %d)", errno, __LINE__);
			return 1;
		}
	}

	return 0;
}

static int sifdefaultroute (unsigned long ouraddr, unsigned long gateway)
{
	struct rtentry rt;

	/*if (defaultroute_exists(&rt) && strcmp(rt.rt_dev, ifname) != 0) {
		if (rt.rt_flags & RTF_GATEWAY)
			error("not replacing existing default route via %I",
			  SIN_ADDR(rt.rt_gateway));
		else
			error("not replacing existing default route through %s",
			  rt.rt_dev);
		return 1;
	}*/

	memset (&rt, 0, sizeof (rt));
	SET_SA_FAMILY (rt.rt_dst, AF_INET);

	rt.rt_dev = ifname;

	SET_SA_FAMILY (rt.rt_genmask, AF_INET);
	SIN_ADDR(rt.rt_genmask) = 0L;

	rt.rt_flags = RTF_UP;
	if (ioctl(ppp_sock, SIOCADDRT, &rt) < 0) {
		if ( ! ok_error ( errno ))
			ppp_debug(PPPLVL_DATA, "default route ioctl(SIOCADDRT): %d", errno);
		return 1;
	}

	return 0;
}

static int sifdown (void)
{
	struct ifreq ifr;

	memset (&ifr, '\0', sizeof (ifr));
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(ppp_sock, SIOCGIFFLAGS, (caddr_t) &ifr) < 0) {
		if (! ok_error (errno))
			ppp_debug(PPPLVL_DATA, "ioctl (SIOCGIFFLAGS): %d (line %d)", errno, __LINE__);
		return 1;
	}

	ifr.ifr_flags &= ~IFF_UP;
	ifr.ifr_flags |= IFF_POINTOPOINT;
	if (ioctl(ppp_sock, SIOCSIFFLAGS, (caddr_t) &ifr) < 0) {
		if (! ok_error (errno))
			ppp_debug(PPPLVL_DATA, "ioctl(SIOCSIFFLAGS): %d (line %d)", errno, __LINE__);
		return 1;
	}

	return 0;
}

extern unsigned long ipcp_get_addr(void);
int ppp_netup(void)
{
	struct ifreq ifr;
	unsigned long ipaddr;

	memset (&ifr, '\0', sizeof (ifr));
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(ppp_sock, SIOCGIFFLAGS, (caddr_t) &ifr) < 0) {
		if (! ok_error (errno))
			ppp_debug(PPPLVL_DATA, "ioctl (SIOCGIFFLAGS): %d (line %d)", errno,  __LINE__);
		return 1;
	}

	sif_isup = 1;

	ifr.ifr_flags |= (IFF_UP | IFF_POINTOPOINT);
	if (ioctl(ppp_sock, SIOCSIFFLAGS, (caddr_t) &ifr) < 0) {
		if (! ok_error (errno))
			ppp_debug(PPPLVL_DATA, "ioctl(SIOCSIFFLAGS): %d (line %d)", errno, __LINE__);
		return 1;
	}

	ipaddr = htonl(ipcp_get_addr());
	if(sifaddr(ipaddr, ipaddr)) return 1;

	sifdefaultroute(ipaddr, ipaddr);

	return 0;
}

int ppp_netdown(void)
{
	//lcp_close(0);

	if(sif_isup) {
		sifdown();
		sif_isup = 0;
	}

	if(ppp_dev_fd >= 0) close(ppp_dev_fd);
	if(ppp_fd >= 0) close(ppp_fd);
	if(ppp_sock >= 0) close(ppp_sock);

	ppp_dev_fd = -1;
	ppp_fd = -1;
	ppp_sock = -1;

	return 0;
}

/*void ppp_sys_test(void)
{
	int i, fd;

	UartOpen(1);
	UartSetBaudrate(1, 57600);
	fd = UartGetFid(1);

	for(i=0; i<10000; i++) {
		if(establish_ppp(fd)) PrintLog(0, "establish_ppp fail\n");
		ppp_netdown();
		disestablish_ppp(fd);
	}

	PrintLog(0, "ppp sys test end\n");
}*/
