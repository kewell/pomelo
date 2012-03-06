#if 0
/*                                                   
 * sound.c   
 * 先录制几秒种音频数据，将其存放在内存缓冲区中，然后再进行回放，
 * 其所有的功能都是通过读写/dev/dsp设备文件来完成                                        
 */                                                  
#include <unistd.h>                                  
#include <fcntl.h>                                   
#include <sys/types.h>                               
#include <sys/ioctl.h>                               
#include <stdlib.h>                                  
#include <stdio.h>  
#include <string.h>
#include <linux/soundcard.h>                         
#define LENGTH 3    /* 存储秒数 */                   
#define RATE 44100   /* 采样频率 */                   
#define SIZE 16      /* 量化位数 */                   
#define CHANNELS 1  /* 声道数目 */                   
/* 用于保存数字音频数据的内存缓冲区 */               
unsigned char sound_buf[LENGTH*RATE*SIZE*CHANNELS/8];      
int main()                                           
{                                                    
	int fd;	/* 声音设备的文件描述符 */                 
	int arg;	/* 用于ioctl调用的参数 */                
	int status;   /* 系统调用的返回值 */   
	int i=0;
	memset(sound_buf,1,LENGTH*RATE*SIZE*CHANNELS/8);
	for(i=0;i<LENGTH*RATE*SIZE*CHANNELS/8;i++){

		if(i%2==0)
			sound_buf[i]=0x55;
		else
			sound_buf[i]=0xff;
	}
	for(i=0;i<5;i++){
		printf("%x	",sound_buf[i]);
	}	

	/* 打开声音设备 */                                 
	fd = open("/dev/dsp", O_WRONLY);                     
	if (fd < 0) {                                      
		perror("open of /dev/dsp failed");               
	exit(1);                                         
	} 

#if 0	
	/* 设置采样时的量化位数 */      
	
	arg = SIZE;                                        
	status = ioctl(fd, SOUND_PCM_WRITE_BITS, &arg);    
	if (status == -1)                                  
		perror("SOUND_PCM_WRITE_BITS ioctl failed");     
	if (arg != SIZE)                                   
		perror("unable to set sample size");  
	
	/* 设置采样时的声道数目 */ 
	
	arg = CHANNELS;                                    
	status = ioctl(fd, SOUND_PCM_WRITE_CHANNELS, &arg);
	if (status == -1)                                  
		perror("SOUND_PCM_WRITE_CHANNELS ioctl failed"); 
	if (arg != CHANNELS)                               
		perror("unable to set number of channels");  
	
	/* 设置采样时的采样频率 */
	
	arg = RATE;                                        
	status = ioctl(fd, SOUND_PCM_WRITE_RATE, &arg);    
	if (status == -1)                                  
		perror("SOUND_PCM_WRITE_WRITE ioctl failed"); 
#endif	
	/* 循环，直到按下Control-C */                      
	                                      
	//printf("Say something:\n");                      
	//status = read(fd, buf, sizeof(buf)); /* 录音 */  
	//if (status != sizeof(buf))                       
	 // perror("read wrong number of bytes");          
	printf("You said:%d = %d\n",sizeof(sound_buf),LENGTH*RATE*SIZE*CHANNELS/8);                           
	status = write(fd, sound_buf, sizeof(sound_buf)); /* 回放 */ 
	
	if (status != sizeof(sound_buf))                       
		perror("wrote wrong number of bytes");         
	/* 在继续录音前等待回放结束 */                   
	//status = ioctl(fd, SOUND_PCM_SYNC, 0);           
	//if (status == -1)                                
	//  perror("SOUND_PCM_SYNC ioctl failed");   
	close(fd);
	return 0;
                                                  
}
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/soundcard.h>

