#include "uda1330.h" 

//#undef	DEBUG
#define DEBUG
#ifdef DEBUG
#define	DPRINTK( x... )		printk("ssc: " x)
#else
#define DPRINTK( x... )
#endif

static struct clk *iis_clock; 
static void __iomem *iis_base; 
/*

static struct s3c2410_dma_client s3c2410iis_dma_out= { 
	.name = "I2SSDO", 
}; 

static struct s3c2410_dma_client s3c2410iis_dma_in = { 
	.name = "I2SSDI", 
}; 
*/
static audio_stream_t output_stream; 
//static audio_stream_t input_stream; /* input */ 

static u_int audio_rate; 
static int audio_channels; 
static int audio_fmt; 
static u_int audio_fragsize; 
static u_int audio_nbfrags; 

static int audio_rd_refcount; 
static int audio_wr_refcount; 
#define audio_active (audio_rd_refcount | audio_wr_refcount) 

static int audio_dev_dsp; 
static int audio_dev_mixer; 
//static int audio_mix_modcnt; 

static int uda1330_volume; 
//static u8 uda_sampling; 
//static int uda1330_boost; 
//static int mixer_igain=0x4; /* -6db*/ 

struct uda_struct_t {
    char *name;
    struct work_struct uda_work;
};
struct workqueue_struct *uda_wq = NULL; //= create_workqueue("uda_wq");
struct uda_struct_t uda_name;

//struct work_struct uda_wq;
void __iomem *sys_base = (void __iomem *) AT91_VA_BASE_SYS;

static void uda1330_l3_address(u8 data) 
{ 
	int i; 
	unsigned long flags; 
	
	local_irq_save(flags);
	at91_set_gpio_value(L3MOD,1);
	at91_set_gpio_output(L3CLK,1);
	udelay(1);
	//at91_set_gpio_output(L3CLK,1);
	//udelay(1);
	at91_set_gpio_value(L3MOD,0);
	//at91_set_gpio_value(L3CLK,0);	
	udelay(1); 	
	for (i = 0; i < 8; i++) { 
		if (data & 0x1) { 			
			at91_set_gpio_value(L3CLK,0);
			at91_set_gpio_value(L3DATA,1);
			udelay(1); 			
			at91_set_gpio_value(L3CLK,1);
		} else { 			
			at91_set_gpio_value(L3CLK,0);
			at91_set_gpio_value(L3DATA,0);
			udelay(1); 			
			at91_set_gpio_value(L3CLK,1);
		} 
		data >>= 1; 
	} 
	udelay(1); 	
	at91_set_gpio_value(L3MOD,1);
	//at91_set_gpio_value(L3CLK,1);
	local_irq_restore(flags); 
} 

static void uda1330_l3_data(u8 data) 
{ 
	int i; 
	unsigned long flags; 
	DPRINTK("%s,data write to L3 = %x\n",__FUNCTION__, data); 
	local_irq_save(flags); 
	udelay(1);	
	for (i = 0; i < 8; i++) { 
		if (data & 0x1) { 
			at91_set_gpio_value(L3CLK,0);
			at91_set_gpio_value(L3DATA,1);
			udelay(1); 			
			at91_set_gpio_value(L3CLK,1);
		} else { 
			at91_set_gpio_value(L3CLK,0);
			at91_set_gpio_value(L3DATA,0);
			udelay(1); 
			at91_set_gpio_value(L3CLK,1);
		} 		
		data >>= 1; 
	} 
	udelay(1);	
	local_irq_restore(flags); 
} 

