/*********************************************************************************
 *      Copyright:  (C) 2012 KEWELL
 *
 *       Filename:  gprs_lowlevel.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(09/26/2012~)
 *         Author:  WENJING <WENJIGN0101@GMAIL.COM>
 *      ChangeLog:  1, Release initial version on "09/26/2012 09:40:14 AM"
 *                  2,
 *                 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 ********************************************************************************/

#include "include/dev_gprs.h"

GSM_DEVICE support_gprs[] = {
    [GSM_GTM900B] = {
                     .name = "GTM900B",
                     .id = GSM_GTM900B,
                     /*10~100 ms specified in user manul. */
                     .poweron_period_time = 100,
                     /*Experience value on L200, datasheet is wrong */
                     .atcmd_active_time = 6000,
                     /*About 2~3 seconds specified in user manul, 
                      * or can send AT command "AT%MSO" to shutdown, but without logout network*/
                     .poweroff_period_time = 3000,
                     /*Experience value on L200, datasheet is wrong */
                     .atcmd_inactive_time = 10000,
                     .ring_call_time = 10,
                     .ring_sms_time = 10,
                     }
    ,
    [GSM_GTM900C] = {
                     .name = "GTM900C",
                     .id = GSM_GTM900C,
                     /*At least 50 ms specified in user manul. */
                     .poweron_period_time = 60,
                     /*Experience value on L200, datasheet is wrong */
                     .atcmd_active_time = 6000,
                     /*At least 50 ms specified in user manul, 
                      * or can send AT command "AT%MSO" to shutdown, but without logout network*/
                     .poweroff_period_time = 50,
                     .atcmd_inactive_time = 5000,
                     .ring_call_time = 10,
                     .ring_sms_time = 10,
                     }
    ,
    [GSM_UC864E] = {            /*Refer to UC864E/G Software User Guide Rev.2 - 22/10/08 */
                    .name = "UC864E",
                    .id = GSM_UC864E,
                    /*Turn GPRS_PWR pin to low level for at least 1 second, Page 14 */
                    .poweron_period_time = 1500,
                    /* Doesn't specified in the datasheet, experience value */
                    .atcmd_active_time = 8000,
                    /* Turn GPRS_PWR pin to low level for at least 2 seconds, Page 16, 
                     * or can send AT command "AT#SHDN" to shutdown */
                    .poweroff_period_time = 3000,
                    .atcmd_inactive_time = 3000,
                    .ring_call_time = 10,
                    .ring_sms_time = 10,
                    }
};

int  dev_count = ARRAY_SIZE(support_gprs);     /*Support GPRS device count, ARRAY_SIZE(support_gprs)*/

void gprs_hw_init(int which)
{	
    static unsigned char gprs_init = 0x00;

    /*If the basic GPRS GPIO port pin not initialized, then init it first */
    if(!(gprs_init & 0x01))
    {
        gprs_init |= 0x01;
        dbg_print("Initialized GSM modeul %s basic GPIO port.\n", support_gprs[which].name);


	    at91_set_A_periph (GPRS_CTS_PIN, DISPULLUP);

        //if (GSM_SIM5215 != which) 
        {
	        at91_set_A_periph (GPRS_RTS_PIN, DISPULLUP);
	        at91_set_gpio_output (GPRS_DTR_PIN, LOWLEVEL);
        }

	    at91_set_gpio_input (GPRS_DSR_PIN, DISPULLUP);
	    at91_set_gpio_input (GPRS_DCD_PIN, DISPULLUP);
	    at91_set_gpio_input (GPRS_RI_PIN, DISPULLUP);
	
	    at91_set_gpio_input (GPRS_CHK_SIM1_PIN, DISPULLUP);
	    at91_set_gpio_input (GPRS_CHK_SIM2_PIN, DISPULLUP);
	
	    at91_set_A_periph (GPRS_RXD_PIN, DISPULLUP);
	    at91_set_A_periph (GPRS_TXD_PIN, ENPULLUP);
	    
        at91_set_gpio_output (GPRS_SELECT_SIM_PIN, HIGHLEVEL); //default set to SIM1
	    
        at91_set_gpio_output (GPRS_ON_PIN, HIGHLEVEL);         //gprs module power on init 

        at91_set_gpio_output (GPRS_RESET_PIN, HIGHLEVEL);  //GTM900 need init this pin??
	    
        at91_set_deglitch(GPRS_RI_PIN, ENABLE);
    }
    /*If USB GSM module has been initilized, then go out */    
    if((gprs_init & 0x02))        
        return;

    if(GSM_UC864E == which)
    {
        gprs_init |= 0x02;
        dbg_print("Initialized USB GSM modeul %s GPIO port.\n", support_gprs[which].name);

        /*vbus is off*/
	    at91_set_gpio_output (GPRS_VBUS_CTRL_PIN, LOWLEVEL); 

        /*onlu UC864E module have the power monitor pin*/
        if(GSM_UC864E == which)
            at91_set_gpio_input (GPRS_POWER_MON_PIN, DISPULLUP);  

        /*3G module have reset pin, default is low level*/
        at91_set_gpio_output (GPRS_RESET_PIN, LOWLEVEL);      
    }
}

