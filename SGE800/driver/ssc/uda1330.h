/******************************************************************************
 * 许继电气股份有限公司                                    版权：2008-2015    *
 ******************************************************************************
 * 本源代码及其相关文档为河南许昌许继电气股份有限公司独家所有，未经明文许     *
 * 可不得擅自修改或发布，否则将追究相关的法律责任。                           *
 *                                                                            *
 *                       河南许昌许继股份有限公司                             *
 *                       www.xjgc.com                                         *
 *                       (0374) 321 2924                                      *
 *                                                                            *
 ******************************************************************************
 * 
 * 项目名称		:	at91sam9260音频驱动
 * 文件名		:	uda1330.c
 * 描述			:	uda1330的mixer驱动
 * 版本			:	1.0.1
 * 作者			:	路冉冉
 *
 * 修改历史记录	:
 * --------------------
 * 01a, 20aug2009, Roy modified
 * --------------------
 *
 ******************************************************************************/

#include <linux/module.h> 
#include <linux/device.h> 
#include <linux/init.h> 
#include <linux/types.h> 
#include <linux/fs.h> 
#include <linux/mm.h> 
#include <linux/slab.h> 
#include <linux/delay.h> 
#include <linux/sched.h> 
//#include <linux/poll.h> 
#include <linux/interrupt.h> 
#include <linux/errno.h> 
#include <linux/sound.h> 
#include <linux/soundcard.h> 

//#include <linux/pm.h> 

#include <asm/uaccess.h> 
#include <asm/io.h> 

#include <linux/semaphore.h>
#include <asm/dma.h> 
//#include <asm/arch/cpu_s3c2410.h> 
//#include <asm/dma.h>
//#include <asm/arch/regs-gpio.h> 
//#include <asm/arch/regs-iis.h> 
//#include <asm/hardware/clock.h> 
//#include <asm/arch/regs-clock.h> 
#include <linux/clk.h>		//clock func
#include <linux/dma-mapping.h> 
#include <linux/workqueue.h> 

#include <asm/dma-mapping.h> 
#include <mach/hardware.h>
//#include <asm/arch/map.h> 
#include <mach/gpio.h>
#include <mach/at91_ssc.h>
#include <linux/atmel_pdc.h>
#include <mach/hardware.h>
//#include <asm/arch/S3C2410.h> 
#define PFX "at91sam9260-uda1330-superlp: " 

//#define MAX_DMA_CHANNELS 0 

/* The S3C2410 has four internal DMA channels. */ 

//#define MAX_S3C2410_DMA_CHANNELS S3C2410_DMA_CHANNELS 



#define DMA_BUF_WR 1 
#define DMA_BUF_RD 0 

#define dma_wrreg(chan, reg, val) writel((val), (chan)->regs + (reg)) 

#define DEF_VOLUME 65 

/* UDA1330 Register bits */ 
#define UDA1330_ADDR (0x5 << 2)
#define UDA1330_REG_DATA (UDA1330_ADDR + 0) 
#define UDA1330_REG_STATUS (UDA1330_ADDR + 2 ) 

/* status control */ 
#define STATUS (0x00)

#define STATUS_SC_512FS (0 << 4) 
#define STATUS_SC_384FS (1 << 4) 
#define STATUS_SC_256FS (2 << 4) 
#define STATUS_IF_MASK (7 << 1) 

#define STATUS_IF_I2S (0 << 1) 
#define STATUS_IF_LSB16 (1 << 1) 
#define STATUS_IF_LSB18 (2 << 1) 
#define STATUS_IF_LSB20 (3 << 1) 
#define STATUS_IF_MSB (4 << 1) 

/* data0 direct control */ 
#define DATA0 (0x00) 
#define DATA0_VOLUME_MASK (0x3f) 
#define DATA0_VOLUME(x) (x) 

