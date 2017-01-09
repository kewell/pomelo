
#ifndef __HI_GPIO_H__ 
#define __HI_GPIO_H__

#define  GPIO_GROUP_MAX       0x13 /* From GPIO 0 to 18*/
#define  DDRC_GROUP_MAX       0x02

#define WRITE_DDRC_REG(group, offset, v32)		\
    (iowrite32(v32, (void*)(g_stGpioInfo.ddrc_ker_vir_addr[group] + offset)))

#define READ_DDRC_REG(group, offset)		\
    (ioread32((void*)(g_stGpioInfo.ddrc_ker_vir_addr[group] + offset)))

#define GET_GPIO_DIRECT(group, bit)   \
    (ioread8((void *)(g_stGpioInfo.gpio_ker_vir_addr[group] + 0x400)) >> (bit))&0x01

#define SET_GPIO_DIRECT(group, bit, dir) \
{ \
    do{ \
        iowrite8(((ioread8((void *)(g_stGpioInfo.gpio_ker_vir_addr[group] + 0x400))&(~(1<<(bit))))|((dir)<<(bit))), \
                (void*)(g_stGpioInfo.gpio_ker_vir_addr[group] + 0x400));  \
    }while(0); \
}

#define READ_GPIO_DATA(group, bit) \
    ioread8((void*)(g_stGpioInfo.gpio_ker_vir_addr[group] + ((1<<(bit))<<2)))

#define WRITE_GPIO_DATA(group, bit, data) \
{ \
    do \
    { \
        iowrite8(((data)<<(bit)), \
                (void*)(g_stGpioInfo.gpio_ker_vir_addr[group] + ((1<<(bit))<<2)));  \
    }while(0); \
}

#define DOG_RESET_TH		(0x6400000 * 20)	//0x3EF1480:in a 144 MHz system: Watch Dog Timer is 1 second.

#define INT_TIMER  	(HZ * 10)

#define WTD_FD      	7

#define HIWDT_LOAD      0x000
#define HIWDT_VALUE     0x004
#define HIWDT_CTRL      0x008
#define HIWDT_INTCLR    0x00C
#define HIWDT_RIS       0x010
#define HIWDT_MIS       0x014
#define HIWDT_LOCK      0xC00
#define HIWDT_UNLOCK_VAL        0x1ACCE551

#define REG_BASE_WATCHDOG	 0x20040000

#define HIWDT_BASE      (REG_BASE_WATCHDOG)

#define HIWDT_REG(x)	(HIWDT_BASE + (x))

#define hiwdt_readl(x)          readl(IO_ADDRESS(HIWDT_REG(x)))
#define hiwdt_writel(v,x)       writel(v, IO_ADDRESS(HIWDT_REG(x)))

typedef struct GPIO_GROUP_BIT_INFO_T
{
    unsigned int  groupnumber;
    unsigned int  bitnumber;
    unsigned int  value;
}GPIO_GROUP_BIT_INFO;

typedef struct GPIO_INFO_T
{
    unsigned long gpio_ker_vir_addr[GPIO_GROUP_MAX];
    int gpio_status;
    struct semaphore   gpio_sem;
    struct semaphore   alarmin_sem;
    unsigned long ddrc_ker_vir_addr[DDRC_GROUP_MAX];
}GPIO_INFO;

#define GPIO_MAGIC   'G'

#define GPIOGET_SERIAL422       0x102
#define GPIOGET_ALARM_OUT       0x103 //259		//Set Alarm Out
#define GPIOGET_ALARM_IN        0x104 //260		//Set Alarm In
#define GPIOGET_BUZZER          0x107 //263		
#define GPIOGET_WATCHDOG        0x109
#define GPIOGET_SERIAL485       0x10A	
#define GPIOGET_WATCHDOGEX      0x10B
#define GPIOGET_IPSTATUS        0x10D
#define GPIOGET_LEDSTATUS	    0x10E //D270
#define GPIOSET_RESETHDMI       0x10F

#define GPIO_SET_DIR            0x110
#define GPIO_GET_DIR            0x111
#define GPIO_READ_BIT  0x112     //_IOWR(IOC_TYPE_GPIO, 0x03, GPIO_GROUP_BIT_INFO)
#define GPIO_WRITE_BIT    0x113 //_IOW(IOC_TYPE_GPIO, 0x04, GPIO_GROUP_BIT_INFO)
#define GPIO_I2C_READ     0x115  //_IO(IOC_TYPE_GPIO, 0x05)
#define GPIO_I2C_WRITE   0x116   //_IO(IOC_TYPE_GPIO, 0x06)
#define GPIOGET_DM_MODE 0x117
#define DDRC_SET_MODE 0x118
#define GPIO_I2C_WRITE_I2C_2 0x119
#define GPIO_I2C_READ_I2C_2   0x120
#define GPIO_I2C_READ16     0x121  //_IO(IOC_TYPE_GPIO, 0x05)
#define GPIO_I2C_WRITE16   0x122   //_IO(IOC_TYPE_GPIO, 0x06)

