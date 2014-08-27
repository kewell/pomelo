#define MODULE_LED_MAJOR	203
#define MODULE_LED_NAME    "led"
#define MODULE_VER	       "1.0.0" /*careful: strlen of MODULE_VER must < 10 */

/* ioctl cmd */
#define LED_ON              1 
#define LED_OFF             0

/* para of gprs pin */

#define LED_LENGHT		8
#define LED_D1_RUN	    AT91_PIN_PB30
#define LED_D2_0		AT91_PIN_PB0	// Rs232/485 1
#define LED_D3_1		AT91_PIN_PB1    // RS232/485 2
#define LED_D4_2		AT91_PIN_PB2    // Rs232/485 3
#define LED_D5_3		AT91_PIN_PB21   // PSTN
#define LED_D6_4		AT91_PIN_PB31   // WIFI
#define LED_D7_5		AT91_PIN_PA22   // SIM 1
#define LED_D8_6		AT91_PIN_PC3    // SIM 2

