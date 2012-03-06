/*
*********************************************************************************************************
*                                             uC/GUI V3.98
*                        Universal graphic software for embedded applications
*
*                       (c) Copyright 2002, Micrium Inc., Weston, FL
*                       (c) Copyright 2002, SEGGER Microcontroller Systeme GmbH
*
*              µC/GUI is protected by international copyright laws. Knowledge of the
*              source code may not be used to write a similar product. This file may
*              only be used in accordance with a license and should not be redistributed
*              in any way. We appreciate your understanding and fairness.
*
----------------------------------------------------------------------
File        : GUIConf.h
Purpose     : Configures abilities, fonts etc.
----------------------------------------------------------------------
*/


#ifndef GUICONF_H
#define GUICONF_H

#include "LCDConf.h"
#define GUI_SUPPORT_MEMDEV        (0)  /* Memory devices available */
#define GUI_OS                    (1)  /* Compile with multitasking support */
#define GUI_SUPPORT_TOUCH         (0)  /* Support a touch screen (req. win-manager) */
#define GUI_SUPPORT_MOUSE         (0)  /* Support a mouse */
#define GUI_SUPPORT_UNICODE       (0)  /* Support mixed ASCII/UNICODE strings */
#define GUI_MAXTASK               (6)
#define GUI_DEFAULT_FONT          &GUI_FontHZ_Song_16
#define GUI_DEFAULT_BKCOLOR		  GUI_WHITE
#define GUI_DEFAULT_COLOR 		  GUI_BLACK


#endif  /* Avoid multiple inclusion */


	 	 			 		    	 				 	  			   	 	 	 	 	 	  	  	      	   		 	 	 		  		  	 		 	  	  			     			       	   	 			  		    	 	     	 				  	 					 	 			   	  	  			 				 		 	 	 			     			 
