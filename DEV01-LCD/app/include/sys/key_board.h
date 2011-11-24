#ifndef _KEY_BOARD_H
#define _KEY_BOARD_H

//#include <asm-generic/ioctl.h>

#define	EM9X60_DEV_MAJOR			251
#define EM9X60_SYSINFO_MINOR		0					//em9x60_sysinfo
#define EM9X60_ISA_MINOR			1					//em9x60_isa
#define EM9X60_GPIO_MINOR			2					//em9x60_gpio
#define EM9X60_KEYPAD_MINOR			3					//em9x60_keypad
#define EM9X60_IRQ1_MINOR			4					//em9x60_irq1
#define EM9X60_IRQ2_MINOR			5					//em9x60_irq2 (EM9160 only)

#define EM9X60_BOARD_TYPE_EM9160	1
#define EM9X60_BOARD_TYPE_EM9260	2
#define EM9X60_BOARD_TYPE_EM9360	3

struct isa_io
{
	unsigned long   dwOffset;
	unsigned char   ucValue;
};

typedef	unsigned int	KEY_CODE;

/*
* Emlinix FEB-15-2010: ioctl cmd code definitions:
*/
#define EM9X60_MAGIC							EM9X60_DEV_MAJOR

#define EM9X60_SYSINFO_IOCTL_GET_DBGSL			_IOR(EM9X60_MAGIC,  0x00, unsigned int)
#define EM9X60_SYSINFO_IOCTL_GET_BOARDTYPE		_IOR(EM9X60_MAGIC,  0x01, unsigned int)

#define EM9X60_ISA_IOCTL_READ_LCDTYPE			_IOR(EM9X60_MAGIC,  0x40, unsigned int)
#define EM9X60_ISA_IOCTL_WRITE_LCDTYPE			_IOW(EM9X60_MAGIC,  0x41, unsigned int)
#define EM9X60_ISA_IOCTL_READ_LCD				_IOWR(EM9X60_MAGIC, 0x42, struct isa_io)
#define EM9X60_ISA_IOCTL_WRITE_LCD				_IOW(EM9X60_MAGIC,  0x43, struct isa_io)
#define EM9X60_ISA_IOCTL_READ_CS0				_IOWR(EM9X60_MAGIC, 0x44, struct isa_io)
#define EM9X60_ISA_IOCTL_WRITE_CS0				_IOW(EM9X60_MAGIC,  0x45, struct isa_io)
#define EM9X60_ISA_IOCTL_READ_CS1				_IOWR(EM9X60_MAGIC, 0x46, struct isa_io)
#define EM9X60_ISA_IOCTL_WRITE_CS1				_IOW(EM9X60_MAGIC,  0x47, struct isa_io)

#define EM9X60_GPIO_IOCTL_OUT_ENABLE			_IOW(EM9X60_MAGIC,  0x60, unsigned int)
#define EM9X60_GPIO_IOCTL_OUT_DISABLE			_IOW(EM9X60_MAGIC,  0x61, unsigned int)
#define EM9X60_GPIO_IOCTL_OUT_SET				_IOW(EM9X60_MAGIC,  0x62, unsigned int)
#define EM9X60_GPIO_IOCTL_OUT_CLEAR				_IOW(EM9X60_MAGIC,  0x63, unsigned int)
#define EM9X60_GPIO_IOCTL_OPEN_DRAIN			_IOW(EM9X60_MAGIC,  0x64, unsigned int)
#define EM9X60_GPIO_IOCTL_PIN_STATE				_IOR(EM9X60_MAGIC,  0x65, unsigned int)

#define EM9X60_IRQ_IOCTL_GET_COUNT				_IOR(EM9X60_MAGIC,  0x80, unsigned int)

#define	GPIO0		(1 <<  0)
#define	GPIO1		(1 <<  1)
#define	GPIO2		(1 <<  2)
#define	GPIO3		(1 <<  3)
#define	GPIO4		(1 <<  4)
#define	GPIO5		(1 <<  5)
#define	GPIO6		(1 <<  6)
#define	GPIO7		(1 <<  7)
#define	GPIO8		(1 <<  8)
#define	GPIO9		(1 <<  9)
#define	GPIO10		(1 << 10)
#define	GPIO11		(1 << 11)
#define	GPIO12		(1 << 12)
#define	GPIO13		(1 << 13)
#define	GPIO14		(1 << 14)
#define	GPIO15		(1 << 15)
#define	GPIO16		(1 << 16)
#define	GPIO17		(1 << 17)
#define	GPIO18		(1 << 18)
#define	GPIO19		(1 << 19)
#define	GPIO20		(1 << 20)
#define	GPIO21		(1 << 21)
#define	GPIO22		(1 << 22)
#define	GPIO23		(1 << 23)
#define	GPIO24		(1 << 24)
#define	GPIO25		(1 << 25)
#define	GPIO26		(1 << 26)
#define	GPIO27		(1 << 27)
#define	GPIO28		(1 << 28)
#define	GPIO29		(1 << 29)
#define	GPIO30		(1 << 30)
#define	GPIO31		(1 << 31)

#define	WATCHDOG_IOCTL_BASE	'W'

#define	WDIOC_KEEPALIVE		_IOR(WATCHDOG_IOCTL_BASE, 5, int)


#endif	//__ASM_ARCH_EM9X60_DRIVERS_H











