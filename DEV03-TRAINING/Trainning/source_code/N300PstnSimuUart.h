
#define MODULE_VER	"1.0.0" /*careful: strlen of MODULE_VER must < 10 */

#define MAJOR_GU_TTYS5		207
#define GU_TTYS5_DEVNAME   	"GU_TTYS5"
#define GU_TTYS5_NODENAME 	"ttyS5"
#define GU_TTYS5_TXD		AT91_PIN_PA4 
#define GU_TTYS5_RXD		AT91_PIN_PA2
#define GU_TTYS5_CTS		AT91_PIN_PC11
#define GU_TTYS5_LINK		AT91_PIN_PB10
#define GU_TTYS5_DATA		AT91_PIN_PB2
//#define GU_TTYS5_TXD		AT91_PIN_PA31    //for debug
//#define GU_TTYS5_RXD		AT91_PIN_PA30	 // for debug

#define MAJOR_GU_TTYS6		208
#define GU_TTYS6_DEVNAME   	"GU_TTYS6"
#define GU_TTYS6_NODENAME 	"ttyS6"
#define GU_TTYS6_TXD		AT91_PIN_PA21 
#define GU_TTYS6_RXD		AT91_PIN_PA3
#define GU_TTYS6_CTS		AT91_PIN_PC13
#define GU_TTYS6_LINK		AT91_PIN_PB9
#define GU_TTYS6_DATA		AT91_PIN_PB3
//#define GU_TTYS6_TXD		AT91_PIN_PA31    //for debug
//#define GU_TTYS6_RXD		AT91_PIN_PA30	 // for debug

#define MAJOR_GU_TTYS7		209
#define GU_TTYS7_DEVNAME   	"GU_TTYS7"
#define GU_TTYS7_NODENAME 	"ttyS7"
#define GU_TTYS7_TXD		AT91_PIN_PB27 
#define GU_TTYS7_RXD		AT91_PIN_PB29
#define GU_TTYS7_CTS		AT91_PIN_PC15
#define GU_TTYS7_LINK		AT91_PIN_PD13
#define GU_TTYS7_DATA		AT91_PIN_PB4
//#define GU_TTYS7_TXD		AT91_PIN_PA31    //for debug
//#define GU_TTYS7_RXD		AT91_PIN_PA30	 // for debug

#define BUF_MAXLEN 4096

struct gu_buf_t
{
	unsigned int    	len;	/* number of chars */
	unsigned char  		bufp[BUF_MAXLEN];  	
	int  			idx;

	spinlock_t 	  	lock;	
	unsigned long	  	flag;

	wait_queue_head_t wq_head;
};

#define INPUT 	   1
#define OUTPUT 	   0
#define HIGHLEVEL 1
#define LOWLEVEL  0
#define DISPULLUP 0
#define PULLUP    1

/*******************************************************************************
*                           BAUD RATE CONTROL                                  *
*******************************************************************************/
struct baud_setting_t
{
	unsigned short	baud_rate;
	int		baud_value;
};

#define BAUD_2400	0x516
#define BAUD_4800	0x28B
#define BAUD_9600	0x145
#define BAUD_19200	0xA2
#define BAUD_38400	0x51
#define BAUD_57600	0x36
#define BAUD_76800	0x28
#define BAUD_115200 	0x1B

static struct baud_setting_t   g_bs_all[] = 
{
	{ BAUD_2400,  	2400   },	
	{ BAUD_4800,  	4800   },	
	{ BAUD_9600,  	9600   },	
	{ BAUD_19200, 	19200  },	
	{ BAUD_38400, 	38400  },	
	{ BAUD_57600, 	57600  },	
	{ BAUD_76800, 	76800  },	
	{ BAUD_115200,	115200 },
};

static inline int get_bd_value( unsigned char bd_rate ) 	
{
	int i=0;
	for( ; i<sizeof(g_bs_all)/sizeof( g_bs_all[0] ) ; i++ )
	{
		if( bd_rate == g_bs_all[i].baud_rate )
			return g_bs_all[i].baud_value;	
	}
	return -1;   /* wrong baud_rate */
}
		
/*******************************************************************************
*   	  SOFTWARE API DEFINITION  FOR System Peripherals       	       *
*******************************************************************************/		
 
 /* Hardware register definition */
typedef volatile unsigned int AT91_REG;