#define GPIOSET_DETECTVGA       0x210
#define GPIO_READ_REG           0x211
#define GPIO_WRITE_REG          0x212
unsigned char gpio_i2c_read(unsigned char devaddress, unsigned char address);
void gpio_i2c_write(unsigned char devaddress, unsigned char address, unsigned char data);
unsigned long gpio_i2c_read16(unsigned char devaddress, int address);
void gpio_i2c_write16(unsigned char devaddress, int address, int data);
extern unsigned char gpio_i2c_read_i2c_2(unsigned char devaddress, unsigned char address);
extern void gpio_i2c_write_i2c_2(unsigned char devaddress, unsigned char address, unsigned char data);
#endif

#define	MAX_ALARM_OUT_PIN	    0x08

#define GPIO_INDEX_ALARM_IN1	0x01
#define GPIO_INDEX_ALARM_OUT    0x06
#define GPIO_INDEX_ALARM_IN0	0x08
#define GPIO_INDEX_PANEL        0x09
#define	GPIO_INDEX_LED	        0x0b
#define GPIO_INDEX_ENCRYT	    0x0c
#define	GPIO_INDEX_RS485        0x0c
#define GPIO_INDEX_RS422        0x10
#define GPIO_INDEX_BEEP         0x11
#define GPIO_INDEX_PWD          0x11

#define SYS_LED_0               0x00
#define SYS_LED_1               0x01

#define PANEL_LED_NET           0x00
#define PANEL_LED_NET_CASE      0x00 | 0x0C
#define PANEL_LED_STATUS        0x01
#define PANEL_LED_STATUS_CASE   0x01 | 0x0C
#define PANEL_LED_ALARM         0x02
#define PANEL_LED_ALARM_CASE    0x02 | 0x0C