static void audio_clear_buf(audio_stream_t * s) 
{ 
	DPRINTK("audio_clear_buf\n"); 
	
	//if(s->dma_ok) s3c2410_dma_ctrl(s->dma_ch, S3C2410_DMAOP_FLUSH); 
	
	if (s->buffers) { 
		int frag; 
		
		for (frag = 0; frag < s->nbfrags; frag++) { 
			if (!s->buffers[frag].master) 
				continue; 
			dma_free_coherent(NULL, 
				s->buffers[frag].master, 
				s->buffers[frag].start, 
				s->buffers[frag].dma_addr); 
		} 
		kfree(s->buffers); 
		s->buffers = NULL; 
	} 
	
	s->buf_idx = 0; 
	s->dma_idx = 0; 
	s->buf = NULL; 
	s->dma = NULL; 
} 
//申请dma缓存
static int audio_setup_buf(audio_stream_t * s)
{ 
	int frag; 
	int dmasize = 0; 
	char *dmabuf = 0; 
	dma_addr_t dmaphys = 0; 
	
	if (s->buffers) 
		return -EBUSY; 
	
	s->nbfrags = audio_nbfrags; 
	s->fragsize = audio_fragsize; 
	
	s->buffers = (audio_buf_t *)kmalloc(sizeof(audio_buf_t) * s->nbfrags, GFP_KERNEL); 
	if (!s->buffers) 
		goto err; 
	memset(s->buffers, 0, sizeof(audio_buf_t) * s->nbfrags); 
	
	for (frag = 0; frag < s->nbfrags; frag++) { 
		audio_buf_t *b = &s->buffers[frag]; 
		
		if (!dmasize) {		 //其实这个if语句只执行了一次
			dmasize = (s->nbfrags - frag) * s->fragsize; 
			do { 
				dmabuf = dma_alloc_coherent(NULL, dmasize, &dmaphys, GFP_KERNEL|GFP_DMA); 
				if (!dmabuf) 
					dmasize -= s->fragsize; 
			} while (!dmabuf && dmasize); 
			if (!dmabuf) 
				goto err; 
			b->master = dmasize; 
		} 
		
		b->start = dmabuf; 
		b->dma_addr = dmaphys; 
		sema_init(&b->sem, 1); 
		DPRINTK("buf %d: start %p dma %d\n", frag, b->start, b->dma_addr); 
		
		dmabuf += s->fragsize; 
		dmaphys += s->fragsize; 
		dmasize -= s->fragsize; 
	} 
	
	s->buf_idx = 0; 
	s->dma_idx = 0; 
	s->buf_len= 0; 
	s->buf = &s->buffers[0]; 
	s->dma = &s->buffers[0]; 
	s->dmaing = 0;
	s->dma_ok = 0;
	return 0; 
	
err: 
	DPRINTK(AUDIO_NAME ": unable to allocate audio memory\n "); 
	audio_clear_buf(s); 
	return -ENOMEM; 
} 
/*
//当DMA接收后要唤醒等待的at91sam9260_mixer_write
static void audio_dmaout_done_callback(s3c2410_dma_chan_t *ch, void *buf, int size, 
									   s3c2410_dma_buffresult_t result) 
{ 
	audio_buf_t *b = (audio_buf_t *) buf; 
	up(&b->sem); 
	wake_up(&b->sem.wait); 
} 
//当DMA接收后要唤醒等待的at91sam9260_mixer_read
static void audio_dmain_done_callback(s3c2410_dma_chan_t *ch, void *buf, int size, 
									  s3c2410_dma_buffresult_t result) 
{ 
	audio_buf_t *b = (audio_buf_t *) buf; 
	b->size = size; 
	up(&b->sem); 
	wake_up(&b->sem.wait); 
} 
*/
/* using when write */ 
//为安全保证，当系统掉电的时候，强烈执行这个函数来保存RAM TO FLASH
/*
static int audio_sync(struct file *file) 
{ 
	audio_stream_t *s = &output_stream; 
	audio_buf_t *b = s->buf; 
	
	DPRINTK("audio_sync\n"); 
	
	if (!s->buffers) 
		return 0; 
	
	if (b->size != 0) { 
		down(&b->sem); 
		s3c2410_dma_enqueue(s->dma_ch, (void *) b, b->dma_addr, b->size); 
		b->size = 0; 
		NEXT_BUF(s, buf); 
	} 
	
	b = s->buffers + ((s->nbfrags + s->buf_idx - 1) % s->nbfrags); 
	if (down_interruptible(&b->sem)) 
		return -EINTR; 
	up(&b->sem); 
	
	return 0; 
} 
*/
static inline int copy_from_user_mono_stereo(char *to, const char *from, int count) 
{ 
	u_int *dst = (u_int *)to; 
	const char *end = from + count; 
	
	
	if ((int)from & 0x2) { 
		u_int v; 
		get_user(v, (const u_short *)from); from += 2; 
		*dst++ = v | (v << 16); 
	} 
	
	while (from < end-2) { 
		u_int v, x, y; 
		get_user(v, (const u_int *)from); from += 4; 
/*------------------------------------------------------------------
　　| 			| 取样1 | 取样2 |
　　| 单声道 	|---------------------------------------------------
　　| 16bit量化 | 声道0 | 声道0 | 声道0 | 声道0 |
　　| 			| (低位字节) | (高位字节) | (低位字节) | (高位字节) |
　　---------------------------------------------------------------*/
		
		x = v >> 16; 
		x |= x << 16;
		
		y = v >> 16; 
		y |= y << 16;
		
		*dst++ = x;		
		*dst++ = y; 
	
	} 
	 
	if (from < end) { 
		u_int v; 
		get_user(v, (const u_short *)from); 
		*dst = v | (v << 16); 
	} 
	
	return 0; 
} 

/******************************************************
本函数用于启动DMA传输,将数据从用户缓
冲区拷贝到内核缓冲区, 置为相关标志。
*******************************************************/
void __inline__ at91sam9260_tx_dma_run(dma_addr_t data, int size)
{
	
	at91_ssc_write(iis_base + ATMEL_PDC_TPR, data );
	at91_ssc_write(iis_base + ATMEL_PDC_TCR, size );

	//at91_ssc_write(iis_base + AT91_SSC_IER, AT91_SSC_TXBUFE);	
	at91_ssc_write(iis_base + AT91_SSC_IER, AT91_SSC_ENDTX);
	at91_ssc_write(iis_base + ATMEL_PDC_PTCR,  ATMEL_PDC_TXTEN);
	at91_ssc_write(iis_base + AT91_SSC_CR, AT91_SSC_TXEN); 
	//DPRINTK("ssc state %lx\n",at91_ssc_read(iis_base + AT91_SSC_CR));
}

/* using when write */ 
static int audio_sync(struct file *file) 
{ 
	audio_stream_t *s = &output_stream; 
	audio_buf_t *b = s->dma; 
	
	DPRINTK("audio_sync\n"); 
	
	if (!s->buffers) 
		return 0; 
	
	if (b->size != 0) { 
		down(&b->sem); 
		//s3c2410_dma_enqueue(s->dma_ch, (void *) b, b->dma_addr, b->size); 
		at91sam9260_tx_dma_run(s->dma->dma_addr, s->dma->size);
		b->size = 0; 
		NEXT_BUF(s, dma); 
	} 
	
	b = s->buffers + ((s->nbfrags + s->dma_idx - 1) % s->nbfrags); 
	if (down_interruptible(&b->sem)) 
		return -EINTR; 
	up(&b->sem); 
	
	return 0; 
}

