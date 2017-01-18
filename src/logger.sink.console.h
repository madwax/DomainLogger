#ifndef __DOMAIN_LOGGER_SINK_CONSOLE_H__
#define __DOMAIN_LOGGER_SINK_CONSOLE_H__

#include "logger.common.h"

/** The console sink object 
*/
typedef struct
{
	DomainLoggerSinkBase base;
	/** 1 if output is to be coloured */
	int isColoured;
	
	/** If console data should be send to the debugger e.g. OutputDebugString */
#if( IS_WIN32 == 1 )
	int toDebugger;
	char *toDebuggerBuffer;
#endif
	/** Report function name, file etc etc etc... */
	int fullOutput;

} DomainLoggerSinkConsole;

extern DomainLoggerSinkSettings* DomainLoggerSinkConsoleCreateSettings();


#endif /*  __DOMAIN_LOGGER_SINK_CONSOLE_H__ */ 