/*
 * flash.h
 * NOR FLASH k8d6*16utm (Samsung) 芯片驱动
 */

#include <asm/arch/gpio.h>

#define FLASH_PHY_BASE    0x10000000    //flash物理地址
static unsigned long flash_virt_base;

//READY=1, BUSY=0
#define FLASHIO_RYBY		AT91_PIN_PA19
#define FLASH_DELAYTIME		500
#define FLASH_POLLTIME		1000000

#define flash_writew(v, a)	(*(volatile unsigned short __force *)(a) = (v))
#define flash_readw(a)		(*(volatile unsigned short __force *)(a))

static void inline flash_program_delay(void)
{
	int i;

	for(i=0; i<FLASH_DELAYTIME; i++);
}

static int flash_program_wait(void)
{
	int i;

	flash_program_delay();  //软件延时

	for(i=0; i<FLASH_POLLTIME; i++) {
		if(at91_get_gpio_value(FLASHIO_RYBY)) break;//判断标志位，是否允许编程
	}

	if(i >= FLASH_POLLTIME) return 1;  //超时，返回1
	else return 0;
}
//擦除flash等待
static int flash_erase_wait(void)
{
	int i;

	flash_program_delay();//flash编程等待

	for(i=0; i<FLASH_POLLTIME; i++) {
		if(!flash_program_wait()) break;
	}

	if(i >= FLASH_POLLTIME) return 1;
	else return 0;
}
//flash写入一个字，形参为首地址和要写入的数据
static int flash_writeword(unsigned short *addr, unsigned short data)
{
	unsigned short *pop;
    //首先将固定数据0xaa，0x55，0xa0写到固定地址
	pop = (unsigned short *)flash_virt_base + 0x555;
	flash_writew(0xaa, pop);

	pop = (unsigned short *)flash_virt_base + 0x2aa;
	flash_writew(0x55, pop);

	pop = (unsigned short *)flash_virt_base + 0x555;
	flash_writew(0xa0, pop);

	flash_writew(data, addr);  //写入要写的地址和数据

	return(flash_program_wait());
}
//将ram数据写入到flash
//入口参数:flash首地址，ram首地址,数据长度len
static int flash_writeblock(unsigned short *flash_addr, unsigned short *ram_addr, unsigned long len)
{
	if(len&0x01) len = (len>>1)+1;
	else len >>= 1;
    //一个字一个字的写
	while(len) {
		flash_writeword(flash_addr++, *ram_addr++);  
		len--;
	}

	return 0;
}
//擦除flash块，形参为首地址
static int flash_eraseblk(unsigned short *addr)
{
	unsigned short *pop;

	pop = (unsigned short *)flash_virt_base + 0x555;
	flash_writew(0xaa, pop);

	flash_program_delay();

	pop = (unsigned short *)flash_virt_base + 0x2aa;
	flash_writew(0x55, pop);

	flash_program_delay();

	pop = (unsigned short *)flash_virt_base + 0x555;
	flash_writew(0x80, pop);

	flash_program_delay();

	pop = (unsigned short *)flash_virt_base + 0x555;
	flash_writew(0xaa, pop);

	flash_program_delay();

	pop = (unsigned short *)flash_virt_base + 0x2aa;
	flash_writew(0x55, pop);

	flash_program_delay();

	flash_writew(0x30, addr); //写入0x30到要擦除的flash地址

	flash_erase_wait();

	return 0;
}

extern unsigned long at91_norflash_virtaddrbase(void);
static void flash_init(void)
{
	at91_set_gpio_input(FLASHIO_RYBY, 1);
	flash_virt_base = at91_norflash_virtaddrbase();
}