#if 1
//放音函数
//这里用到阻塞和信号灯的概念，就是把用户空间的内存数据送到DMA传输队列中等待传输去把
static ssize_t at91sam9260_audio_write(struct file *file, const char *buffer, 
									size_t count, loff_t * ppos) 
{ 
	const char *buffer0 = buffer; 
	char *buf_usr = buffer;
	audio_stream_t *s = &output_stream; 
	int chunksize, ret = 0; 
	
	//DPRINTK("%s, start count=%d\n",__FUNCTION__, count); 
	
	switch (file->f_flags & O_ACCMODE) { 
		case O_WRONLY: 
		case O_RDWR: 
			break; 
		default: 
			return -EPERM; 
	} 
	// 打开设备后，第一次写数据时，创立缓冲区。
	if (!s->buffers && audio_setup_buf(s)) 
			return -ENOMEM; 
	
	count &= ~0x03; 	 //以4为单位传输
	
	while (count > 0) { 
		audio_buf_t *b = s->buf; 

		if (file->f_flags & O_NONBLOCK) { 
			ret = -EAGAIN; 
			if (down_trylock(&b->sem)) 
				break; 
		} else { 
			ret = -ERESTARTSYS; 
			if (down_interruptible(&b->sem)) 
				break; 
		} 
	
			//立体声数据从用户空间到内核驱动缓存
		if (audio_channels == 2) { 
			chunksize = s->fragsize - b->size; 
			if (chunksize > count) 
				chunksize = count; 
			DPRINTK("stereo write %d to %d\n", chunksize, s->buf_idx); 
			//memset(b->start + b->size, 0, chunksize); 
			if (copy_from_user(b->start + b->size, buf_usr, chunksize)) { 
				up(&b->sem); 
				return -EFAULT; 
			} 
			b->size += chunksize; 
		//单声道数据从用户空间到内核驱动缓存
		} else { 
			chunksize = (s->fragsize - b->size) >> 1 ; 
			
			if (chunksize > count) 
				chunksize = count; 
			DPRINTK("mono write %d to buf_idx %d\n", chunksize*2, s->buf_idx); 
			if (copy_from_user_mono_stereo(b->start + b->size, buf_usr, chunksize))
			{ 				
				up(&b->sem); 
				return -EFAULT; 
			} 		
			b->size += chunksize*2; 
		} 
		s->buf_len++;
		DPRINTK("buf_usr addr %x\n", buf_usr); 
		buf_usr += chunksize; 
		count -= chunksize; 
	
		
		//如果这次填充缓冲片没有填满，会释放这一片的信号量，
		//继续对这一片进行操作，直到填满开的缓冲片大小为止，
		//保证了缓冲片内不会有空白区间，这对音频是致命的
		if (b->size < s->fragsize) { 
			s->buf_len--;
			up(&b->sem); 
			break; 
		} 
				 
		//启动DMA传输
		if( s->dmaing == 0 ){
			//DPRINTK("%s,dma_addr = %d\n",__FUNCTION__,s->dma->dma_addr); 
			DPRINTK("%s,first start dma!!transmit %d,dma_idx %d\n",
					__FUNCTION__,s->dma->size,s->dma_idx); 
			at91sam9260_tx_dma_run(s->dma->dma_addr, s->dma->size);
			s->dmaing = 1;
		}		
		//b->size = 0; 	
		NEXT_BUF(s, buf); 	
		
			
	} 

	
	//buffer0是要传数据的首地址，固定不动的，buffer是现在传的地址
	//通过buffer-buffer0，可以得到传了多少，只有在返回0的时候是表明
	//这次传递的包全部传完，不然系统应该通过ret得到断点处并继续	
	if ((buf_usr - buffer0)) 
		ret = buf_usr- buffer0; 
	
	DPRINTK("audio_write : end count = %d\n\n", ret); 
	
	return ret; 
} 

#else   

//放音函数
//这里用到阻塞和信号灯的概念，就是把用户空间的内存数据送到DMA传输队列中等待传输去把
static ssize_t at91sam9260_audio_write(struct file *file, const char *buffer, 
									size_t count, loff_t * ppos) 
{ 
	const char *buffer0 = buffer; 
	audio_stream_t *s = &output_stream; //
	
	int chunksize, ret = 0; 
	int only = 0;
	
	//DPRINTK("%s, start count=%d\n",__FUNCTION__, count); 
	
	switch (file->f_flags & O_ACCMODE) { 
		case O_WRONLY: 
		case O_RDWR: 
			break; 
		default: 
			return -EPERM; 
	} 
	//if (!s->buffers && audio_setup_buf(s)) 
	//		return -ENOMEM; 
	
	count &= ~0x03; 	 //以4为单位传输
	
	while (count > 0) { 
		audio_buf_t *b = s->buf; 
		audio_buf_t *b_next = b+1; 
		if (file->f_flags & O_NONBLOCK) { 
			ret = -EAGAIN; 
			if (down_trylock(&b->sem)) 
				break; 
		} else { 
			ret = -ERESTARTSYS; 
			if (down_interruptible(&b->sem)) 
				break; 
		} 
		
		if(only == 0){
			
			//立体声数据从用户空间到内核驱动缓存
			if (audio_channels == 2) { 
				chunksize = s->fragsize - b->size; 
				if (chunksize > count) 
					chunksize = count; 
				DPRINTK("%dwrite %d to %d\n", only,chunksize, s->buf_idx); 
				//memset(b->start + b->size, 0, chunksize); 
				if (copy_from_user(b->start + b->size, buffer, chunksize)) { 
					up(&b->sem); 
					return -EFAULT; 
				} 
				b->size += chunksize; 
			//单声道数据从用户空间到内核驱动缓存
			} else { 
				chunksize = (s->fragsize - b->size) >> 1; 
				
				if (chunksize > count) 
					chunksize = count; 
				DPRINTK("mono write %d to %d\n", chunksize*2, s->buf_idx); 
				if (copy_from_user_mono_stereo(b->start + b->size, buffer, chunksize))
				{ 
					up(&b->sem); 
					return -EFAULT; 
				} 
				//DPRINTK("mono write %x\n", *(s->buf->start)); 
				b->size += chunksize*2; 
			} 
			only++;
		}else
			NEXT_BUF(s, buf); 
		buffer += chunksize; 
		count -= chunksize; 
		if (b->size < s->fragsize) { 
			up(&b->sem); 
			break; 
		} 
		//启动DMA传输
		DPRINTK("%s,start dma!!transmit %d\n",__FUNCTION__,b->size); 
		at91sam9260_tx_dma_run(b->dma_addr, b->size);

		b->size = 0; 
		if(count <= 0)
			break;
	
		//NEXT_BUF(s, buf); 
		/************************************************/
		
		if (audio_channels == 2) { 
			chunksize = s->fragsize - b_next->size; 
			if (chunksize > count) 
				chunksize = count; 
			DPRINTK("%dwrite %d to %d\n",only, chunksize, s->buf_idx+1); 
			//memset(b->start + b->size, 0, chunksize); 
			if (copy_from_user(b_next->start + b_next->size, buffer, chunksize)) { 
				up(&b->sem); 
				return -EFAULT; 
			} 
			b_next->size += chunksize; 
		//单声道数据从用户空间到内核驱动缓存
		} else { 
			chunksize = (s->fragsize - b_next->size) >> 1; 
			
			if (chunksize > count) 
				chunksize = count; 
			DPRINTK("mono write %d to %d\n", chunksize*2, s->buf_idx+1); 
			if (copy_from_user_mono_stereo(b_next->start + b_next->size, buffer, chunksize))
			{ 
				up(&b->sem); 
				return -EFAULT; 
			} 
			//DPRINTK("mono write %x\n", *(s->buf->start)); 
			b_next->size += chunksize*2; 
		} 		
			
	} 
	
	if ((buffer - buffer0)) 
		ret = buffer - buffer0; 
	
	DPRINTK("audio_write : end count = %d\n\n", ret); 
	
	return ret; 
} 

