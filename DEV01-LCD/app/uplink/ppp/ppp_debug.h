#ifndef _PPP_DEBUG_H
#define _PPP_DEBUG_H

#define PPP_DEBUG_USE_STD		0

#define PPPLVL_ALM		1
#define PPPLVL_NOTE		2
#define PPPLVL_DATA		8

#if PPP_DEBUG_USE_STD == 1
#include <stdarg.h>
static inline void ppp_debug(int level, char *format, ...)
{
	va_list va;

	va_start(va, format);
	vprintf(format, va);
	va_end(va);
}
#else
#include "include/debug.h"
#define ppp_debug	PrintLog
#endif

#endif /*_PPP_DEBUG_H*/

