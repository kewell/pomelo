#define __BGA__              1

/*************************public defination***********************/
#if __BGA__
        #define __DRIVER_VERSION__ "BGA"
#endif

/*************************macro defination***********************/
#define MODULE_ID    		241
#define MODULE_NAME  		"rs485"
#define MODULE_VER          "1.0.0"

#define RS485_1_CRTL			AT91_PIN_PB3
#define RS485_2_CRTL			AT91_PIN_PA29

#define RS485_1_TXD			AT91_PIN_PB8
#define RS485_2_TXD			AT91_PIN_PB31
#define RS485_1_RXD			AT91_PIN_PB9
#define RS485_2_RXD			AT91_PIN_PB30
