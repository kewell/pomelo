#define EEP_LEN		84
#define HW_V_LEN	8
#define HW_V_BEGIN	52

/*para of ioctrl */
#define DEBUG	0

/* ioctrl(fd, cmd, 0/1); 
*/

/*if return < 0 , fail */
//#define GPRS_POWERON	1
/* ioctrl(fd, cmd, any); 
*/
//#define GPRS_POWERDOWN	3

/* ioctrl(fd, cmd, any); 
*/
#define SOFTWARE_VER	3

#define GPRS_RESET		18

#define GPRS_DTR		19

/* 
	char buf[10];
	ioctrl(fd, cmd, buf); 
	eg: you can get ver: buf[50] = "V1.0.0"
*/
#define HARDWARE_VER	4

/*
	char buf[10];
	ioctrl(fd, cmd, buf); 
	eg: you can get ver: buf[10] = "10.12.13"
*/
#define GET_SIM_SLOT	5

/*
	
	sim_slot = ioctrl(fd, cmd, any); 
	sim_slot == 1 -> sim1 is choosen
	sim_slot == 2 -> sim2 is choosen
*/
#define SET_SIM_SLOT	6

/* 
	ioctrl(fd, cmd, Slot_NO);
*/
#define MODEM_INIT      7

/* 
			
	ioctrl(fd, cmd, Modem_No); 
	Modem_No: 1~MAX_MODEM
*/
#define RS485_1_SELECT 8

/* 	
	slot_id = ioctrl(fd, cmd, any); 
*/
#define RS485_2_SELECT	9

#define RS232_SELECT	10


#define SET_LED_GREEN	11

/* 
	
	ioctrl(fd, cmd, 0/1); 
*/
#define SET_LED 12

/* 
	
	ioctrl(fd, cmd, 0/1); 
*/
#define SIM_WORK	13

/* 
	work = ioctrl(fd, cmd, any);
	1: sim card 1 working now.
	2: sim card 2 working now.
	0: no sim card working now.
*/
#define GPRS_STATE	14

/*
	state = ioctrl(fd, cmd, any);

*/
#define BEEP_ALARM	15

/*

	ioctrl(fd, cmd, freq);
	
*/
#define BEEP_DISALARM	16

/*

	ioctrl(fd, cmd, freq);
	
*/
#define OPEN_DEBUG	17

#define GET_LED_STATUS	21


/*MODULE ID */

/*
gprs_drv	
modem_drv
gpio_drv
led_drv
beep_drv
RTC_drv
eeprom_dvr

*/

/* para of sys function */
#define ENPULLUP	1
#define DISPULLUP	0

#define HIGHLEVEL	1
#define LOWLEVEL	0

#define INPUT		1
#define OUTPUT		0
