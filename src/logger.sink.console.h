#ifndef __DOMAIN_LOGGER_SINK_CONSOLE_H__
#define __DOMAIN_LOGGER_SINK_CONSOLE_H__

#include "logger.common.h"

/** The new Interface */
#define DOMAINLOGGER_SINK_CONSOLE_INTERFACE_OUTPUT_BUFFER_SIZE  1024

typedef struct _DomainLogSinkConsoleInterface
{
	DomainLogSinkInterface base;
	char outputBuffer[ DOMAINLOGGER_SINK_CONSOLE_INTERFACE_OUTPUT_BUFFER_SIZE ];

	uint32_t useColourOutput;

	void (*renderCb)( struct _DomainLogSinkConsoleInterface *pSink, LogMessage *pMsg );

#if( DL_PLATFORM_IS_WIN32 == 1 )
 //	HANDLE hConsole;
#endif

} DomainLogSinkConsoleInterface;


/** For the stdout console sink */
DomainLogSinkInterface* DomainLoggerConsoleSinkCreate( void );

void DomainLoggerConsoleSinkEnable( DomainLogSinkInterface *pTheSink, int consoleOutputFlags );

/* Use to destroy the console */
void DomainLoggerConsoleSinkDestroy( DomainLogSinkInterface *pTheSink );

#if ( DL_PLATFORM_IS_DARWIN == 1 )

extern void DomainLoggerConsoleNSLog( const char *message, ... ) __attribute__( ( format( printf, 1, 2 ) ) );

#endif


#endif /*  __DOMAIN_LOGGER_SINK_CONSOLE_H__ */ 