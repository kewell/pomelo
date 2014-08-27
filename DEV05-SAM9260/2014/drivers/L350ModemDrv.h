#define MODULE_ID	201
#define MODULE_NAME "modem"
#define MODULE_VER	"1.0.0"			/*careful: strlen of MODULE_VER must < 10 */

/*para of modem pin */
#define MODEM_PIN_RESET				AT91_PIN_PB20   /*低有效 */
//#define MODEM_LNK		/*低有效 */??????????

/* Uart_0 is ttyS_1*/
#define MODEM_PIN_RXD				AT91_PIN_PB5
#define MODEM_PIN_TXD				AT91_PIN_PB4
#define MODEM_PIN_CTS				AT91_PIN_PB27
#define MODEM_PIN_RTS				AT91_PIN_PB26

#define MODEM_PIN_DTR				AT91_PIN_PB24
#define MODEM_PIN_DCD				AT91_PIN_PB23
#define MODEM_PIN_RI				AT91_PIN_PB25

#define MODEM_IOCTL_POWERON			1
#define MODEM_IOCTL_RESET			7
#define MODEM_IOCTL_DTR				5
#define MODEM_IOCTL_RING			3
#define MODEM_IOCTL_LINK			4