#endif


/*
static int at91sam9260_mixer_ioctl(struct inode *inode, struct file *file, 
								unsigned int cmd, unsigned long arg) 
{ 
	int ret; 
	long val = 0; 
	
	switch (cmd) { 
	case SOUND_MIXER_INFO: 
		{ 
			mixer_info info; 
			strncpy(info.id, "UDA1330", sizeof(info.id)); 
			strncpy(info.name,"Philips UDA1330", sizeof(info.name)); 
			info.modify_counter = audio_mix_modcnt; 
			return copy_to_user((void *)arg, &info, sizeof(info)); 
		} 
		
	case SOUND_OLD_MIXER_INFO: 
		{ 
			_old_mixer_info info; 
			strncpy(info.id, "UDA1330", sizeof(info.id)); 
			strncpy(info.name,"Philips UDA1330", sizeof(info.name)); 
			return copy_to_user((void *)arg, &info, sizeof(info)); 
		} 
		
	case SOUND_MIXER_READ_STEREODEVS: 
		return put_user(0, (long *) arg); 
		
	case SOUND_MIXER_READ_CAPS: 
		val = SOUND_CAP_EXCL_INPUT; 
		return put_user(val, (long *) arg); 
		
	case SOUND_MIXER_WRITE_VOLUME: 
		ret = get_user(val, (long *) arg); 
		if (ret) 
			return ret; 
		uda1330_volume = 63 - (((val & 0xff) + 1) * 63) / 100; 
		uda1330_l3_address(UDA1330_REG_DATA); 
		uda1330_l3_data(uda1330_volume); 
		break; 
		
	case SOUND_MIXER_READ_VOLUME: 
		val = ((63 - uda1330_volume) * 100) / 63; 
		val |= val << 8; 
		return put_user(val, (long *) arg); 
		
	case SOUND_MIXER_READ_IGAIN: 
		val = ((31- mixer_igain) * 100) / 31; 
		return put_user(val, (int *) arg); 
		
	case SOUND_MIXER_WRITE_IGAIN: 
		ret = get_user(val, (int *) arg); 
		if (ret) 
			return ret; 
		mixer_igain = 31 - (val * 31 / 100); 
		// use mixer gain channel 1
		uda1330_l3_address(UDA1330_REG_DATA0); 
		uda1330_l3_data(EXTADDR(EXT0)); 
		uda1330_l3_data(EXTDATA(EXT0_CH1_GAIN(mixer_igain))); 
		break; 
		
	default: 
		DPRINTK("mixer ioctl %u unknown\n", cmd); 
		return -ENOSYS; 
	} 
	
	audio_mix_modcnt++; 
	return 0; 
} 
*/
//通过s3c2410_get_bus_clk获得总线时钟，在通过sample_rate来计算比例因子并返回
static int iispsr_value(int s_bit_clock, int sample_rate) 
{ 
	int i, prescaler = 0; 
	unsigned long tmpval; 
	unsigned long tmpval384; 
	unsigned long tmpval384min = 0xffff; 
	
	tmpval384 = clk_get_rate(iis_clock) / s_bit_clock; 
	
	for (i = 0; i < 32; i++) { 
		tmpval = tmpval384/(i+1); 
		if (PCM_ABS((sample_rate - tmpval)) < tmpval384min) { 
			tmpval384min = PCM_ABS((sample_rate - tmpval)); 
			prescaler = i; 
		} 
	} 
	
	DPRINTK("%s,prescaler = %d\n", __FUNCTION__,prescaler); 
	
	return prescaler; 
} 
/*
	PCLK(通俗来说就是声音的采样频率如：44.1K，它的产生是有2410系统时钟分频得到的。
　　具体过程：2410主频202M，它的APH总线频率是202/4=50M，在经过IIS的PSR（分频比例因子）
	得到的一个频率用于IIS时钟输出也可以说是同步)　这个时钟是输出给1380的，
	对于1380来说它的采样频率即：AD的频率是可以编程控制的。
　　资料中表明：
	1 可以直接用IIS穿过来的时钟就是上面解释的，
	2 可以用PLL分频后的时钟来确定AD的频率，DA同理就是了。
　　注意：如果是双声道，上面的就要变化下了，呵呵各位大哥应该都比我明白。
　　SCLK：在IIS中有2个PSR控制起的，一个用于外部B，另个用于内部A（SCLK）形成时钟用于
　　PCLK8或者16倍频 （对把这里有点不敢确定不过应该没错就是了）
*/
//通过iispsr_value(）返回的值赋给已经映射了的寄存器
static long audio_set_dsp_speed(long val) 
{ 
	unsigned int prescaler; 
	prescaler=iispsr_value(S_CLOCK_FREQ, val);
	//	| IISPSR_B(iispsr_value(S_CLOCK_FREQ, val))); 
	//writel(prescaler, iis_base + S3C2410_IISPSR); 
	at91_ssc_write(iis_base + AT91_SSC_CMR, prescaler/2);
	DPRINTK("%s,dsp_speed:%ld,prescaler:%i\n",__FUNCTION__,val,prescaler); 
	return (audio_rate = val); 
} 

