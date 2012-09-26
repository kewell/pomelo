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

#define PLATDRV_MAGIC           0x05

/*===========================================================================
 *                 ioctl command for all the drivers 0x01~0x0F
 *===========================================================================*/

/*args is enable or disable*/
#define SET_DRV_DEBUG               _IOW (PLATDRV_MAGIC, 0x01, unsigned int)
#define GET_DRV_VER                 _IOR (PLATDRV_MAGIC, 0x02, unsigned int)

/*===========================================================================
 *                 ioctl command for few ioctl() cmd driver 0x10~0x2F
 *===========================================================================*/
/* DETECT driver  */
#define DETECT_STATUS               _IO   (PLATDRV_MAGIC, 0x12)
#define DETECT_WIFI_ID              _IO   (PLATDRV_MAGIC, 0x13)

/* Restore key */
#define RESTR_KEY_STATUS            _IOR  (PLATDRV_MAGIC, 0x14, unsigned int)
#define QUERY_PRESS_TIME            _IOR  (PLATDRV_MAGIC, 0x15, unsigned int)

/* BEEP driver */
#define BEEP_DISALARM               _IO  (PLATDRV_MAGIC, 0x16)
#define BEEP_ENALARM                _IO  (PLATDRV_MAGIC, 0x17)
#define SET_DEFAULT_BEEP_FREQ       _IOW (PLATDRV_MAGIC, 0x18, unsigned int)

/* LED driver */
#define LED_OFF                     _IO   (PLATDRV_MAGIC, 0x18)
#define LED_ON                      _IO   (PLATDRV_MAGIC, 0x19)
#define LED_BLINK                   _IOW  (PLATDRV_MAGIC, 0x1A, unsigned int)
#define ALL_LED_OFF                 _IO   (PLATDRV_MAGIC, 0x1B)
#define ALL_LED_ON                  _IO   (PLATDRV_MAGIC, 0x1C)
#define ALL_LED_BLINK               _IOW  (PLATDRV_MAGIC, 0x1D, unsigned int)
/*===========================================================================
 *                   ioctl command for GPRS driver 0x30~0x4F
 *===========================================================================*/
#define GPRS_POWERDOWN              _IO  (PLATDRV_MAGIC, 0x30)
#define GPRS_POWERON                _IOW (PLATDRV_MAGIC, 0x31, unsigned int)
#define GPRS_RESET                  _IOW (PLATDRV_MAGIC, 0x32, unsigned int)
#define GPRS_POWERMON               _IO  (PLATDRV_MAGIC, 0x33)
/*Get Which SIM slot work, ioctl(fd, GET_SIM_SLOT, 0)*/
#define CHK_WORK_SIMSLOT            _IO  (PLATDRV_MAGIC, 0x34)
/*Set Which SIM slot work now*/
#define SET_WORK_SIMSLOT            _IOW (PLATDRV_MAGIC, 0x35, unsigned int)
/*Check the specify SIM door status*/
#define GPRS_CHK_SIMDOOR            _IOW (PLATDRV_MAGIC, 0x36, unsigned int)
#define GPRS_SET_DTR                _IOW (PLATDRV_MAGIC, 0x37, unsigned int)
#define GPRS_SET_RTS                _IOW (PLATDRV_MAGIC, 0x38, unsigned int)
#define GPRS_GET_RING               _IOR (PLATDRV_MAGIC, 0x39, unsigned int)
#define SET_PWUP_TIME               _IOW (PLATDRV_MAGIC, 0x3A, unsigned int)
#define SET_PWDOWN_TIME             _IOW (PLATDRV_MAGIC, 0x3B, unsigned int)
#define SET_RESET_TIME              _IOW (PLATDRV_MAGIC, 0x3C, unsigned int)
#define SET_GPRS_USB_WORK           _IOW (PLATDRV_MAGIC, 0x3D, unsigned int)

#endif                          /* __PLAT_IOCTL_H */