typedef struct _AT91S_SYS {
	AT91_REG	 AIC_SMR[32]; 	// Source Mode Register
	AT91_REG	 AIC_SVR[32]; 	// Source Vector Register
	AT91_REG	 AIC_IVR; 	// IRQ Vector Register
	AT91_REG	 AIC_FVR; 	// FIQ Vector Register
	AT91_REG	 AIC_ISR; 	// Interrupt Status Register
	AT91_REG	 AIC_IPR; 	// Interrupt Pending Register
	AT91_REG	 AIC_IMR; 	// Interrupt Mask Register
	AT91_REG	 AIC_CISR; 	// Core Interrupt Status Register
	AT91_REG	 Reserved0[2]; 	//
	AT91_REG	 AIC_IECR; 	// Interrupt Enable Command Register
	AT91_REG	 AIC_IDCR; 	// Interrupt Disable Command Register
	AT91_REG	 AIC_ICCR; 	// Interrupt Clear Command Register
	AT91_REG	 AIC_ISCR; 	// Interrupt Set Command Register
	AT91_REG	 AIC_EOICR; 	// End of Interrupt Command Register
	AT91_REG	 AIC_SPU; 	// Spurious Vector Register
	AT91_REG	 AIC_DCR; 	// Debug Control Register (Protect)
	AT91_REG	 Reserved1[1]; 	//
	AT91_REG	 AIC_FFER; 	// Fast Forcing Enable Register
	AT91_REG	 AIC_FFDR; 	// Fast Forcing Disable Register
	AT91_REG	 AIC_FFSR; 	// Fast Forcing Status Register
	AT91_REG	 Reserved2[45]; 	//
	AT91_REG	 DBGU_CR; 	// Control Register
	AT91_REG	 DBGU_MR; 	// Mode Register
	AT91_REG	 DBGU_IER; 	// Interrupt Enable Register
	AT91_REG	 DBGU_IDR; 	// Interrupt Disable Register
	AT91_REG	 DBGU_IMR; 	// Interrupt Mask Register
	AT91_REG	 DBGU_CSR; 	// Channel Status Register
	AT91_REG	 DBGU_RHR; 	// Receiver Holding Register
	AT91_REG	 DBGU_THR; 	// Transmitter Holding Register
	AT91_REG	 DBGU_BRGR; 	// Baud Rate Generator Register
	AT91_REG	 Reserved3[7]; 	//
	AT91_REG	 DBGU_C1R; 	// Chip ID1 Register
	AT91_REG	 DBGU_C2R; 	// Chip ID2 Register
	AT91_REG	 DBGU_FNTR; 	// Force NTRST Register
	AT91_REG	 Reserved4[45]; 	//
	AT91_REG	 DBGU_RPR; 	// Receive Pointer Register
	AT91_REG	 DBGU_RCR; 	// Receive Counter Register
	AT91_REG	 DBGU_TPR; 	// Transmit Pointer Register
	AT91_REG	 DBGU_TCR; 	// Transmit Counter Register
	AT91_REG	 DBGU_RNPR; 	// Receive Next Pointer Register
	AT91_REG	 DBGU_RNCR; 	// Receive Next Counter Register
	AT91_REG	 DBGU_TNPR; 	// Transmit Next Pointer Register
	AT91_REG	 DBGU_TNCR; 	// Transmit Next Counter Register
	AT91_REG	 DBGU_PTCR; 	// PDC Transfer Control Register
	AT91_REG	 DBGU_PTSR; 	// PDC Transfer Status Register
	AT91_REG	 Reserved5[54]; 	//
	AT91_REG	 PIOA_PER; 	// PIO Enable Register
	AT91_REG	 PIOA_PDR; 	// PIO Disable Register
	AT91_REG	 PIOA_PSR; 	// PIO Status Register
	AT91_REG	 Reserved6[1]; 	//
	AT91_REG	 PIOA_OER; 	// Output Enable Register
	AT91_REG	 PIOA_ODR; 	// Output Disable Registerr
	AT91_REG	 PIOA_OSR; 	// Output Status Register
	AT91_REG	 Reserved7[1]; 	//
	AT91_REG	 PIOA_IFER; 	// Input Filter Enable Register
	AT91_REG	 PIOA_IFDR; 	// Input Filter Disable Register
	AT91_REG	 PIOA_IFSR; 	// Input Filter Status Register
	AT91_REG	 Reserved8[1]; 	//
	AT91_REG	 PIOA_SODR; 	// Set Output Data Register
	AT91_REG	 PIOA_CODR; 	// Clear Output Data Register
	AT91_REG	 PIOA_ODSR; 	// Output Data Status Register
	AT91_REG	 PIOA_PDSR; 	// Pin Data Status Register
	AT91_REG	 PIOA_IER; 	// Interrupt Enable Register
	AT91_REG	 PIOA_IDR; 	// Interrupt Disable Register
	AT91_REG	 PIOA_IMR; 	// Interrupt Mask Register
	AT91_REG	 PIOA_ISR; 	// Interrupt Status Register
	AT91_REG	 PIOA_MDER; 	// Multi-driver Enable Register
	AT91_REG	 PIOA_MDDR; 	// Multi-driver Disable Register
	AT91_REG	 PIOA_MDSR; 	// Multi-driver Status Register
	AT91_REG	 Reserved9[1]; 	//
	AT91_REG	 PIOA_PPUDR; 	// Pull-up Disable Register
	AT91_REG	 PIOA_PPUER; 	// Pull-up Enable Register
	AT91_REG	 PIOA_PPUSR; 	// Pad Pull-up Status Register
	AT91_REG	 Reserved10[1]; 	//
	AT91_REG	 PIOA_ASR; 	// Select A Register
	AT91_REG	 PIOA_BSR; 	// Select B Register
	AT91_REG	 PIOA_ABSR; 	// AB Select Status Register
	AT91_REG	 Reserved11[9]; 	//
	AT91_REG	 PIOA_OWER; 	// Output Write Enable Register
	AT91_REG	 PIOA_OWDR; 	// Output Write Disable Register
	AT91_REG	 PIOA_OWSR; 	// Output Write Status Register
	AT91_REG	 Reserved12[85]; 	//
	AT91_REG	 PIOB_PER; 	// PIO Enable Register
	AT91_REG	 PIOB_PDR; 	// PIO Disable Register
	AT91_REG	 PIOB_PSR; 	// PIO Status Register
	AT91_REG	 Reserved13[1]; 	//
	AT91_REG	 PIOB_OER; 	// Output Enable Register
	AT91_REG	 PIOB_ODR; 	// Output Disable Registerr
	AT91_REG	 PIOB_OSR; 	// Output Status Register
	AT91_REG	 Reserved14[1]; 	//
	AT91_REG	 PIOB_IFER; 	// Input Filter Enable Register
	AT91_REG	 PIOB_IFDR; 	// Input Filter Disable Register
	AT91_REG	 PIOB_IFSR; 	// Input Filter Status Register
	AT91_REG	 Reserved15[1]; 	//
	AT91_REG	 PIOB_SODR; 	// Set Output Data Register
	AT91_REG	 PIOB_CODR; 	// Clear Output Data Register
	AT91_REG	 PIOB_ODSR; 	// Output Data Status Register
	AT91_REG	 PIOB_PDSR; 	// Pin Data Status Register
	AT91_REG	 PIOB_IER; 	// Interrupt Enable Register
	AT91_REG	 PIOB_IDR; 	// Interrupt Disable Register
	AT91_REG	 PIOB_IMR; 	// Interrupt Mask Register
	AT91_REG	 PIOB_ISR; 	// Interrupt Status Register
	AT91_REG	 PIOB_MDER; 	// Multi-driver Enable Register
	AT91_REG	 PIOB_MDDR; 	// Multi-driver Disable Register
	AT91_REG	 PIOB_MDSR; 	// Multi-driver Status Register
	AT91_REG	 Reserved16[1]; 	//
	AT91_REG	 PIOB_PPUDR; 	// Pull-up Disable Register
	AT91_REG	 PIOB_PPUER; 	// Pull-up Enable Register
	AT91_REG	 PIOB_PPUSR; 	// Pad Pull-up Status Register
	AT91_REG	 Reserved17[1]; 	//
	AT91_REG	 PIOB_ASR; 	// Select A Register
	AT91_REG	 PIOB_BSR; 	// Select B Register
	AT91_REG	 PIOB_ABSR; 	// AB Select Status Register
	AT91_REG	 Reserved18[9]; 	//
	AT91_REG	 PIOB_OWER; 	// Output Write Enable Register
	AT91_REG	 PIOB_OWDR; 	// Output Write Disable Register
	AT91_REG	 PIOB_OWSR; 	// Output Write Status Register
	AT91_REG	 Reserved19[85]; 	//
	AT91_REG	 PIOC_PER; 	// PIO Enable Register
	AT91_REG	 PIOC_PDR; 	// PIO Disable Register
	AT91_REG	 PIOC_PSR; 	// PIO Status Register
	AT91_REG	 Reserved20[1]; 	//
	AT91_REG	 PIOC_OER; 	// Output Enable Register
	AT91_REG	 PIOC_ODR; 	// Output Disable Registerr
	AT91_REG	 PIOC_OSR; 	// Output Status Register
	AT91_REG	 Reserved21[1]; 	//
	AT91_REG	 PIOC_IFER; 	// Input Filter Enable Register
	AT91_REG	 PIOC_IFDR; 	// Input Filter Disable Register
	AT91_REG	 PIOC_IFSR; 	// Input Filter Status Register
	AT91_REG	 Reserved22[1]; 	//
	AT91_REG	 PIOC_SODR; 	// Set Output Data Register
	AT91_REG	 PIOC_CODR; 	// Clear Output Data Register
	AT91_REG	 PIOC_ODSR; 	// Output Data Status Register
	AT91_REG	 PIOC_PDSR; 	// Pin Data Status Register
	AT91_REG	 PIOC_IER; 	// Interrupt Enable Register
	AT91_REG	 PIOC_IDR; 	// Interrupt Disable Register
	AT91_REG	 PIOC_IMR; 	// Interrupt Mask Register
	AT91_REG	 PIOC_ISR; 	// Interrupt Status Register
	AT91_REG	 PIOC_MDER; 	// Multi-driver Enable Register
	AT91_REG	 PIOC_MDDR; 	// Multi-driver Disable Register
	AT91_REG	 PIOC_MDSR; 	// Multi-driver Status Register
	AT91_REG	 Reserved23[1]; 	//
	AT91_REG	 PIOC_PPUDR; 	// Pull-up Disable Register
	AT91_REG	 PIOC_PPUER; 	// Pull-up Enable Register
	AT91_REG	 PIOC_PPUSR; 	// Pad Pull-up Status Register
	AT91_REG	 Reserved24[1]; 	//
	AT91_REG	 PIOC_ASR; 	// Select A Register
	AT91_REG	 PIOC_BSR; 	// Select B Register
	AT91_REG	 PIOC_ABSR; 	// AB Select Status Register
	AT91_REG	 Reserved25[9]; 	//
	AT91_REG	 PIOC_OWER; 	// Output Write Enable Register
	AT91_REG	 PIOC_OWDR; 	// Output Write Disable Register
	AT91_REG	 PIOC_OWSR; 	// Output Write Status Register
	AT91_REG	 Reserved26[85]; 	//
	AT91_REG	 PIOD_PER; 	// PIO Enable Register
	AT91_REG	 PIOD_PDR; 	// PIO Disable Register
	AT91_REG	 PIOD_PSR; 	// PIO Status Register
	AT91_REG	 Reserved27[1]; 	//
	AT91_REG	 PIOD_OER; 	// Output Enable Register
	AT91_REG	 PIOD_ODR; 	// Output Disable Registerr
	AT91_REG	 PIOD_OSR; 	// Output Status Register
	AT91_REG	 Reserved28[1]; 	//
	AT91_REG	 PIOD_IFER; 	// Input Filter Enable Register
	AT91_REG	 PIOD_IFDR; 	// Input Filter Disable Register
	AT91_REG	 PIOD_IFSR; 	// Input Filter Status Register
	AT91_REG	 Reserved29[1]; 	//
	AT91_REG	 PIOD_SODR; 	// Set Output Data Register
	AT91_REG	 PIOD_CODR; 	// Clear Output Data Register
	AT91_REG	 PIOD_ODSR; 	// Output Data Status Register
	AT91_REG	 PIOD_PDSR; 	// Pin Data Status Register
	AT91_REG	 PIOD_IER; 	// Interrupt Enable Register
	AT91_REG	 PIOD_IDR; 	// Interrupt Disable Register
	AT91_REG	 PIOD_IMR; 	// Interrupt Mask Register
	AT91_REG	 PIOD_ISR; 	// Interrupt Status Register
	AT91_REG	 PIOD_MDER; 	// Multi-driver Enable Register
	AT91_REG	 PIOD_MDDR; 	// Multi-driver Disable Register
	AT91_REG	 PIOD_MDSR; 	// Multi-driver Status Register
	AT91_REG	 Reserved30[1]; 	//
	AT91_REG	 PIOD_PPUDR; 	// Pull-up Disable Register
	AT91_REG	 PIOD_PPUER; 	// Pull-up Enable Register
	AT91_REG	 PIOD_PPUSR; 	// Pad Pull-up Status Register
	AT91_REG	 Reserved31[1]; 	//
	AT91_REG	 PIOD_ASR; 	// Select A Register
	AT91_REG	 PIOD_BSR; 	// Select B Register
	AT91_REG	 PIOD_ABSR; 	// AB Select Status Register
	AT91_REG	 Reserved32[9]; 	//
	AT91_REG	 PIOD_OWER; 	// Output Write Enable Register
	AT91_REG	 PIOD_OWDR; 	// Output Write Disable Register
	AT91_REG	 PIOD_OWSR; 	// Output Write Status Register
	AT91_REG	 Reserved33[85]; 	//
	AT91_REG	 PMC_SCER; 	// System Clock Enable Register
	AT91_REG	 PMC_SCDR; 	// System Clock Disable Register
	AT91_REG	 PMC_SCSR; 	// System Clock Status Register
	AT91_REG	 Reserved34[1]; 	//
	AT91_REG	 PMC_PCER; 	// Peripheral Clock Enable Register
	AT91_REG	 PMC_PCDR; 	// Peripheral Clock Disable Register
	AT91_REG	 PMC_PCSR; 	// Peripheral Clock Status Register
	AT91_REG	 Reserved35[1]; 	//
	AT91_REG	 CKGR_MOR; 	// Main Oscillator Register
	AT91_REG	 CKGR_MCFR; 	// Main Clock  Frequency Register
	AT91_REG	 CKGR_PLLAR; 	// PLL A Register
	AT91_REG	 CKGR_PLLBR; 	// PLL B Register
	AT91_REG	 PMC_MCKR; 	// Master Clock Register
	AT91_REG	 Reserved36[3]; 	//
	AT91_REG	 PMC_PCKR[8]; 	// Programmable Clock Register
	AT91_REG	 PMC_IER; 	// Interrupt Enable Register
	AT91_REG	 PMC_IDR; 	// Interrupt Disable Register
	AT91_REG	 PMC_SR; 	// Status Register
	AT91_REG	 PMC_IMR; 	// Interrupt Mask Register
	AT91_REG	 Reserved37[36]; 	//
	AT91_REG	 ST_CR; 	// Control Register
	AT91_REG	 ST_PIMR; 	// Period Interval Mode Register
	AT91_REG	 ST_WDMR; 	// Watchdog Mode Register
	AT91_REG	 ST_RTMR; 	// Real-time Mode Register
	AT91_REG	 ST_SR; 	// Status Register
	AT91_REG	 ST_IER; 	// Interrupt Enable Register
	AT91_REG	 ST_IDR; 	// Interrupt Disable Register
	AT91_REG	 ST_IMR; 	// Interrupt Mask Register
	AT91_REG	 ST_RTAR; 	// Real-time Alarm Register
	AT91_REG	 ST_CRTR; 	// Current Real-time Register
	AT91_REG	 Reserved38[54]; 	//
	AT91_REG	 RTC_CR; 	// Control Register
	AT91_REG	 RTC_MR; 	// Mode Register
	AT91_REG	 RTC_TIMR; 	// Time Register
	AT91_REG	 RTC_CALR; 	// Calendar Register
	AT91_REG	 RTC_TIMALR; 	// Time Alarm Register
	AT91_REG	 RTC_CALALR; 	// Calendar Alarm Register
	AT91_REG	 RTC_SR; 	// Status Register
	AT91_REG	 RTC_SCCR; 	// Status Clear Command Register
	AT91_REG	 RTC_IER; 	// Interrupt Enable Register
	AT91_REG	 RTC_IDR; 	// Interrupt Disable Register
	AT91_REG	 RTC_IMR; 	// Interrupt Mask Register
	AT91_REG	 RTC_VER; 	// Valid Entry Register
	AT91_REG	 Reserved39[52]; 	//
	AT91_REG	 MC_RCR; 	// MC Remap Control Register
	AT91_REG	 MC_ASR; 	// MC Abort Status Register
	AT91_REG	 MC_AASR; 	// MC Abort Address Status Register
	AT91_REG	 Reserved40[1]; 	//
	AT91_REG	 MC_PUIA[16]; 	// MC Protection Unit Area
	AT91_REG	 MC_PUP; 	// MC Protection Unit Peripherals
	AT91_REG	 MC_PUER; 	// MC Protection Unit Enable Register
	AT91_REG	 Reserved41[2]; 	//
	AT91_REG	 EBI_CSA; 	// Chip Select Assignment Register
	AT91_REG	 EBI_CFGR; 	// Configuration Register
	AT91_REG	 Reserved42[2]; 	//
	AT91_REG	 EBI_SMC2_CSR[8]; 	// SMC2 Chip Select Register
	AT91_REG	 EBI_SDRC_MR; 	// SDRAM Controller Mode Register
	AT91_REG	 EBI_SDRC_TR; 	// SDRAM Controller Refresh Timer Register
	AT91_REG	 EBI_SDRC_CR; 	// SDRAM Controller Configuration Register
	AT91_REG	 EBI_SDRC_SRR; 	// SDRAM Controller Self Refresh Register
	AT91_REG	 EBI_SDRC_LPR; 	// SDRAM Controller Low Power Register
	AT91_REG	 EBI_SDRC_IER; 	// SDRAM Controller Interrupt Enable Register
	AT91_REG	 EBI_SDRC_IDR; 	// SDRAM Controller Interrupt Disable Register
	AT91_REG	 EBI_SDRC_IMR; 	// SDRAM Controller Interrupt Mask Register
	AT91_REG	 EBI_SDRC_ISR; 	// SDRAM Controller Interrupt Mask Register
	AT91_REG	 Reserved43[3]; 	//
	AT91_REG	 EBI_BFC_MR; 	// BFC Mode Register
} AT91S_SYS, *AT91PS_SYS;