//*----------------------------------------------------------------------------
//* \fn    AT91F_SSC_SetBaudrate
//* \brief Set the baudrate according to the CPU clock
//*----------------------------------------------------------------------------
__inline void AT91_SSC_SetBaudrate (       
        unsigned int mainClock, // \arg peripheral clock
        unsigned int speed)     // \arg SSC baudrate
{
        unsigned int baud_value;
        //* Define the baud rate divisor register
        if (speed == 0)
           baud_value = 0;
        else
        {
           baud_value = (unsigned int) (mainClock * 10)/(2*speed);
           if ((baud_value % 10) >= 5)
                  baud_value = (baud_value / 10) + 1;
           else
                  baud_value /= 10;
        }
		at91_ssc_write(iis_base + AT91_SSC_CMR, baud_value);
		DPRINTK("%s,baud_value = %d\n",__FUNCTION__,baud_value);
    
}


static int at91sam9260_audio_ioctl(struct inode *inode, struct file *file, 
								uint cmd, ulong arg) 
{ 
	long val; 
	
	switch (cmd) { 
	case SNDCTL_DSP_SETFMT: 
		get_user(val, (long *) arg); 
		if (val & AUDIO_FMT_MASK) { 
			audio_fmt = val; 
			break; 
		} else 
			return -EINVAL; 
		
	case SNDCTL_DSP_CHANNELS: 
		break;
	case SNDCTL_DSP_STEREO: 
		get_user(val, (long *) arg); 
		if (cmd == SNDCTL_DSP_STEREO) 
			val = val ? 2 : 1; 
		if (val != 1 && val != 2) 
			return -EINVAL; 
		audio_channels = val; 
		break; 
		
	case SOUND_PCM_READ_CHANNELS: 
		put_user(audio_channels, (long *) arg); 
		break; 
		
	case SNDCTL_DSP_SPEED: 
		get_user(val, (long *) arg); 
		//val = AT91_SSC_SetBaudrate(MCK,(u32)val); 
		val = audio_rate;
		if (val < 0) 
			return -EINVAL; 
		put_user(val, (long *) arg); 
		break; 
		
	case SOUND_PCM_READ_RATE: 
		put_user(audio_rate, (long *) arg); 
		break; 
		
	case SNDCTL_DSP_GETFMTS: 
		put_user(AUDIO_FMT_MASK, (long *) arg); 
		break; 
		
	case SNDCTL_DSP_GETBLKSIZE: 
		if(file->f_mode & FMODE_WRITE) 
			return put_user(audio_fragsize, (long *) arg); 
		else 
			return put_user(audio_fragsize, (int *) arg); 
		
	case SNDCTL_DSP_SETFRAGMENT: 
		if (file->f_mode & FMODE_WRITE) { 
			if (output_stream.buffers) 
				return -EBUSY; 
			get_user(val, (long *) arg); 
			audio_fragsize = 1 << (val & 0xFFFF); 
			if (audio_fragsize < 16) 
				audio_fragsize = 16; 
			if (audio_fragsize > 16384) 
				audio_fragsize = 16384; 
			audio_nbfrags = (val >> 16) & 0x7FFF; 
			if (audio_nbfrags < 2) 
				audio_nbfrags = 2; 
			if (audio_nbfrags * audio_fragsize > 128 * 1024) 
				audio_nbfrags = 128 * 1024 / audio_fragsize; 
			if (audio_setup_buf(&output_stream)) 
				return -ENOMEM; 
			
		} 
		/*
		if (file->f_mode & FMODE_READ) { 
			if (input_stream.buffers) 
				return -EBUSY; 
			get_user(val, (int *) arg); 
			audio_fragsize = 1 << (val & 0xFFFF); 
			if (audio_fragsize < 16) 
				audio_fragsize = 16; 
			if (audio_fragsize > 16384) 
				audio_fragsize = 16384; 
			audio_nbfrags = (val >> 16) & 0x7FFF; 
			if (audio_nbfrags < 2) 
				audio_nbfrags = 2; 
			if (audio_nbfrags * audio_fragsize > 128 * 1024) 
				audio_nbfrags = 128 * 1024 / audio_fragsize; 
			if (audio_setup_buf(&input_stream)) 
				return -ENOMEM; 
			
		} 
		*/
		break; 
		
	case SNDCTL_DSP_SYNC: 
		return 0;//audio_sync(file); 
		break;
		
	case SNDCTL_DSP_GETOSPACE: 
		{ 
			audio_stream_t *s = &output_stream; 
			audio_buf_info *inf = (audio_buf_info *) arg; 
			int err = access_ok(VERIFY_WRITE, inf, sizeof(*inf)); 
			int i; 
			int frags = 0, bytes = 0; 
			
			if (err) 
				return err; 
			for (i = 0; i < s->nbfrags; i++) { 
				if (atomic_read(&s->buffers[i].sem.count) > 0) { 
					if (s->buffers[i].size == 0) frags++; 
					bytes += s->fragsize - s->buffers[i].size; 
				} 
			} 
			put_user(frags, &inf->fragments); 
			put_user(s->nbfrags, &inf->fragstotal); 
			put_user(s->fragsize, &inf->fragsize); 
			put_user(bytes, &inf->bytes); 
			break; 
		} 
		
	case SNDCTL_DSP_GETISPACE: 
		{ 
			/*
			audio_stream_t *s = &input_stream; 
			audio_buf_info *inf = (audio_buf_info *) arg; 
			int err = verify_area(VERIFY_WRITE, inf, sizeof(*inf)); 
			int i; 
			int frags = 0, bytes = 0; 
			
			if (!(file->f_mode & FMODE_READ)) 
				return -EINVAL; 
			
			if (err) 
				return err; 
			for(i = 0; i < s->nbfrags; i++){ 
				if (atomic_read(&s->buffers[i].sem.count) > 0) 
				{ 
					if (s->buffers[i].size == s->fragsize) 
						frags++; 
					bytes += s->buffers[i].size; 
				} 
			} 
			put_user(frags, &inf->fragments); 
			put_user(s->nbfrags, &inf->fragstotal); 
			put_user(s->fragsize, &inf->fragsize); 
			put_user(bytes, &inf->bytes); 
			*/
			break; 
		} 
	case SNDCTL_DSP_RESET: 
		if (file->f_mode & FMODE_READ) { 
			//audio_clear_buf(&input_stream); 
		} 
		if (file->f_mode & FMODE_WRITE) { 
			audio_clear_buf(&output_stream); 
		} 
		return 0; 
	case SNDCTL_DSP_NONBLOCK: 
		file->f_flags |= O_NONBLOCK; 
		return 0; 
	case SNDCTL_DSP_POST: 
	case SNDCTL_DSP_SUBDIVIDE: 
	case SNDCTL_DSP_GETCAPS: 
	case SNDCTL_DSP_GETTRIGGER: 
	case SNDCTL_DSP_SETTRIGGER: 
	case SNDCTL_DSP_GETIPTR: 
	case SNDCTL_DSP_GETOPTR: 
	case SNDCTL_DSP_MAPINBUF: 
	case SNDCTL_DSP_MAPOUTBUF: 
	case SNDCTL_DSP_SETSYNCRO: 
	case SNDCTL_DSP_SETDUPLEX: 
		return -ENOSYS; 
	default: 
		break;
		//return at91sam9260_mixer_ioctl(inode, file, cmd, arg); 
	} 
	
	return 0; 
} 
void uda_wq_handler(struct work_struct *work )
{
	audio_stream_t *s = &output_stream;	
	audio_buf_t *b = s->dma;
	
	//struct uda_struct_t *my_name = container_of(work, struct uda_struct_t, uda_work);	
	//DPRINTK("Hello world, my name is %s!\n", my_name->name);
	
	s->buf_len--;
	b->size = 0;
	DPRINTK("%s,end, dma_index = %d,\t\t\tdma_len left = %d\n\n",
			__FUNCTION__,s->dma_idx,s->buf_len);	
	up(&b->sem); 
	//wake_up(&b->sem.wait); 
	//mdelay(2000);	
	if((s->buf_len == 0) && (s->dmaing == 1)){		//完成搬运
		DPRINTK("%s dma done!!\n\n",__FUNCTION__);
		s->dmaing = 0;			
	}else{		//执行下一次搬运
		NEXT_BUF(s, dma); 
		//s->buf_len--;
		//b->size = 0;
		//up(&b->sem); 
		//NEXT_BUF(s, dma); 
		//DPRINTK("%s,dma_addr = %d\n",__FUNCTION__,s->dma->dma_addr); 
		DPRINTK("%s,start dma!!transmit %d,dma_index = %d\n",__FUNCTION__,s->dma->size,s->dma_idx); 
		at91sam9260_tx_dma_run(s->dma->dma_addr,s->dma->size);
	}

}

