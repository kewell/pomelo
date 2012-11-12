/********************************************************************************
 *      Copyright:  (C) 2012 KEWELL 
 *
 *       Filename:  plat_ioctl.h
 *    Description:  This head file 
 *
 *        Version:  1.0.0(09/26/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "09/26/2012 09:28:04 AM"
 *                  2,
 *                 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *                 
 ********************************************************************************/

#ifndef __PLAT_IOCTL_H
#define __PLAT_IOCTL_H

#include <asm/ioctl.h>

#define PLATDRV_MAGIC           0x13

/*===========================================================================
 *                 ioctl command for all the drivers 0x01~0x0F
 *===========================================================================*/

/*args is enable or disable*/
#define SET_DRV_DEBUG               _IOW (PLATDRV_MAGIC, 0x01, unsigned int)
#define GET_DRV_VER                 _IOR (PLATDRV_MAGIC, 0x01, unsigned int)

/*===========================================================================
 *                 ioctl command for few ioctl() cmd driver 0x10~0x2F
 *===========================================================================*/
/* BEEP driver */
#define BEEP_DISALARM               _IO  (PLATDRV_MAGIC, 0x10)
#define BEEP_ENALARM                _IO  (PLATDRV_MAGIC, 0x11)
#define SET_DEFAULT_BEEP_FREQ       _IOW (PLATDRV_MAGIC, 0x10, unsigned int)

/* LED driver */
#define LED_OFF                     _IO   (PLATDRV_MAGIC, 0x20)
#define LED_ON                      _IO   (PLATDRV_MAGIC, 0x21)
#define LED_BLINK                   _IOW  (PLATDRV_MAGIC, 0x20, unsigned int)
#define ALL_LED_OFF                 _IO   (PLATDRV_MAGIC, 0x22)
#define ALL_LED_ON                  _IO   (PLATDRV_MAGIC, 0x23)
#define ALL_LED_BLINK               _IOW  (PLATDRV_MAGIC, 0x21, unsigned int)
/*===========================================================================
 *                   ioctl command for GPRS driver 0x30~0x4F
 *===========================================================================*/
#define GPRS_POWERDOWN              _IO  (PLATDRV_MAGIC, 0x30)
#define GPRS_POWERON                _IOW (PLATDRV_MAGIC, 0x30, unsigned int)
#define GPRS_RESET                  _IOW (PLATDRV_MAGIC, 0x31, unsigned int)
#define GPRS_POWERMON               _IO  (PLATDRV_MAGIC, 0x31)
/*Get Which SIM slot work, ioctl(fd, GET_SIM_SLOT, 0)*/
#define CHK_WORK_SIMSLOT            _IO  (PLATDRV_MAGIC, 0x32)
/*Set Which SIM slot work now*/
#define SET_WORK_SIMSLOT            _IOW (PLATDRV_MAGIC, 0x32, unsigned int)
/*Check the specify SIM door status*/
#define GPRS_CHK_SIMDOOR            _IOW (PLATDRV_MAGIC, 0x33, unsigned int)
#define GPRS_SET_DTR                _IOW (PLATDRV_MAGIC, 0x34, unsigned int)
#define GPRS_SET_RTS                _IOW (PLATDRV_MAGIC, 0x35, unsigned int)
#define GPRS_GET_RING               _IOR (PLATDRV_MAGIC, 0x30, unsigned int)
#define SET_PWUP_TIME               _IOW (PLATDRV_MAGIC, 0x36, unsigned int)
#define SET_PWDOWN_TIME             _IOW (PLATDRV_MAGIC, 0x37, unsigned int)
#define SET_RESET_TIME              _IOW (PLATDRV_MAGIC, 0x38, unsigned int)
#define SET_GPRS_USB_WORK           _IOW (PLATDRV_MAGIC, 0x39, unsigned int)

#endif                          /* __PLAT_IOCTL_H */