typedef struct _AT91S_TC {
	AT91_REG	 TC_CCR; 	// Channel Control Register
	AT91_REG	 TC_CMR; 	// Channel Mode Register
	AT91_REG	 Reserved0[2]; 	// 
	AT91_REG	 TC_CV; 	// Counter Value
	AT91_REG	 TC_RA; 	// Register A
	AT91_REG	 TC_RB; 	// Register B
	AT91_REG	 TC_RC; 	// Register C
	AT91_REG	 TC_SR; 	// Status Register
	AT91_REG	 TC_IER; 	// Interrupt Enable Register
	AT91_REG	 TC_IDR; 	// Interrupt Disable Register
	AT91_REG	 TC_IMR; 	// Interrupt Mask Register
} AT91S_TC, *AT91PS_TC;

typedef struct _AT91S_TCB {
	AT91S_TC	 TCB_TC0; 	// TC Channel 0
	AT91_REG	 Reserved0[4]; 	// 
	AT91S_TC	 TCB_TC1; 	// TC Channel 1
	AT91_REG	 Reserved1[4]; 	// 
	AT91S_TC	 TCB_TC2; 	// TC Channel 2
	AT91_REG	 Reserved2[4]; 	// 
	AT91_REG	 TCB_BCR; 	// TC Block Control Register
	AT91_REG	 TCB_BMR; 	// TC Block Mode Register
} AT91S_TCB, *AT91PS_TCB;