static irqreturn_t  atmel_dma_tx_interrupt(int irq,void *dev_id)
{
	//static void __iomem *sys_base = (void __iomem *) AT91_VA_BASE_SYS;
	//audio_stream_t *s = &output_stream;	
	//audio_buf_t *b = s->dma;
	//DPRINTK("%s,imr = %lx, pdsr = %lx\n",__FUNCTION__,
	//	at91_ssc_read(iis_base + AT91_SSC_IMR),
	//	at91_ssc_read(iis_base + ATMEL_PDC_PTSR));
	//DPRINTK("%s,end dma !!buf_index = %d, dma_index = %d,left buf_len = %d\n",__FUNCTION__,
	//			s->buf_idx,s->dma_idx,s->buf_len);
	
	//sys_base = (void __iomem *) AT91_VA_BASE_SYS;	
	at91_ssc_write(sys_base + AT91_AIC_ICCR, 1 << AT91SAM9260_ID_SSC);//clear ssc interrupt
	//DPRINTK("%s,sr = %lx,imr = %lx\n",__FUNCTION__,at91_ssc_read(iis_base + AT91_SSC_SR),at91_ssc_read(iis_base + AT91_SSC_IMR));
	//at91_ssc_write(iis_base + AT91_SSC_IDR, AT91_SSC_TXBUFE);
	at91_ssc_write(iis_base + AT91_SSC_IDR, AT91_SSC_ENDTX);
	at91_ssc_write(iis_base + AT91_SSC_CR, AT91_SSC_TXDIS);
	at91_ssc_write(iis_base + ATMEL_PDC_PTCR,  ATMEL_PDC_TXTDIS);
	
	at91_ssc_write(iis_base + ATMEL_PDC_RNPR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_RNCR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_TNPR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_TNCR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_RPR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_RCR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_TPR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_TCR, (u32 )0 );
	/*
	up(&b->sem); 
	wake_up(&b->sem.wait); 
	//s->dma_ok = 1;
	
	if((s->buf_len == AUDIO_NBFRAGS_DEFAULT) && (s->dmaing == 1)){
		DPRINTK("%s dma done!!\n",__FUNCTION__);
		s->dmaing = 0;
		return IRQ_HANDLED;		
	}
	
	NEXT_BUF(s, dma); 
	//s->buf_len++;
	//at91sam9260_tx_dma_run(s->dma->dma_addr,s->dma->size);
	*/
	queue_work(uda_wq, &(uda_name.uda_work));
	//schedule_work(&(uda_name.uda_work));
	//DPRINTK("%s irq done!!\n",__FUNCTION__);
	return IRQ_HANDLED;
}

