#define MODULE_ID	204
#define MODULE_NAME 	"beep"
#define MODULE_VER	"1.0.0"

#define BEEP_PIN	AT91_PIN_PC2

#define BEEP_DEBUG
#ifdef	BEEP_DEBUG
	#define print(x) printk x
#else
	#define print(x)
#endif