#define GPIO_DATA               0x3FC
#define GPIO_DIR                0x400
/* ¸´ÓÃ¼Ä´æÆ÷¸ÅÀÀ(»ùµØÖ·ÊÇ0x200F_0000) */
#define	VIU0_CLK			0x000
#define	VIU0_VS			0x004
#define	VIU0_HS			0x008
#define	VIU0_DAT15			0x00C
#define	VIU0_DAT14			0x010
#define	VIU0_DAT13			0x014
#define	VIU0_DAT12			0x018
#define	VIU0_DAT11			0x01C
#define	VIU0_DAT10			0x020
#define	VIU0_DAT9			0x024
#define	VIU0_DAT8			0x028
#define	VIU0_DAT7			0x02C
#define	VIU0_DAT6			0x030
#define	VIU0_DAT5			0x034
#define	VIU0_DAT4			0x038
#define	VIU0_DAT3			0x03C
#define	VIU0_DAT2			0x040
#define	VIU0_DAT1			0x044
#define	VIU0_DAT0			0x048
#define	VIU1_CLK			0x04C
#define	VIU1_VS			0x050
#define	VIU1_HS			0x054
#define	VIU1_DAT15			0x058
#define	VIU1_DAT14			0x05C
#define	VIU1_DAT13			0x060
#define	VIU1_DAT12			0x064
#define	VIU1_DAT11			0x068
#define	VIU1_DAT10			0x06C
#define	VIU1_DAT9			0x070
#define	VIU1_DAT8			0x074
#define	VIU1_DAT7			0x078
#define	VIU1_DAT6			0x07C
#define	VIU1_DAT5			0x080
#define	VIU1_DAT4			0x084
#define	VIU1_DAT3			0x088
#define	VIU1_DAT2			0x08C
#define	VIU1_DAT1			0x090
#define	VIU1_DAT0			0x094
#define	VIU2_CLK			0x098
#define	VIU2_VS			0x09C
#define	VIU2_HS			0x0A0
#define	VIU2_DAT15			0x0A4
#define	VIU2_DAT14			0x0A8
#define	VIU2_DAT13			0x0AC
#define	VIU2_DAT12			0x0B0
#define	VIU2_DAT11			0x0B4
#define	VIU2_DAT10			0x0B8
#define	VIU2_DAT9			0x0BC
#define	VIU2_DAT8			0x0C0
#define	VIU2_DAT7			0x0C4
#define	VIU2_DAT6			0x0C8
#define	VIU2_DAT5			0x0CC
#define	VIU2_DAT4			0x0D0
#define	VIU2_DAT3			0x0D4
#define	VIU2_DAT2			0x0D8
#define	VIU2_DAT1			0x0DC
#define	VIU2_DAT0			0x0E0
#define	VGA_HS			0x0E4
#define	VGA_VS			0x0E8
#define	VOU1120_CLK			0x0EC
#define	VOU1120_VS			0x0F0
#define	VOU1120_HS			0x0F4
#define	VOU1120_DATA15			0x0F8
#define	VOU1120_DATA14			0x0FC
#define	VOU1120_DATA13			0x100
#define	VOU1120_DATA12			0x104
#define	VOU1120_DATA11			0x108
#define	VOU1120_DATA10			0x10C
#define	VOU1120_DATA9			0x110
#define	VOU1120_DATA8			0x114
#define	VOU1120_DATA7			0x118
#define	VOU1120_DATA6			0x11C
#define	VOU1120_DATA5			0x120
#define	VOU1120_DATA4			0x124
#define	VOU1120_DATA3			0x128
#define	VOU1120_DATA2			0x12C
#define	VOU1120_DATA1			0x130
#define	VOU1120_DATA0			0x134
#define	SIO0_RCLK			0x138
#define	SIO0_RFS			0x13C
#define	SIO0_DIN			0x140
#define	SIO1_RCLK			0x144
#define	SIO1_RFS			0x148
#define	SIO1_DIN			0x14C
#define	SIO2_RCLK			0x150
#define	SIO2_RFS			0x154
#define	SIO2_DIN			0x158
#define	SIO3_RCLK			0x15C
#define	SIO3_RFS			0x160
#define	SIO3_DIN			0x164
#define	SIO4_XCLK			0x168
#define	SIO4_XFS			0x16C
#define	SIO4_RCLK			0x170
#define	SIO4_RFS			0x174
#define	SIO4_DOUT			0x178
#define	SIO4_DIN			0x17C
#define	SPI_SCLK			0x180
#define	SPI_SDO			0x184
#define	SPI_SDI			0x188
#define	SPI_CSN0			0x18C
#define	SPI_CSN6			0x190
#define	SPI_CSN7			0x194
#define	I2C_SDA			0x198
#define	I2C_SCL			0x19C
#define	UART1_RTSN			0x1A0
#define	UART1_RXD			0x1A4
#define	UART1_TXD			0x1A8
#define	UART1_CTSN			0x1AC
#define	RGMII0_TXCK			0x1B0
#define	RGMII0_CRS			0x1B4
#define	RGMII0_COL			0x1B8
#define	RGMII1_RXDV			0x1BC
#define	RGMII1_RXD3			0x1C0
#define	RGMII1_RXD2			0x1C4
#define	RGMII1_RXD1			0x1C8
#define	RGMII1_RXD0			0x1CC
#define	RGMII1_RXCK			0x1D0
#define	RGMII1_TXEN			0x1D4
#define	RGMII1_TXD3			0x1D8
#define	RGMII1_TXD2			0x1DC
#define	RGMII1_TXD1			0x1E0
#define	RGMII1_TXD0			0x1E4
#define	RGMII1_TXCK			0x1E8
#define	RGMII1_TXCKOUT			0x1EC
#define	RGMII1_CRS			0x1F0
#define	RGMII1_COL			0x1F4
#define	IR_IN			0x1F8
#define	NF_DQ0			0x1FC
#define	NF_DQ1			0x200
#define	NF_DQ2			0x204
#define	NF_DQ3			0x208
#define	NF_DQ4			0x20C
#define	NF_DQ5			0x210
#define	NF_DQ6			0x214
#define	NF_DQ7			0x218
#define	NF_RDY0			0x21C
#define	NF_RDY1			0x220
#define	SFC_DIO			0x224
#define	SFC_WP_IO2			0x228
#define	SFC_DOI			0x22C
#define	SFC_HOLD_IO3			0x230
#define	USB0_OVRCUR			0x234
#define	USB0_PWREN			0x238
#define	USB1_OVRCUR			0x23C
#define	USB1_PWREN			0x240
#define	HDMI_HOTPLUG			0x244
#define	HDMI_CEC			0x248
#define	HDMI_SDA			0x24C
#define	HDMI_SCL			0x250
#define	GPIO18_3			0x254
#define	GPIO18_4			0x258