#define AT91C_BASE_TC5		(0xFFFA4080) // (TC5) Base Address
#define AT91C_BASE_TC4		(0xFFFA4040) // (TC4) Base Address
#define AT91C_BASE_TC3		(0xFFFA4000) // (TC3) Base Address
#define AT91C_BASE_TCB1		(0xFFFA4000) // (TCB1) Base Address
#define AT91C_BASE_TC2		(0xFFFA0080) // (TC2) Base Address
#define AT91C_BASE_TC1		(0xFFFA0040) // (TC1) Base Address
#define AT91C_BASE_TC0		(0xFFFA0000) // (TC0) Base Address
#define AT91C_BASE_TCB0		(0xFFFA0000) // (TCB0) Base Address

// *****************************************************************************
// //               PERIPHERAL ID DEFINITIONS FOR AT91RM9200
// // *****************************************************************************
#define AT91C_ID_FIQ    ( 0) // Advanced Interrupt Controller (FIQ)
#define AT91C_ID_SYS    ( 1) // System Peripheral
#define AT91C_ID_PIOA   ( 2) // Parallel IO Controller A
#define AT91C_ID_PIOB   ( 3) // Parallel IO Controller B
#define AT91C_ID_PIOC   ( 4) // Parallel IO Controller C
#define AT91C_ID_PIOD   ( 5) // Parallel IO Controller D
#define AT91C_ID_US0    ( 6) // USART 0
#define AT91C_ID_US1    ( 7) // USART 1
#define AT91C_ID_US2    ( 8) // USART 2
#define AT91C_ID_US3    ( 9) // USART 3
#define AT91C_ID_MCI    (10) // Multimedia Card Interface
#define AT91C_ID_UDP    (11) // USB Device Port
#define AT91C_ID_TWI    (12) // Two-Wire Interface
#define AT91C_ID_SPI    (13) // Serial Peripheral Interface
#define AT91C_ID_SSC0   (14) // Serial Synchronous Controller 0
#define AT91C_ID_SSC1   (15) // Serial Synchronous Controller 1
#define AT91C_ID_SSC2   (16) // Serial Synchronous Controller 2
#define AT91C_ID_TC0    (17) // Timer Counter 0
#define AT91C_ID_TC1    (18) // Timer Counter 1
#define AT91C_ID_TC2    (19) // Timer Counter 2
#define AT91C_ID_TC3    (20) // Timer Counter 3
#define AT91C_ID_TC4    (21) // Timer Counter 4
#define AT91C_ID_TC5    (22) // Timer Counter 5
#define AT91C_ID_UHP    (23) // USB Host port
#define AT91C_ID_EMAC   (24) // Ethernet MAC
#define AT91C_ID_IRQ0   (25) // Advanced Interrupt Controller (IRQ0)
#define AT91C_ID_IRQ1   (26) // Advanced Interrupt Controller (IRQ1)
#define AT91C_ID_IRQ2   (27) // Advanced Interrupt Controller (IRQ2)
#define AT91C_ID_IRQ3   (28) // Advanced Interrupt Controller (IRQ3)
#define AT91C_ID_IRQ4   (29) // Advanced Interrupt Controller (IRQ4)
#define AT91C_ID_IRQ5   (30) // Advanced Interrupt Controller (IRQ5)
#define AT91C_ID_IRQ6   (31) // Advanced Interrupt Controller (IRQ6)