static int at91sam9260_audio_open(struct inode *inode, struct file *file) 
{ 
	int retval;
	//audio_stream_t *s = &output_stream; 
	int cold = !audio_active; 
	
	DPRINTK("audio_open\n"); 
	if ((file->f_flags & O_ACCMODE) == O_RDONLY) { 
		if (audio_rd_refcount || audio_wr_refcount) 
			return -EBUSY; 
		audio_rd_refcount++; 
	} else if ((file->f_flags & O_ACCMODE) == O_WRONLY) { 
		if (audio_wr_refcount) 
			return -EBUSY; 
		audio_wr_refcount++; 
	} else if ((file->f_flags & O_ACCMODE) == O_RDWR) { 
		if (audio_rd_refcount || audio_wr_refcount) 
			return -EBUSY; 
		audio_rd_refcount++; 
		audio_wr_refcount++; 
	} else 
		return -EINVAL; 
	
	if (cold) { 
		//audio_rate = AUDIO_RATE_DEFAULT; 
		//audio_channels = AUDIO_CHANNELS_DEFAULT; 
		//audio_fragsize = AUDIO_FRAGSIZE_DEFAULT; 
		//audio_nbfrags = AUDIO_NBFRAGS_DEFAULT; 
		if ((file->f_mode & FMODE_WRITE)){ 
			//init_at91sam9260_iis_bus_tx(); 
			audio_clear_buf(&output_stream); 
		
		retval = request_irq(AT91SAM9260_ID_SSC, 
								atmel_dma_tx_interrupt, 
								IRQF_DISABLED, 
								AUDIO_TXEMT_IRQ, 
								NULL);
		uda_name.name ="Roy";
		uda_wq = create_workqueue("uda_wq");
		INIT_WORK(&(uda_name.uda_work),uda_wq_handler);		
		//queue_work(uda_wq, &(uda_name.uda_work));
		
		} 
		
		/*
		if ((file->f_mode & FMODE_READ)){ 
			init_at91sam9260_iis_bus_rx(); 
			audio_clear_buf(&input_stream); 
		} */
	} 
	return 0; 
} 


static int at91sam9260_mixer_open(struct inode *inode, struct file *file) 
{ 
	return 0; 
} 


static int at91sam9260_audio_release(struct inode *inode, struct file *file) 
{ 
	DPRINTK("audio_release\n"); 
	/*
	if (file->f_mode & FMODE_READ) { 
		if (audio_rd_refcount == 1) 
			audio_clear_buf(&input_stream); 
		audio_rd_refcount = 0; 
	} 
	*/
	if(file->f_mode & FMODE_WRITE) { 
		if (audio_wr_refcount == 1) { 
			//audio_sync(file); 
			
			
			destroy_workqueue(uda_wq);
			free_irq(AT91SAM9260_ID_SSC, NULL);
			audio_clear_buf(&output_stream); 
			audio_wr_refcount = 0; 
			//at91_ssc_write(sys_base + AT91_AIC_ICCR, 1 << AT91SAM9260_ID_SSC);//clear ssc interrupt
		} 
	} 	
	return 0; 
} 


static int at91sam9260_mixer_release(struct inode *inode, struct file *file) 
{ 
	return 0; 
} 


static struct file_operations at91sam9260_audio_fops = { 
//	llseek: at91sam9260_audio_llseek, 
	write: at91sam9260_audio_write, 
//	read: at91sam9260_audio_read, 
//	poll: at91sam9260_audio_poll, 
	ioctl: at91sam9260_audio_ioctl, 
	open: at91sam9260_audio_open, 
	release: at91sam9260_audio_release 
}; 

static struct file_operations at91sam9260_mixer_fops = { 
	//ioctl: at91sam9260_mixer_ioctl, 
	open: at91sam9260_mixer_open, 
	release: at91sam9260_mixer_release 
}; 

static void init_uda1330(void) 
{ 
//	unsigned long flags; 
	at91_set_gpio_output(L3CLK,1);
	at91_set_gpio_output(L3DATA,0);
	at91_set_gpio_output(L3MOD,1);
	//at91_set_gpio_output(L3CLK,0);
	uda1330_volume = 62 - ((DEF_VOLUME * 61) / 100); 

	
	//local_irq_save(flags); 
	
	//at91_set_gpio_value(L3MOD,1); 	
	//at91_set_gpio_value(L3CLK,1); 

	//写L3状态status寄存器地址
	uda1330_l3_address(UDA1330_REG_STATUS); 
	//写L3状态status寄存器数据，
	uda1330_l3_data(STATUS | STATUS_SC_384FS | STATUS_IF_I2S); 

	udelay(5);
	
	//写L3数据DATA寄存器地址
	uda1330_l3_address(UDA1330_REG_DATA); 
	
	//写数据寄存器，音量大小
	//uda1330_l3_data(DATA0 | DATA0_VOLUME_MASK); 
	/*

	//连续写时钟模式
	at91_set_gpio_output(L3MOD,0);
	udelay(1);
	at91_set_gpio_output(L3MOD,1);
	*/
	// 最大音量
	uda1330_l3_data(DATA0 | DATA0_VOLUME(60)); 
	/*连续写时钟模式
	at91_set_gpio_output(L3MOD,0);
	udelay(5);
	at91_set_gpio_output(L3MOD,1);
	*/
	//连续写数据寄存器，
	//udelay(1);
	//uda1330_l3_address(UDA1330_REG_DATA);
	uda1330_l3_address(UDA1330_REG_DATA);
	uda1330_l3_data(DATA1 | DATA_DEEMP_44KHz | DATA_MUTE); 
	
	//local_irq_restore(flags); 
} 

