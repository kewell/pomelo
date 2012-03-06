/*
 * linux/arch/arm/mach-at91/board-sam9260ek.c
 *
 *  Copyright (C) 2005 SAN People
 *  Copyright (C) 2006 Atmel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/irq.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
/* roy add */
#include <linux/mtd/physmap.h>
/* add end */
#include <asm/arch/board.h>
#include <asm/arch/gpio.h>
#include <asm/arch/at91sam926x_mc.h>

#include "generic.h"


/*
 * Serial port configuration.
 *    0 .. 5 = USART0 .. USART5
 *    6      = DBGU
 */
static struct at91_uart_config __initdata ek_uart_config = {
	.console_tty	= 0,				/* ttyS0 */
	.nr_tty		= 7,
	.tty_map	= { 6, 0, 1, 2, 3, 4, 5 }	/*roy add tty up to 1-5, ttyS0, ..., ttyS6 */
};

static void __init ek_map_io(void)
{
	/* Initialize processor: 18.432 MHz crystal */
	at91sam9260_initialize(18432000);

	/* Setup the serial ports and console */
	at91_init_serial(&ek_uart_config);
}

static void __init ek_init_irq(void)
{
	at91sam9260_init_interrupts(NULL);
}


/*
 * USB Host port
 */
static struct at91_usbh_data __initdata ek_usbh_data = {
	.ports		= 2,
};

/*
 * USB Device port
 */
static struct at91_udc_data __initdata ek_udc_data = {
	.vbus_pin	= AT91_PIN_PC5,
	.pullup_pin	= 0,		/* pull-up driven by UDC */
};


/*
 * SPI devices.
 */
static struct spi_board_info ek_spi_devices[] = {
#if !defined(CONFIG_MMC_AT91)
	{	/* tlv1504 */
		.modalias	= "tlv1504",		//roy modify
		.chip_select	= 0,
		.max_speed_hz	= 15 * 1000 * 1000,
		.bus_num	= 0,
	},
#if defined(CONFIG_MTD_AT91_DATAFLASH_CARD)
	{	/* DataFlash card */
		.modalias	= "mtd_dataflash",
		.chip_select	= 0,
		.max_speed_hz	= 15 * 1000 * 1000,
		.bus_num	= 0,
	},
#endif
#endif
#if defined(CONFIG_SND_AT73C213) || defined(CONFIG_SND_AT73C213_MODULE)
	{	/* AT73C213 DAC */
		.modalias	= "at73c213",
		.chip_select	= 0,
		.max_speed_hz	= 10 * 1000 * 1000,
		.bus_num	= 1,
	},
#endif
};


/*
 * MACB Ethernet device
 */
static struct at91_eth_data __initdata ek_macb_data = {
	.phy_irq_pin	= AT91_PIN_PA7,
	.is_rmii	= 1,
};


/*
 * NAND flash
 */
static struct mtd_partition __initdata ek_nand_partition[] = {
	{
		.name	= "Partition 1",
		.offset	= 0,
		.size	= 256 * 1024,
	},
	{
		.name	= "Partition 2",
		.offset	= 256 * 1024,
		.size	= MTDPART_SIZ_FULL,
	},
};

static struct mtd_partition * __init nand_partitions(int size, int *num_partitions)
{
	*num_partitions = ARRAY_SIZE(ek_nand_partition);
	return ek_nand_partition;
}

static struct at91_nand_data __initdata ek_nand_data = {
	.ale		= 21,
	.cle		= 22,
//	.det_pin	= ... not connected
	.rdy_pin	= AT91_PIN_PC13,
	.enable_pin	= AT91_PIN_PC14,
	.partition_info	= nand_partitions,
#if defined(CONFIG_MTD_NAND_AT91_BUSWIDTH_16)
	.bus_width_16	= 1,
#else
	.bus_width_16	= 0,
#endif
};



// roy add