// -------- AIC_SMR : (AIC Offset: 0x0) Control Register --------
#define AT91C_AIC_PRIOR       (0x7 <<  0) // (AIC) Priority Level
#define AT91C_AIC_PRIOR_LOWEST               (0x0) // (AIC) Lowest priority level
#define AT91C_AIC_PRIOR_HIGHEST              (0x7) // (AIC) Highest priority level
#define AT91C_AIC_SRCTYPE     (0x3 <<  5) // (AIC) Interrupt Source Type
#define AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE  (0x0 <<  5) // (AIC) Internal Sources Code Label Level Sensitive
#define AT91C_AIC_SRCTYPE_INT_EDGE_TRIGGERED   (0x1 <<  5) // (AIC) Internal Sources Code Label Edge triggered
#define AT91C_AIC_SRCTYPE_EXT_HIGH_LEVEL       (0x2 <<  5) // (AIC) External Sources Code Label High-level Sensitive
#define AT91C_AIC_SRCTYPE_EXT_POSITIVE_EDGE    (0x3 <<  5) // (AIC) External Sources Code Label Positive Edge triggered
 

#define AT91C_VA_BASE_TCB1	AT91_IO_P2V(AT91C_BASE_TCB1)
#define AT91C_VA_BASE_TCB0	AT91_IO_P2V(AT91C_BASE_TCB0)

#define AT91_SYS		((AT91PS_SYS) AT91_VA_BASE_SYS)

