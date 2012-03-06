/*
*********************************************************************************************************
*                                             uC/GUI V3.98
*                        Universal graphic software for embedded applications
*
*                       (c) Copyright 2002, Micrium Inc., Weston, FL
*                       (c) Copyright 2002, SEGGER Microcontroller Systeme GmbH
*
*              礐/GUI is protected by international copyright laws. Knowledge of the
*              source code may not be used to write a similar product. This file may
*              only be used in accordance with a license and should not be redistributed
*              in any way. We appreciate your understanding and fairness.
*
----------------------------------------------------------------------
File        : LCDConf_1375_C8_C320x240.h
Purpose     : Sample configuration file
----------------------------------------------------------------------
*/

#ifndef LCDCONF_H
#define LCDCONF_H

/*********************************************************************
*
*                   General configuration of LCD
*
**********************************************************************
*/
#define	PIN_BACKLIGHT	PIN_PA2		//IO口编号
#define LCD_XSIZE      (160)   /* X-resolution of LCD, Logical coor. */
#define LCD_YSIZE      (160)   /* Y-resolution of LCD, Logical coor. */

#define LCD_BITSPERPIXEL (4)
#define LCD_CONTROLLER (1698)
#define LCD_FIXEDPALETTE  (4) /* 选择色彩模式 */

#define LCD_SWAP_RB         (0)

#define LCD_BUS_16
#define LCD_MAX_LOG_COLORS  (2)

#define LCD_ADDR_BASE    		(0x60000000)
#define LCD_ADDR_CMD_OFFSET		0
#define LCD_ADDR_DATA_OFFSET	1



#endif /* LCDCONF_H */

	 	 			 		    	 				 	  			   	 	 	 	 	 	  	  	      	   		 	 	 		  		  	 		 	  	  			     			       	   	 			  		    	 	     	 				  	 					 	 			   	  	  			 				 		 	 	 			     			 