static void init_at91sam9260_iis_bus(void)
{ 

	audio_rate = AUDIO_RATE_DEFAULT; 
	audio_channels = AUDIO_CHANNELS_DEFAULT; 
	audio_fragsize = AUDIO_FRAGSIZE_DEFAULT; 
	audio_nbfrags = AUDIO_NBFRAGS_DEFAULT; 

	//PB16设置BCK
	at91_set_A_periph(AT91_PIN_PB16,0);
	//PB17设置WS
	at91_set_A_periph(AT91_PIN_PB17,0);
	//PB18设置DATA
	at91_set_A_periph(AT91_PIN_PB18,0);

	at91_ssc_write(iis_base + AT91_SSC_CR, //软复位ssc，禁用输入输出功能
			AT91_SSC_RXDIS | AT91_SSC_TXDIS |AT91_SSC_SWRST );
	at91_ssc_write(iis_base + AT91_SSC_IDR, (u32)0xFFFFFFFF );
	clk_disable(iis_clock);

	/* 清除DMA发送及接收寄存器 */	
	at91_ssc_write(iis_base + ATMEL_PDC_PTCR, ATMEL_PDC_RXTDIS | ATMEL_PDC_TXTDIS);
	at91_ssc_write(iis_base + ATMEL_PDC_RNPR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_RNCR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_TNPR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_TNCR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_RPR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_RCR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_TPR, (u32 )0 );
	at91_ssc_write(iis_base + ATMEL_PDC_TCR, (u32 )0 );

	//设置ssc时钟分频
	AT91_SSC_SetBaudrate(MCK,
			FILE_SAMPLING_FREQ * (BITS_BY_SLOT * SLOT_BY_FRAME));                    
	//使能发送中断             
	//at91_ssc_write(iis_base + AT91_SSC_IER, 1 );	                                                        
	/* 设置发送时钟模式寄存器
	 * 选择预分频时钟，时钟持续输出到TK,上升沿触发数据发送，
	 * 数据传输启动延时为1位，
	 * 
	 */
	at91_ssc_write(iis_base + AT91_SSC_TCMR,
				I2S_ASY_MASTER_TX_SETTING(BITS_BY_SLOT,SLOT_BY_FRAME));	
	/* 设置发送帧模式寄存器 */
	at91_ssc_write(iis_base + AT91_SSC_TFMR,
				I2S_ASY_TX_FRAME_SETTING(BITS_BY_SLOT,SLOT_BY_FRAME));
	
	//setup the prescaler 
	//audio_set_dsp_speed(audio_rate); 
	clk_enable(iis_clock);
	//使能发送空中断
	//at91_ssc_write(iis_base + AT91_SSC_IDR, AT91_SSC_TXBUFE);
	//at91_ssc_write(iis_base + ATMEL_PDC_PTCR,  ATMEL_PDC_TXTDIS);
	//at91_ssc_write(iis_base + AT91_SSC_CR, AT91_SSC_TXDIS);
	
	//at91_ssc_write(iis_base + AT91_SSC_IER, AT91_SSC_TXBUFE);
	//at91_ssc_write(iis_base + AT91_SSC_IMR, AT91_SSC_TXBUFE);
	/* 使能发送 */
	//at91_ssc_write(iis_base + AT91_SSC_CR, AT91_SSC_TXEN);
	//at91_ssc_write(iis_base + AT91_SSC_THR, 0xaaaaaaaa);
}


static int at91sam9260_iis_probe(void) 
{ 
	//int retval=0;
/**************************** 内存映射 ****************************************/	

	iis_base = ioremap(AT91SAM9260_BASE_SSC, 0x128);
	if (iis_base == 0) { 
		DPRINTK(KERN_INFO PFX "failed to ioremap() region\n"); 
		return -EINVAL; 
	} 	
	iis_clock = clk_get(NULL, "ssc_clk"); 
	if (iis_clock == NULL) { 
		DPRINTK(KERN_INFO PFX "failed to find clock source\n"); 
		return -ENOENT; 
	} 
	clk_enable(iis_clock);

/*
	retval = request_irq(AT91SAM9260_ID_SSC, 
								atmel_dma_tx_interrupt, 
								IRQF_DISABLED, 
								AUDIO_TXEMT_IRQ, 
								NULL);
	uda_name.name ="Roy";
	uda_wq = create_workqueue("uda_wq");
	INIT_WORK(&(uda_name.uda_work),uda_wq_handler);		
	//queue_work(uda_wq, &(uda_name.uda_work));
	*/
/**************************** 硬件初始化 **************************************/	

	init_at91sam9260_iis_bus(); 	
	init_uda1330(); 

/*****************************注册设备*****************************************/	
	
	audio_dev_dsp = register_sound_dsp(&at91sam9260_audio_fops, -1); 
	audio_dev_mixer = register_sound_mixer(&at91sam9260_mixer_fops, -1); 
	
	DPRINTK(AUDIO_NAME_VERBOSE " %s,initialize done\n",__FUNCTION__); 	
	return 0; 
} 


static int at91sam9260_iis_remove(void) { 
	unregister_sound_dsp(audio_dev_dsp); 
	unregister_sound_mixer(audio_dev_mixer); 
	//audio_clear_dma(&output_stream,&s3c2410iis_dma_out); 
	//audio_clear_dma(&input_stream,&s3c2410iis_dma_in); /* input */ 
	
	at91_ssc_write(iis_base + AT91_SSC_CR, //软复位ssc，禁用输入输出功能
					AT91_SSC_RXDIS | AT91_SSC_TXDIS |AT91_SSC_SWRST );
	at91_ssc_write(iis_base + AT91_SSC_IDR, (u32)0xFFFFFFFF );
	
	clk_disable(iis_clock);
	clk_put(iis_clock);
	
	//destroy_workqueue(uda_wq);
	//free_irq(AT91SAM9260_ID_SSC, NULL);
	DPRINTK(AUDIO_NAME_VERBOSE " unloaded\n\n"); 
	return 0; 
} 

/*
static struct device_driver at91sam9260_iis_driver = { 
	.name = "at91sam9260-iis", 
		.bus = &platform_bus_type, 
		.probe = at91sam9260_iis_probe, 
		.remove = at91sam9260_iis_remove, 
}; 
*/
static int __init at91sam9260_uda1330_init(void) { 
	//memzero(&input_stream, sizeof(audio_stream_t)); 
	DPRINTK(AUDIO_NAME_VERBOSE " %s,initialized\n",__FUNCTION__); 	
	memzero(&output_stream, sizeof(audio_stream_t));
	at91sam9260_iis_probe();
	return 0;
	//return driver_register(&at91sam9260_iis_driver); 
} 

static void __exit at91sam9260_uda1330_exit(void) { 
	//driver_unregister(&at91sam9260_iis_driver); 
	
	at91sam9260_iis_remove();
} 


module_init(at91sam9260_uda1330_init); 
module_exit(at91sam9260_uda1330_exit); 

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Roy<luranran@gmail.com>"); 
MODULE_DESCRIPTION("at91sam9260 uda1330 sound driver"); 