// -------- TC_CCR : (TC Offset: 0x0) TC Channel Control Register -------- 
#define AT91C_TC_CLKEN        ( 0x1 <<  0) // (TC) Counter Clock Enable Command
#define AT91C_TC_CLKDIS       ( 0x1 <<  1) // (TC) Counter Clock Disable Command
#define AT91C_TC_SWTRG        ( 0x1 <<  2) // (TC) Software Trigger Command
// -------- TC_CMR : (TC Offset: 0x4) TC Channel Mode Register: Capture Mode / Waveform Mode -------- 
#define AT91C_TC_TCCLKS       ( 0x7 <<  0) // (TC) Clock Selection
#define		AT91C_TC_TIMER_DIV1_CLOCK             ( 0x0 <<  0) // (TC) MCK/2
#define		AT91C_TC_TIMER_DIV2_CLOCK             ( 0x1 <<  0) // (TC) MCK/8
#define		AT91C_TC_TIMER_DIV3_CLOCK             ( 0x2 <<  0) // (TC) MCK/32
#define		AT91C_TC_TIMER_DIV4_CLOCK             ( 0x3 <<  0) // (TC) MCK/128
#define		AT91C_TC_TIMER_DIV5_CLOCK             ( 0x4 <<  0) // (TC) MCK/256 = SLOW CLOCK
#define		AT91C_TC_TIMER_XC0                    ( 0x5 <<  0) // (TC) XC0
#define		AT91C_TC_TIMER_XC1                    ( 0x6 <<  0) // (TC) XC1
#define		AT91C_TC_TIMER_XC2                    ( 0x7 <<  0) // (TC) XC2
#define	AT91C_TC_CLKI         ( 0x1 <<  3) // (TC) Clock Invert
#define AT91C_TC_BURST        ( 0x3 <<  4) // (TC) Burst Signal Selection
#define AT91C_TC_CPCSTOP      ( 0x1 <<  6) // (TC) Counter Clock Stopped with RC Compare
#define AT91C_TC_CPCDIS       ( 0x1 <<  7) // (TC) Counter Clock Disable with RC Compare
#define AT91C_TC_EEVTEDG      ( 0x3 <<  8) // (TC) External Event Edge Selection
#define 	AT91C_TC_EEVTEDG_NONE                 ( 0x0 <<  8) // (TC) Edge: None
#define 	AT91C_TC_EEVTEDG_RISING               ( 0x1 <<  8) // (TC) Edge: rising edge
#define 	AT91C_TC_EEVTEDG_FALLING              ( 0x2 <<  8) // (TC) Edge: falling edge
#define 	AT91C_TC_EEVTEDG_BOTH                 ( 0x3 <<  8) // (TC) Edge: each edge
#define AT91C_TC_EEVT         ( 0x3 << 10) // (TC) External Event  Selection
#define 	AT91C_TC_EEVT_NONE                 ( 0x0 << 10) // (TC) Signal selected as external event: TIOB TIOB direction: input
#define 	AT91C_TC_EEVT_RISING               ( 0x1 << 10) // (TC) Signal selected as external event: XC0 TIOB direction: output
#define 	AT91C_TC_EEVT_FALLING              ( 0x2 << 10) // (TC) Signal selected as external event: XC1 TIOB direction: output
#define 	AT91C_TC_EEVT_BOTH                 ( 0x3 << 10) // (TC) Signal selected as external event: XC2 TIOB direction: output
#define AT91C_TC_ENETRG       ( 0x1 << 12) // (TC) External Event Trigger enable
#define AT91C_TC_WAVESEL      ( 0x3 << 13) // (TC) Waveform  Selection
#define 	AT91C_TC_WAVESEL_UP                   ( 0x0 << 13) // (TC) UP mode without atomatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UP_AUTO              ( 0x2 << 13) // (TC) UP mode with automatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UPDOWN               ( 0x1 << 13) // (TC) UPDOWN mode without automatic trigger on RC Compare
#define 	AT91C_TC_WAVESEL_UPDOWN_AUTO          ( 0x3 << 13) // (TC) UPDOWN mode with automatic trigger on RC Compare
#define AT91C_TC_CPCTRG       ( 0x1 << 14) // (TC) RC Compare Trigger Enable
#define AT91C_TC_WAVE         ( 0x1 << 15) // (TC) 
#define AT91C_TC_ACPA         ( 0x3 << 16) // (TC) RA Compare Effect on TIOA
#define 	AT91C_TC_ACPA_NONE                 ( 0x0 << 16) // (TC) Effect: none
#define 	AT91C_TC_ACPA_SET                  ( 0x1 << 16) // (TC) Effect: set
#define 	AT91C_TC_ACPA_CLEAR                ( 0x2 << 16) // (TC) Effect: clear
#define 	AT91C_TC_ACPA_TOGGLE               ( 0x3 << 16) // (TC) Effect: toggle
#define AT91C_TC_ACPC         ( 0x3 << 18) // (TC) RC Compare Effect on TIOA
#define 	AT91C_TC_ACPC_NONE                 ( 0x0 << 18) // (TC) Effect: none
#define 	AT91C_TC_ACPC_SET                  ( 0x1 << 18) // (TC) Effect: set
#define 	AT91C_TC_ACPC_CLEAR                ( 0x2 << 18) // (TC) Effect: clear
#define 	AT91C_TC_ACPC_TOGGLE               ( 0x3 << 18) // (TC) Effect: toggle
#define AT91C_TC_AEEVT        ( 0x3 << 20) // (TC) External Event Effect on TIOA
#define 	AT91C_TC_AEEVT_NONE                 ( 0x0 << 20) // (TC) Effect: none
#define 	AT91C_TC_AEEVT_SET                  ( 0x1 << 20) // (TC) Effect: set
#define 	AT91C_TC_AEEVT_CLEAR                ( 0x2 << 20) // (TC) Effect: clear
#define 	AT91C_TC_AEEVT_TOGGLE               ( 0x3 << 20) // (TC) Effect: toggle
#define AT91C_TC_ASWTRG       ( 0x3 << 22) // (TC) Software Trigger Effect on TIOA
#define 	AT91C_TC_ASWTRG_NONE                 ( 0x0 << 22) // (TC) Effect: none
#define 	AT91C_TC_ASWTRG_SET                  ( 0x1 << 22) // (TC) Effect: set
#define 	AT91C_TC_ASWTRG_CLEAR                ( 0x2 << 22) // (TC) Effect: clear
#define 	AT91C_TC_ASWTRG_TOGGLE               ( 0x3 << 22) // (TC) Effect: toggle
#define AT91C_TC_BCPB         ( 0x3 << 24) // (TC) RB Compare Effect on TIOB
#define 	AT91C_TC_BCPB_NONE                 ( 0x0 << 24) // (TC) Effect: none
#define 	AT91C_TC_BCPB_SET                  ( 0x1 << 24) // (TC) Effect: set
#define 	AT91C_TC_BCPB_CLEAR                ( 0x2 << 24) // (TC) Effect: clear
#define 	AT91C_TC_BCPB_TOGGLE               ( 0x3 << 24) // (TC) Effect: toggle
#define AT91C_TC_BCPC         ( 0x3 << 26) // (TC) RC Compare Effect on TIOB
#define 	AT91C_TC_BCPC_NONE                 ( 0x0 << 26) // (TC) Effect: none
#define 	AT91C_TC_BCPC_SET                  ( 0x1 << 26) // (TC) Effect: set
#define 	AT91C_TC_BCPC_CLEAR                ( 0x2 << 26) // (TC) Effect: clear
#define 	AT91C_TC_BCPC_TOGGLE               ( 0x3 << 26) // (TC) Effect: toggle
#define AT91C_TC_BEEVT        ( 0x3 << 28) // (TC) External Event Effect on TIOB
#define 	AT91C_TC_BEEVT_NONE                 ( 0x0 << 28) // (TC) Effect: none
#define 	AT91C_TC_BEEVT_SET                  ( 0x1 << 28) // (TC) Effect: set
#define 	AT91C_TC_BEEVT_CLEAR                ( 0x2 << 28) // (TC) Effect: clear
#define 	AT91C_TC_BEEVT_TOGGLE               ( 0x3 << 28) // (TC) Effect: toggle
#define AT91C_TC_BSWTRG       ( 0x3 << 30) // (TC) Software Trigger Effect on TIOB
#define 	AT91C_TC_BSWTRG_NONE                 ( 0x0 << 30) // (TC) Effect: none
#define 	AT91C_TC_BSWTRG_SET                  ( 0x1 << 30) // (TC) Effect: set
#define 	AT91C_TC_BSWTRG_CLEAR                ( 0x2 << 30) // (TC) Effect: clear
#define 	AT91C_TC_BSWTRG_TOGGLE               ( 0x3 << 30) // (TC) Effect: toggle
// -------- TC_SR : (TC Offset: 0x20) TC Channel Status Register -------- 
#define AT91C_TC_COVFS        ( 0x1 <<  0) // (TC) Counter Overflow
#define AT91C_TC_LOVRS        ( 0x1 <<  1) // (TC) Load Overrun
#define AT91C_TC_CPAS         ( 0x1 <<  2) // (TC) RA Compare
#define AT91C_TC_CPBS         ( 0x1 <<  3) // (TC) RB Compare
#define AT91C_TC_CPCS         ( 0x1 <<  4) // (TC) RC Compare
#define AT91C_TC_LDRAS        ( 0x1 <<  5) // (TC) RA Loading
#define AT91C_TC_LDRBS        ( 0x1 <<  6) // (TC) RB Loading
#define AT91C_TC_ETRCS        ( 0x1 <<  7) // (TC) External Trigger
#define AT91C_TC_ETRGS        ( 0x1 << 16) // (TC) Clock Enabling
#define AT91C_TC_MTIOA        ( 0x1 << 17) // (TC) TIOA Mirror
#define AT91C_TC_MTIOB        ( 0x1 << 18) // (TC) TIOA Mirror
// // -------- TC_IER : (TC Offset: 0x24) TC Channel Interrupt Enable Register -------- 
// // -------- TC_IDR : (TC Offset: 0x28) TC Channel Interrupt Disable Register -------- 
// // -------- TC_IMR : (TC Offset: 0x2c) TC Channel Interrupt Mask Register -------- 
 

/*******************************************************************************
*   			AT91RM9200 GPIO  CONTROL                    	       *
*******************************************************************************/

/* data led ctrl */
#define ttys5_ctrl_led(x)	\
	(x)?(AT91_SYS->PIOB_SODR=(1<<2)):(AT91_SYS->PIOB_CODR=(1<<2))
	
#define ttys6_ctrl_led(x)	\
	(x)?(AT91_SYS->PIOB_SODR=(1<<3)):(AT91_SYS->PIOB_CODR=(1<<3))

#define ttys7_ctrl_led(x)	\
	(x)?(AT91_SYS->PIOB_SODR=(1<<4)):(AT91_SYS->PIOB_CODR=(1<<4))