/*
*NOR flash
*/
#define EK_FLASH_BASE_1	AT91_CHIPSELECT_0
#define EK_FLASH_SIZE_1	0x800000             /*roy modify*/
#define EK_FLASH_BASE_2	AT91_CHIPSELECT_2
#define EK_FLASH_SIZE_2	0x800000             /*roy modify*/

static struct mtd_partition  ek_norflash_partition_1[] = {

	[0] = {
		.name	= "rootfs(cramfs)",
		.offset = 0x00200000,
		.size	= 0x00100000,
	},
	[1] = {
		.name	= "user(jffs2)",
		.offset	= 0x00300000,
		.size	= 0x00500000,
	},
};

static struct mtd_partition  ek_norflash_partition_2[] = {
	[0] = {
		.name	= "data(jffs2)",
		.offset	= 0x00000000,
		.size	= 0x00800000,
	},
};

static struct physmap_flash_data ek_norflash_data_1 = {
	.width		= 2,	
	.nr_parts	= 2,
	.parts	= ek_norflash_partition_1,
};

static struct physmap_flash_data ek_norflash_data_2 = {
	.width		= 2,	
	.nr_parts	= 1,
	.parts	= ek_norflash_partition_2,
};

static struct resource ek_norflash_resource_1 = {
	.start		= EK_FLASH_BASE_1,
	.end		= EK_FLASH_BASE_1 + EK_FLASH_SIZE_1 - 1,
	.flags		= IORESOURCE_MEM,
};

static struct resource ek_norflash_resource_2 = {
	.start		= EK_FLASH_BASE_2,
	.end		= EK_FLASH_BASE_2 + EK_FLASH_SIZE_2 - 1,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device ek_flash_1 = {
	.name		= "physmap-flash1",
	.id		= 0,
	.dev		= {
				.platform_data	= &ek_norflash_data_1,
			},
	.resource	= &ek_norflash_resource_1,
	.num_resources	= 1,
};

static struct platform_device ek_flash_2 = {
	.name		= "physmap-flash2",
	.id		= 0,
	.dev		= {
				.platform_data	= &ek_norflash_data_2,
			},
	.resource	= &ek_norflash_resource_2,
	.num_resources	= 1,
};
//end add


/*
 * MCI (SD/MMC)
 */
static struct at91_mmc_data __initdata ek_mmc_data = {
	.slot_b		= 0,				//roy modify
	.wire4		= 1,
	.det_pin	= AT91_PIN_PA4,		//roy modify
	.wp_pin		= AT91_PIN_PA5,		// roy modify
//	.vcc_pin	= ... not connected
};

static void __init ek_board_init(void)
{
	/* Serial */
	at91_add_device_serial();
	/* USB Host */
	at91_add_device_usbh(&ek_usbh_data);
	/* USB Device */
	at91_add_device_udc(&ek_udc_data);
	/* SPI */
	at91_add_device_spi(ek_spi_devices, ARRAY_SIZE(ek_spi_devices));
	/* NAND */
	at91_add_device_nand(&ek_nand_data);
	/* roy add */
	/* NOR Flash */
	platform_device_register(&ek_flash_1);
	platform_device_register(&ek_flash_2);
	/* add end*/
	/* Ethernet */
	at91_add_device_eth(&ek_macb_data);
	/* MMC */
	at91_add_device_mmc(0, &ek_mmc_data);
	/* I2C */
	at91_add_device_i2c();
}

MACHINE_START(AT91SAM9260EK, "XJ Atmel AT91SAM9260")
	/* Maintainer: Atmel */
	.phys_io	= AT91_BASE_SYS,
	.io_pg_offst	= (AT91_VA_BASE_SYS >> 18) & 0xfffc,
	.boot_params	= AT91_SDRAM_BASE + 0x100,
	.timer		= &at91sam926x_timer,
	.map_io		= ek_map_io,
	.init_irq	= ek_init_irq,
	.init_machine	= ek_board_init,
MACHINE_END