#define DATA1 (0x80) 
#define DATA_DEEMP_NONE (0x0 << 3) 
#define DATA_DEEMP_32KHz (0x1 << 3) 
#define DATA_DEEMP_44KHz (0x2 << 3) 
#define DATA_DEEMP_48KHz (0x3 << 3) 
#define DATA_MUTE (0x1 << 2) 


#define AUDIO_NAME "UDA1330" 
#define AUDIO_TXEMT_IRQ "UDA1330"

#define AUDIO_NAME_VERBOSE "UDA1330 audio driver" 

#define AUDIO_FMT_MASK (AFMT_S16_LE) 
#define AUDIO_FMT_DEFAULT (AFMT_S16_LE) 

#define AUDIO_CHANNELS_DEFAULT 1
#define AUDIO_RATE_DEFAULT 8000 

#define AUDIO_NBFRAGS_DEFAULT 8
#define AUDIO_FRAGSIZE_DEFAULT 8192

#define S_CLOCK_FREQ 384 
#define PCM_ABS(a) (a < 0 ? -a : a) 
//用audio_buf_t来管理一段内存，在用audio_stream_t来管理N个audio_buf_t
//这样的好处是提高了音频给CPU的负担
typedef struct { 
	int size; /* buffer size */ 
	char *start; /* point to actual buffer */ 
	dma_addr_t dma_addr; /* physical buffer address */ 
	struct semaphore sem; /* down before touching the buffer */ 
	int master; /* owner for buffer allocation, contain size when true */ 
} audio_buf_t; 

typedef struct { 
	audio_buf_t *buffers; /* pointer to audio buffer structures */ 
	audio_buf_t *buf; /* current buffer used by read/write */ 
	audio_buf_t *dma;/* current buffer used by dma process */ 
	u_int dmaing;/*DMA运行状态*/
	u_int dma_ok;
	u_int buf_len; /*剩余buf大小*/
	u_int buf_idx; /* index for the pointer above */ 
	u_int dma_idx; /* index for the pointer dma */ 
	u_int fragsize; /* fragment i.e. buffer size */ 
	u_int nbfrags; /* nbr of fragments */ 
	//dmach_t dma_ch; /* DMA channel (channel2 for audio) */ 
	//u_int dma_ok; 
} audio_stream_t; 

#define NEXT_BUF(_s_,_b_) {	\
(_s_)->_b_##_idx++;	\
(_s_)->_b_##_idx %= (_s_)->nbfrags;	\
(_s_)->_b_ = (_s_)->buffers + (_s_)->_b_##_idx; } 


#define at91_ssc_read(a)	((unsigned long) __raw_readl(a))
#define at91_ssc_write(a,v)	__raw_writel((v),(a))

#define FILE_SAMPLING_FREQ	8000 // in Hz
// Constant declarations used by I2S mode
#define SLOT_BY_FRAME	2
#define BITS_BY_SLOT	16

#define MCK 99000000
//l3接口定义
#define L3DATA AT91_PIN_PC13
#define L3MOD AT91_PIN_PC3
#define L3CLK AT91_PIN_PC2

#define I2S_ASY_MASTER_TX_SETTING(nb_bit_by_slot, nb_slot_by_frame)( +\
AT91_SSC_CKS_DIV   +\
AT91_SSC_CKO_CONTINUOUS      +\
AT91_SSC_START_FALLING_RF+\
((1<<16) & AT91_SSC_STTDLY) +\
((((nb_bit_by_slot*nb_slot_by_frame)/2)-1) <<24))

#define I2S_ASY_TX_FRAME_SETTING(nb_bit_by_slot, nb_slot_by_frame)( +\
(nb_bit_by_slot-1)  +\
AT91_SSC_MSBF   +\
(((nb_slot_by_frame-1)<<8) & AT91_SSC_DATNB)  +\
(((nb_bit_by_slot-1)<<16) & AT91_SSC_FSLEN) +\
AT91_SSC_FSOS_NEGATIVE)