/* cts status */
#define ttys5_get_cts()	\
	({AT91_SYS->PIOC_PDSR & (1<<11);})
	
#define ttys6_get_cts()	\
	({AT91_SYS->PIOC_PDSR & (1<<13);})

#define ttys7_get_cts()	\
	({AT91_SYS->PIOC_PDSR & (1<<15);})

/*ttys5*/					  
#define ttys5_put(x)	\
	(x)?(AT91_SYS->PIOA_SODR=(1<<4)):(AT91_SYS->PIOA_CODR=(1<<4))

#define ttys5_get()	\
	({AT91_SYS->PIOA_PDSR & (1<<2);})

/*ttys6*/					  
#define ttys6_put(x)	\
	(x)?(AT91_SYS->PIOA_SODR=(1<<21)):(AT91_SYS->PIOA_CODR=(1<<21))

#define ttys6_get()	\
	({AT91_SYS->PIOA_PDSR & (1<<3);})

/*ttys7*/					  
#define ttys7_put(x)	\
	(x)?(AT91_SYS->PIOB_SODR=(1<<27)):(AT91_SYS->PIOB_CODR=(1<<27))

#define ttys7_get()	\
	({AT91_SYS->PIOB_PDSR & (1<<29);})

/*******************************************************************************
*   			AT91RM9200 TIMER   CONTROL                    	       *
*******************************************************************************/

/*
blk : 0, 1
ch :  0, 1, 2
tc  :  0 - 6  blk=0:0,1,2 ; blk=1:3,4,5
 */
#define start_tc(blk,ch,tc,freq)					\
		AT91_SYS->PMC_PCER |= 1 << AT91C_ID_TC##tc;								\
		g_tcbase##blk->TCB_TC##ch.TC_CCR = AT91C_TC_CLKDIS;            /* clock disable */			\
		g_tcbase##blk->TCB_TC##ch.TC_IDR = 0xFFFFFFFF;                 /* all interrupts are disabled */	\
		g_tcbase##blk->TCB_TC##ch.TC_CMR = AT91C_TC_CPCTRG;       /* clock1,RC Compare triger */		\
		g_tcbase##blk->TCB_TC##ch.TC_RC  = 59904000/(2*(freq));      	 /*load delay required */		\
		g_tcbase##blk->TCB_TC##ch.TC_CCR = AT91C_TC_CLKEN;             /* clock enable */			\
		g_tcbase##blk->TCB_TC##ch.TC_CCR = AT91C_TC_SWTRG;             /*software trigger enable */		\
		g_tcbase##blk->TCB_TC##ch.TC_IER = AT91C_TC_CPCS              /* RC compare interrupt enable */	

#define restart_tc(blk,ch,tc,freq)					\
		g_tcbase##blk->TCB_TC##ch.TC_RC  = 59904000/(2*(freq));    	 /*load delay required */		\
		g_tcbase##blk->TCB_TC##ch.TC_CCR = AT91C_TC_CLKEN;             /* clock enable */			\
		g_tcbase##blk->TCB_TC##ch.TC_CCR = AT91C_TC_SWTRG;             /*software trigger enable */		\
		g_tcbase##blk->TCB_TC##ch.TC_IER = AT91C_TC_CPCS              /* RC compare interrupt enable */	

#define stop_tc(blk,ch,tc)					\
		g_tcbase##blk->TCB_TC##ch.TC_CCR = AT91C_TC_CLKDIS            /* clock disable */			\

/* tty:5,6,7*/
#define config_tcirq(tty)	\
	config_tcirq##tty

#define config_tcirq5	\
	AT91_SYS->AIC_ICCR = 1 << 20;								\
        AT91_SYS->AIC_SMR[20] = (AT91_SYS->AIC_SMR[20] & ~AT91C_AIC_SRCTYPE) | (0x1 << 5);  /*or (0x0 << 5)*/    \
        AT91_SYS->AIC_SMR[20] = (AT91_SYS->AIC_SMR[20] & ~AT91C_AIC_PRIOR) | 0x7

#define config_tcirq6	\
	AT91_SYS->AIC_ICCR = 1 << 21;								\
        AT91_SYS->AIC_SMR[21] = (AT91_SYS->AIC_SMR[21] & ~AT91C_AIC_SRCTYPE) | (0x1 << 5);  /*or (0x0 << 5)*/   \
        AT91_SYS->AIC_SMR[21] = (AT91_SYS->AIC_SMR[21] & ~AT91C_AIC_PRIOR) | 0x7

#define config_tcirq7	\
	AT91_SYS->AIC_ICCR = 1 << 22;								\
        AT91_SYS->AIC_SMR[22] = (AT91_SYS->AIC_SMR[22] & ~AT91C_AIC_SRCTYPE) | (0x1 << 5);  /*or (0x0 << 5)*/   \
        AT91_SYS->AIC_SMR[22] = (AT91_SYS->AIC_SMR[22] & ~AT91C_AIC_PRIOR) | 0x7

#define disable_tc(blk,ch)	 /* bkl : 0, 1 ; ch : 0, 1, 2 */		\
			g_tcbase##blk->TCB_TC##ch.TC_CCR = AT91C_TC_CLKDIS	
			/*udelay(5)*/					

#define clear_tc_sr(blk,ch)	 /*  bkl : 0, 1 ; ch : 0, 1, 2 */	\
		g_tcbase##blk->TCB_TC##ch.TC_SR	

/*******************************************************************************
*                           AT91RM9200 EXTERNAL IRQ                            *
*******************************************************************************/

enum recv_int_status_t{ first_into=0, can_recv=1, should_ignore=2 };

/* ttys5,6,7 configure rxd irq type */
#define ttys_cfg_rxdirq(tty,type)	\
	ttys##tty##_cfg_rxdirq(type)

#define ttys5_cfg_rxdirq(x)	\
	AT91_SYS->AIC_ICCR = 1 << 29;								\
        AT91_SYS->PMC_PCER = 1 << AT91C_ID_PIOA;   						\
        AT91_SYS->PIOA_IFER = 1 << 2;								\
        AT91_SYS->AIC_SMR[29] = (AT91_SYS->AIC_SMR[29] & ~AT91C_AIC_SRCTYPE) | ((x) << 5);	\
        AT91_SYS->AIC_SMR[29] = (AT91_SYS->AIC_SMR[29] & ~AT91C_AIC_PRIOR) | 0x5

#define ttys6_cfg_rxdirq(x)   \
	AT91_SYS->AIC_ICCR = 1 << 30;								\
        AT91_SYS->PMC_PCER = 1 << AT91C_ID_PIOA;                                                \
        AT91_SYS->PIOA_IFER = 1 << 3;                                                           \
        AT91_SYS->AIC_SMR[30] = (AT91_SYS->AIC_SMR[30] & ~AT91C_AIC_SRCTYPE) | ((x) << 5);	\
        AT91_SYS->AIC_SMR[30] = (AT91_SYS->AIC_SMR[30] & ~AT91C_AIC_PRIOR) | 0x5