#define OPEN_DSP_FAILED     0x00000001    /*open dsp failed!*/
#define SAMPLERATE_STATUS     0x00000002    /*samplerate status failed*/
#define SET_SAMPLERATE_FAILED  0x00000003       /*set samplerate failed*/
#define CHANNELS_STATUS       0x00000004    /*Channels status failed*/
#define SET_CHANNELS_FAILED    0x00000005    /*set channels failed*/
#define FMT_STATUS       0x00000006    /*FMT status failed*/
#define SET_FMT_FAILED     0x00000007    /*set fmt failed*/
#define OPEN_FILE_FAILED        0x00000008    /*opem filed failed*/
/*  RIFF WAVE Chunk
以'FIFF'作为标示，然后紧跟着为size字段，该size是整个wav文件大小减去ID
　　和Size所占用的字节数，即FileLen - 8 = Size。然后是Type字段，为'WAVE'，表
　　示是wav文件。
*/
struct RIFF_H
{
	char szRiffID[4]; // 'R','I','F','F'
	unsigned int dwRiffSize;
	char szRiffFormat[4]; // 'W','A','V','E'
};
/*
						Format Chunk
　　====================================================================
　　| | 字节数 | 具体内容 |
　　====================================================================
　　| ID | 4 Bytes | 'fmt ' |
　　--------------------------------------------------------------------
　　| Size | 4 Bytes | 数值为16或18，18则最后又附加信息 |
　　-------------------------------------------------------------------- ----
　　| FormatTag | 2 Bytes | 编码方式，一般为0x0001 | |
　　-------------------------------------------------------------------- |
　　| Channels | 2 Bytes | 声道数目，1--单声道；2--双声道 | |
　　-------------------------------------------------------------------- |
　　| SamplesPerSec | 4 Bytes | 采样频率 | |
　　-------------------------------------------------------------------- |
　　| AvgBytesPerSec| 4 Bytes | 每秒所需字节数 | |===> WAVE_FORMAT
　　-------------------------------------------------------------------- |
　　| BlockAlign | 2 Bytes | 数据块对齐单位(每个采样需要的字节数) | |
　　-------------------------------------------------------------------- |
　　| BitsPerSample | 2 Bytes | 每个采样需要的bit数 | |
　　-------------------------------------------------------------------- |
　　| | 2 Bytes | 附加信息（可选，通过Size来判断有无） | |
　　-------------------------------------------------------------------- ----
　　					图3 Format Chunk

　　以'fmt '作为标示。一般情况下Size为16，此时最后附加信息没有；如果为18
　　则最后多了2个字节的附加信息。主要由一些软件制成的wav格式中含有该2个字节的
　　附加信息。
　　结构定义如下：
*/
struct WAVE_FORMAT
{
	unsigned short wFormatTag;
	unsigned short wChannels;
	unsigned int dwSamplesPerSec;
	unsigned int dwAvgBytesPerSec;
	unsigned short wBlockAlign;
	unsigned short wBitsPerSample;
	//unsigned short accb;
};
struct FMT_BLOCK
{
	char szFmtID[4]; // 'f','m','t',' '
	unsigned int dwFmtSize;
	struct WAVE_FORMAT wavFormat;
};
/*
Fact Chunk
　　==================================
　　| |所占字节数| 具体内容 |
　　==================================
　　| ID | 4 Bytes | 'fact' |
　　----------------------------------
　　| Size | 4 Bytes | 数值为4 |
　　----------------------------------
　　| data | 4 Bytes | |
　　----------------------------------
　　图4 Fact Chunk
　　Fact Chunk是可选字段，一般当wav文件由某些软件转化而成，则包含该Chunk。
　　结构定义如下：
*/

struct FACT_BLOCK
{	
	char szFactID[4]; // 'f','a','c','t'
	unsigned int dwFactSize;
	unsigned int data;
};
/*
Data Chunk
　　==================================
　　| |所占字节数| 具体内容 |
　　==================================
　　| ID | 4 Bytes | 'data' |
　　----------------------------------
　　| Size | 4 Bytes | |
　　----------------------------------
　　| data | | |
　　----------------------------------
　　图5 Data Chunk
　　Data Chunk是真正保存wav数据的地方，以'data'作为该Chunk的标示。然后是
　　数据的大小。紧接着就是wav数据。根据Format Chunk中的声道数以及采样bit数，
　　wav数据的bit位置可以分成以下几种形式：
　　---------------------------------------------------------------------
　　| 单声道 | 取样1 | 取样2 | 取样3 | 取样4 |
　　| |--------------------------------------------------------
　　| 8bit量化 | 声道0 | 声道0 | 声道0 | 声道0 |
　　---------------------------------------------------------------------
　　| 双声道 | 取样1 | 取样2 |
　　| |--------------------------------------------------------
　　| 8bit量化 | 声道0(左) | 声道1(右) | 声道0(左) | 声道1(右) |
　　---------------------------------------------------------------------
　　| | 取样1 | 取样2 |
　　| 单声道 |--------------------------------------------------------
　　| 16bit量化 | 声道0 | 声道0 | 声道0 | 声道0 |
　　| | (低位字节) | (高位字节) | (低位字节) | (高位字节) |
　　---------------------------------------------------------------------
　　| | 取样1 |
　　| 双声道 |--------------------------------------------------------
　　| 16bit量化 | 声道0(左) | 声道0(左) | 声道1(右) | 声道1(右) |
　　| | (低位字节) | (高位字节) | (低位字节) | (高位字节) |
　　---------------------------------------------------------------------
　　图6 wav数据bit位置安排方式
　　Data Chunk头结构定义如下：
*/
struct DATA_BLOCK
{
	char szDataID[4]; // 'd','a','t','a'
	unsigned int dwDataSize;
};

