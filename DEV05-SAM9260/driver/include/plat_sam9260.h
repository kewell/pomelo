/********************************************************************************
 *      Copyright:  (C) 2012 KEWELL 
 *
 *       Filename:  plat_sam9260.h
 *    Description:  This head file 
 *
 *        Version:  1.0.0(09/26/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "09/26/2012 09:25:43 AM"
 *                  2,
 *                 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *                 
 ********************************************************************************/

#ifndef __PLAT_SAM9260_H
#define __PLAT_SAM9260_H

#include "plat_ioctl.h"

/*Pin definition For GPRS/3G Driver*/
#define GPRS_VBUS_CTRL_PIN          AT91_PIN_PC9
#define GPRS_POWER_MON_PIN          AT91_PIN_PB13
#define GPRS_ON_PIN                 AT91_PIN_PA10
#define GPRS_RESET_PIN              AT91_PIN_PA11
#define GPRS_RTS_PIN                AT91_PIN_PB28
#define GPRS_DTR_PIN                AT91_PIN_PA27
#define GPRS_DCD_PIN                AT91_PIN_PA28
#define GPRS_CTS_PIN                AT91_PIN_PB29
#define GPRS_DSR_PIN                AT91_PIN_PA25
#if (defined HWVER_OLD)
#define GPRS_RI_PIN                 AT91_PIN_PA26
#elif (defined HWVER_NEW)
#define GPRS_RI_PIN                 AT91_PIN_PB30
#endif
#define GPRS_TXD_PIN                AT91_PIN_PB6
#define GPRS_RXD_PIN                AT91_PIN_PB7
#define GPRS_SELECT_SIM_PIN         AT91_PIN_PC1
#define GPRS_CHK_SIM1_PIN           AT91_PIN_PC6
#define GPRS_CHK_SIM2_PIN           AT91_PIN_PC7

/*Pin definition For LED Driver*/
#define LED_COUNT                   8
#if (defined HWVER_OLD)
#define LED_D1_RUN                  AT91_PIN_PB30
#elif (defined HWVER_NEW)
#define LED_D1_RUN                  AT91_PIN_PA26
#endif
#define LED_D2_0                    AT91_PIN_PB0    // Rs232/485 1
#define LED_D3_1                    AT91_PIN_PB1    // RS232/485 2
#define LED_D4_2                    AT91_PIN_PB2    // Rs232/485 3
#define LED_D5_3                    AT91_PIN_PB21   // PSTN
#define LED_D6_4                    AT91_PIN_PB31   // WIFI
#define LED_D7_5                    AT91_PIN_PA22   // SIM 1
#define LED_D8_6                    AT91_PIN_PC3    // SIM 2

/*Pin definition For Beep Driver*/
#define BEEP_PIN                    AT91_PIN_PC2

/* Pin definition for restore key driver */
#define RESTR_KEY                   AT91_PIN_PB22
#define RESTR_KEY_IRQ               50

#endif                          /*End __PLAT_SAM9260_H */