#define ttys7_cfg_rxdirq(x)   \
	AT91_SYS->AIC_ICCR = 1 << 25;								\
        AT91_SYS->PMC_PCER = 1 << AT91C_ID_PIOB;                                                \
        AT91_SYS->PIOB_IFER = 1 << 29;                                                          \
        AT91_SYS->AIC_SMR[25] = (AT91_SYS->AIC_SMR[25] & ~AT91C_AIC_SRCTYPE) | ((x) << 5);	\
        AT91_SYS->AIC_SMR[25] = (AT91_SYS->AIC_SMR[25] & ~AT91C_AIC_PRIOR) | 0x5

/* ttys5,6,7 disable rxd irq type */
#define ttys5_dis_rxdirq	\
	AT91_SYS->AIC_IDCR = 1 << 29

#define ttys6_dis_rxdirq	\
	AT91_SYS->AIC_IDCR = 1 << 30

#define ttys7_dis_rxdirq	\
	AT91_SYS->AIC_IDCR = 1 << 25


/* ttys5,6,7 clear rxd irq type */
#define ttys5_clr_rxdirq	\
	AT91_SYS->AIC_ICCR = 1 << 29

#define ttys6_clr_rxdirq	\
	AT91_SYS->AIC_ICCR = 1 << 30

#define ttys7_clr_rxdirq	\
	AT91_SYS->AIC_ICCR = 1 << 25

/* ttys5,6,7 restart rxd irq type */
#define ttys5_restart_rxdirq	\
	AT91_SYS->AIC_IECR = 1 << 29 

#define ttys6_restart_rxdirq    \
	        AT91_SYS->AIC_IECR = 1 << 30 

#define ttys7_restart_rxdirq    \
	        AT91_SYS->AIC_IECR = 1 << 25 

/*******************************************************************************
*                                  GU SETTING                                  *
*******************************************************************************/

#define GU_GET_VER       3             
#define GU_CLEAR_BUF    18
#define GU_GET_LENGTH   19		/*GET DATA LENGTH */
#define GU_SET_UART	20	       /* uart set */

struct gu_setting_t
{
	unsigned short  baudrate;
	unsigned short  parity;
	unsigned short	databits;
	unsigned short	stopbits;
};

enum gu_status_t { 
		   head_bit1, head_bit2, head_bit3, head_bit4,
		   pre_bit1,  pre_bit2,  pre_bit3,  pre_bit4,  pre_bit5,  pre_bit6, pre_bit7, pre_bit8, 		
		   start_bit, data_bit1, data_bit2, data_bit3, data_bit4, 
		   data_bit5, data_bit6, data_bit7, data_bit8, stop_bit,  free_bit, free_bit2,
		   tail_bit1, tail_bit2, tail_bit3, tail_bit4 
		 };

#define	baud_set(bs,br,par,data,stop)	\
	(bs).baudrate = (br);		\
	(bs).parity   = (par);		\
	(bs).databits = (data);		\
	(bs).stopbits = (stop)			

/*******************************************************************************
*                                PSTN LED CONTROL                              *
*******************************************************************************/

/* 
 * n = 5,6,7
 * x = 0 : led off; x = 1 :lef on 
 */
#define gu_ttys_ctrl_led(n,x)		\
	at91_set_gpio_value(GU_TTYS##n##_DATA, x)

/*******************************************************************************
*                               DEBUG  FUNCTION                                *
*******************************************************************************/


#if 0
static void debug(char* fmt, ...)
{
	char	buf[1024];
	va_list va;

	buf[0] = 0;
	va_start( va, fmt );
	vsprintf( buf , fmt, va );
	printk(KERN_INFO "%s\t[%3d]:\t",__FUNCTION__, __LINE__);
	printk(KERN_INFO  buf );						
}
#endif

static inline void print_buf(struct gu_buf_t* bufp)
{
	int i=0;
	printk(KERN_INFO "[%s]:%d\t", __FUNCTION__, __LINE__);
	for(; i<bufp->len; i++ )
		printk(KERN_INFO "%c", bufp->bufp[i] );
	printk(KERN_INFO "\n");
}

/*******************************************************************************
*                                BUF FUNCTION                                  *
*******************************************************************************/

#define buf_init(buf_ptr)			\
	memset((buf_ptr)->bufp,0,BUF_MAXLEN );	\
	(buf_ptr)->len= 0;			\
	(buf_ptr)->idx=0;			\
	spin_lock_init(&(buf_ptr)->lock);	\
	init_waitqueue_head(&(buf_ptr)->wq_head)	

#define buf_clear(buf_ptr)					\
	spin_lock_irqsave(&(buf_ptr)->lock,(buf_ptr)->flag);	\
	(buf_ptr)->len = 0;			\
	(buf_ptr)->idx=0;			\
	memset((buf_ptr)->bufp,0,BUF_MAXLEN );	\
	spin_unlock_irqrestore(&(buf_ptr)->lock,(buf_ptr)->flag)

/*******************************************************************************
* function name: 	put_to_buf
* description:		add string from user space to buf kernel space
* input param:	
*				char* bufp:	
*output param:
*				struct gu_buf_t *bufp
*return value:		
*				>=0 : number of chars to be put into
*				<0   : failed
*caller:
*function called:
*				strlen
*				
********************************************************************************/

static inline int put_to_buf(struct gu_buf_t *buf, const unsigned char __user * user_bufp, size_t size)
{
	int len ;		 		/* bufp length */	
	char tmp[BUF_MAXLEN];
	
	if(!buf || !user_bufp)
		return -1;

	if( 0 == size)
		return 0;

	if( BUF_MAXLEN < size )
		len = BUF_MAXLEN;
	else
		len = size;
	if(copy_from_user( tmp, user_bufp, len ))
		return -1;

	spin_lock_irqsave( &buf->lock, buf->flag );
	memcpy( buf->bufp, tmp, len );
	buf->len = len;
	buf->idx = 0;
	spin_unlock_irqrestore( &buf->lock, buf->flag);
	
	return len;	
}

/******************************************************************************
*
*copy
*
******************************************************************************/
static inline int get_from_buf(struct gu_buf_t *buf, unsigned char __user * user_bufp,  size_t count )
{
	int len;
	/*if(!buf || !user_bufp)*/
	/*	return -1;	*/
	/*if( 0 == count)	*/
	/*	return 0;	*/
	
	len = buf->len;
	
	spin_lock_irqsave( &buf->lock, buf->flag );
	if(copy_to_user( user_bufp, buf->bufp, len ))
		return -1;

	buf->len = 0;
	buf->idx = 0;
	spin_unlock_irqrestore( &buf->lock, buf->flag );
	
	return len;	
}