#pragma pack(2)

 struct SOUND_FILE_HEAD{
	char szRiffID[4]; // 'R','I','F','F'	
	unsigned int dwRiffSize;
	char szRiffFormat[4]; // 'W','A','V','E'

	char szFmtID[4]; // 'f','m','t',' '
	unsigned int dwFmtSize;

	unsigned short wFormatTag;
	unsigned short wChannels;
	unsigned int dwSamplesPerSec;
	unsigned int dwAvgBytesPerSec;
	unsigned short wBlockAlign;
	unsigned short wBitsPerSample;
	unsigned short accb;

	char szFactID[4]; // 'f','a','c','t'
	unsigned int dwFactSize;
	unsigned int factdata;

	char szDataID[4]; // 'd','a','t','a'
	unsigned int dwDataSize;
	/*
	struct RIFF_H riff_h;	
	struct FMT_BLOCK fmt_blk;	
	struct FACT_BLOCK fact;
	struct DATA_BLOCK data_blk;
	*/
};
#pragma pack()


//struct SOUND_FILE_HEAD sound_file_h;

int Uda1330Audio_Play(char *pathname,int nSampleRate,int nChannels,int fmt)
{
	int dsp_fd,mix_fd,status,arg;
	dsp_fd = open("/dev/dsp" , O_WRONLY);   /*open dsp*/
	if(dsp_fd < 0) 
		return  OPEN_DSP_FAILED;
#if 1
	arg = nSampleRate;
	
	status = ioctl(dsp_fd,SOUND_PCM_WRITE_RATE,&arg); /*set samplerate*/
	if(status < 0)	{
		close(dsp_fd);
		return SAMPLERATE_STATUS;
	} 
	if(arg != nSampleRate){
		close(dsp_fd);
		return SET_SAMPLERATE_FAILED;
	} 
	arg = nChannels;  /*set channels*/   
	status = ioctl(dsp_fd, SOUND_PCM_WRITE_CHANNELS, &arg); 
	if(status < 0){
		close(dsp_fd);
		return CHANNELS_STATUS;
	} 
	if( arg != nChannels){
		close(dsp_fd);
		return SET_CHANNELS_FAILED;
	}
	arg = fmt; /*set bit fmt*/
	status = ioctl(dsp_fd, SOUND_PCM_WRITE_BITS, &arg); 
	if(status < 0)
	{
		close(dsp_fd);
		return FMT_STATUS;
	} 
	if(arg != fmt)
	{
		close(dsp_fd);
		return SET_FMT_FAILED;
	}/*到此设置好了DSP的各个参数*/  
#endif
	
	struct SOUND_FILE_HEAD *sound_file_h;
	FILE *file_fd = fopen(pathname,"r");
	if(file_fd == NULL)
	{
		close(dsp_fd);
		return OPEN_FILE_FAILED;
	}
	int num = 2*nChannels*nSampleRate*fmt/8;
	int get_num; 
	char buf[num];
	
	char buf_temp[100];
	char *buf_t;
	buf_t = buf_temp;
	get_num = fread(buf_t, sizeof(struct SOUND_FILE_HEAD), 1, file_fd);
	//printf("get_num = %d,struct size=%d\n ",get_num,sizeof(struct SOUND_FILE_HEAD));
	sound_file_h = (struct SOUND_FILE_HEAD *)buf_t;
	/*
	printf("rffid  = %s,size = %d,format = %s\n ",sound_file_h->szRiffID,
												sound_file_h->dwRiffSize,
												sound_file_h->szRiffFormat);
	printf("fmtid  = %s,size = %d,channels = %d,fs = %d,datalign = %d\n ",sound_file_h->szFmtID,
												sound_file_h->dwFmtSize,
												sound_file_h->wChannels,
												sound_file_h->dwSamplesPerSec,
												sound_file_h->wBlockAlign);
	printf("factid = %s,size = %d\n ",sound_file_h->szFactID,
											sound_file_h->dwFactSize);
	printf("dataid = %s,size = %d\n ",sound_file_h->szDataID,
												sound_file_h->dwDataSize);
	*/
	printf("fileinfo:channels %d, Fs %d Hz, fmt %d bit, size %d B, %d Kbps,%3.1f s\n",
												
												sound_file_h->wChannels,
												sound_file_h->dwSamplesPerSec,
												sound_file_h->wBitsPerSample,
												sound_file_h->dwDataSize,
												sound_file_h->dwAvgBytesPerSec*8/1000,
												(double)(100*sound_file_h->dwDataSize/sound_file_h->dwAvgBytesPerSec)/100);
	while(feof(file_fd) == 0)
	{
		get_num = fread(buf, 1, num, file_fd);
		printf("get_num = %d, num = %d\n",get_num,num);
		write(dsp_fd,buf,get_num); 
		if((get_num != num) && (feof(file_fd) == 0))
		{			
			close(dsp_fd);
			fclose(file_fd);
			return -1;
		} 
	}
	sleep(5);
	close(dsp_fd);
	fclose(file_fd);
	return 0; 
}
/*
*test
*/
int main()
{	
	int value;
	value = Uda1330Audio_Play("test8km16.wav",8000,1,16);
	fprintf(stderr,"value is %d",value);
	
	return 0;
} 
#endif