void gprs_powerup(int which)
{
    dbg_print("Power up %s module.\n", support_gprs[which].name);
    /*USB 3G GPRS module power up procedure will goes here, till now, only UC864E support it*/
    if (POWERON == gprs_powermon(which))
    {
        gprs_reset(which);           /*If GPRS module already power up, then we reset it */
        return;
    }
	//Currently it is power off

    /*step1: turn off vbus for 3G module*/
	if (GSM_UC864E == which)
	{		
		at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, LOWLEVEL);
		SLEEP (100);
    }
    /*step2: power on gprs module*/
	at91_set_gpio_value (GPRS_ON_PIN, LOWLEVEL);		

	SLEEP(support_gprs[which].poweron_period_time);

	at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);
    
    /**/
    if (GSM_UC864E == which)
    {
        at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, HIGHLEVEL);
    }    

    dbg_print("Delay for %ld ms wait for AT command ready\n", support_gprs[which].atcmd_active_time);
    SLEEP(support_gprs[which].atcmd_active_time);
}

void gprs_powerdown(int which)
{
    if (POWEROFF == gprs_powermon(which))   /*Alread power off */
        return;

    dbg_print("Power down %s module.\n", support_gprs[which].name);

	// Currently it is power on
    
    /*step1: turn off vbus*/
    if (GSM_UC864E == which)
    {
        at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, LOWLEVEL);
        SLEEP (100);
    }

    /*step 2: turn off gprs module*/
	at91_set_gpio_value (GPRS_ON_PIN, LOWLEVEL);

	SLEEP(support_gprs[which].poweroff_period_time);		

	at91_set_gpio_value (GPRS_ON_PIN, HIGHLEVEL);

    dbg_print("Delay for %ld ms wait for AT command shutdown\n", support_gprs[which].atcmd_inactive_time);    
    SLEEP(support_gprs[which].atcmd_inactive_time);
}

void gprs_reset(int which)
{
    /*GSM_GTM900B/C doesn't support GPRS reset */
    if (GSM_GTM900B == which || GSM_GTM900C == which)
       return;

    dbg_print("Reset %s module.\n", support_gprs[which].name);
	/*step1: turn off vbus*/	
    if(GSM_UC864E == which)
    {
        at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, LOWLEVEL);
        SLEEP (100);
    }

    /*step2: reset pin place high*/
    at91_set_gpio_value (GPRS_RESET_PIN, LOWLEVEL);
    SLEEP (10);
	at91_set_gpio_value (GPRS_RESET_PIN, HIGHLEVEL);
	SLEEP (200);

    if(GSM_UC864E == which)   
    {
        at91_set_gpio_value (GPRS_VBUS_CTRL_PIN, HIGHLEVEL);
    }

    dbg_print("Delay for %ld ms wait for AT command ready\n", support_gprs[which].atcmd_active_time);    
    SLEEP(support_gprs[which].atcmd_active_time);
}

/*Till now only UC864E support it*/
int gprs_powermon(int which)
{
    /*Only UC864E module support Power monitor ping */
    if (GSM_UC864E != which)
        return -1;
    /*Lowlevel means power off and highlevel means power on */
	if (LOWLEVEL == at91_get_gpio_value (GPRS_POWER_MON_PIN))
	{
		return POWEROFF;
	}
	return POWERON;
}

int gprs_set_worksim(int sim)  
{
    if (SIM1 != sim && SIM2 != sim)
    {
        printk("ERROR: Set unknow SIM%d to work.\n", sim);
        return -1;
    }

    dbg_print("Set SIM%d work\n", sim);
    if (SIM2 == sim)
        at91_set_gpio_value (GPRS_SELECT_SIM_PIN, LOWLEVEL);
    else
        at91_set_gpio_value (GPRS_SELECT_SIM_PIN, HIGHLEVEL);

    return 0;
}

int gprs_get_worksim(void)  
{
	if (LOWLEVEL == at91_get_gpio_value (GPRS_SELECT_SIM_PIN))
		return SIM2;
	else
		return SIM1;
}

int gprs_chk_simdoor(int sim) 
{
    if (SIM1 != sim && SIM2 != sim)
    {
        printk("ERROR: Check unknow SIM%d to work.\n", sim);
        return -1;
    }

    dbg_print("Check SIM%d work\n", sim);
	if (SIM1 == sim)
		return at91_get_gpio_value (GPRS_CHK_SIM1_PIN);
	else
		return at91_get_gpio_value (GPRS_CHK_SIM2_PIN);

    return 0;
}

void gprs_set_dtr(int which, int level)
{
	if (LOWLEVEL == level)
		at91_set_gpio_value (GPRS_DTR_PIN, LOWLEVEL);
	else
		at91_set_gpio_value (GPRS_DTR_PIN, HIGHLEVEL);
}

void gprs_set_rts(int which, int level)
{
    if (LOWLEVEL == level)
        at91_set_gpio_value (GPRS_RTS_PIN, LOWLEVEL);
    else
        at91_set_gpio_value (GPRS_RTS_PIN, HIGHLEVEL);
}

