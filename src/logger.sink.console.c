#include "logger.sink.console.h"


static void DomainLoggerConsoleSinkThreadInit( DomainLogSinkInterface *pSink )
{
	DLOG_UNUSED( pSink );

/*
	DomainLogSinkConsoleInterface *pIs;
	pIs = ( DomainLogSinkConsoleInterface* )pSink->data;
*/
}

static void DomainLoggerConsoleSinkProcessMessage( DomainLogSinkInterface *pSink, LogMessage *pMsg )
{
	DomainLogSinkConsoleInterface *pIs;

	pIs = ( DomainLogSinkConsoleInterface* )pSink->data; 

	if( pIs->renderCb != NULL )
	{
		( *( pIs->renderCb ) )( pIs, pMsg );
	}
}

void DomainLoggerConsoleSinkDestroy( DomainLogSinkInterface *pTheSink )
{
	DomainLogSinkConsoleInterface *pTheConsoleSink;
	
	pTheConsoleSink = ( DomainLogSinkConsoleInterface* )pTheSink->data;
	
	LogMemoryFree( pTheConsoleSink );
}

DomainLogSinkInterface* DomainLoggerConsoleSinkCreate( void )
{
	DomainLogSinkConsoleInterface *r;
	size_t sz;

	sz = sizeof( DomainLogSinkConsoleInterface );
	if( ( r = ( DomainLogSinkConsoleInterface* )LogMemoryAlloc( sz ) ) == NULL )
	{
		return NULL;
	}

	LogMemoryZero( r, sz );

	r->base.data = ( void* )r;

	r->useColourOutput = 0;		
	r->base.enabled = 0;
	
	r->base.OnLoggingThreadInitCb = &DomainLoggerConsoleSinkThreadInit;
	r->base.OnLoggingThreadOnProcessMessageCb = &DomainLoggerConsoleSinkProcessMessage;
	r->base.OnLoggingThreadClosedCb = NULL;
	r->base.OnLoggingShutdownCb = &DomainLoggerConsoleSinkDestroy;

	r->renderCb = NULL;

	return &r->base;
}

static char *DomainLoggerConsoleLoggingLevels[] = { "[Fatal]","[Error]","[System]","[Warning]","[Info]","[Debug]","[Verbose]" };


#if( DL_PLATFORM_IS_WIN32 == 1 )

static void DomainLoggerSinkConsoleWrite( HANDLE hTo, char *data, DWORD len )
{
	DWORD d;

	if( len == 0 )
	{
		len = ( DWORD )strlen( data );
	}

	while( len != 0 )
	{
		if( WriteFile( hTo, data, len, &d, NULL ) == 0 )
		{
			len = 0;
		}
		else
		{
			len -= d;
		}
	}
}


static void DomainLoggerSinkConsoleWriteWin32Colour( HANDLE hTo, const char *msg, WORD colour )
{
	if( colour != 0 )
	{
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;

		GetConsoleScreenBufferInfo( hTo, &consoleInfo );
		SetConsoleTextAttribute( hTo, colour );
		DomainLoggerSinkConsoleWrite( hTo, ( char* )msg, ( DWORD )strlen( msg ) );
		SetConsoleTextAttribute( hTo, consoleInfo.wAttributes );
	}
	else
	{
		DomainLoggerSinkConsoleWrite( hTo, ( char* )msg, ( DWORD )strlen( msg ) );
	}
}


static WORD DomainLoggerConsoleLoggerLevelColours[] = { 206, 12, 10, 14, 7, 6, 4 };

	
static void DomainLoggerConsoleSinkMono( DomainLogSinkConsoleInterface *pTheSink, LogMessage *pMsg )
{
	HANDLE hStdOut;
	if( ( hStdOut = GetStdHandle( STD_OUTPUT_HANDLE ) ) !=  INVALID_HANDLE_VALUE )
	{
		snprintf( pTheSink->outputBuffer, DOMAINLOGGER_SINK_CONSOLE_INTERFACE_OUTPUT_BUFFER_SIZE, "%s[%s][%d] \"", DomainLoggerConsoleLoggingLevels[ pMsg->level ], pMsg->lpDomain, pMsg->threadId );
		DomainLoggerSinkConsoleWrite( hStdOut, pTheSink->outputBuffer, 0 );
		DomainLoggerSinkConsoleWrite( hStdOut, pMsg->msg, pMsg->msgLength );
		DomainLoggerSinkConsoleWrite( hStdOut, "\"\n", 0 );
	}
}


static void DomainLoggerConsoleSinkColour( DomainLogSinkConsoleInterface *pTheSink, LogMessage *pMsg )
{
	HANDLE hStdOut;

	if( ( hStdOut = GetStdHandle( STD_OUTPUT_HANDLE ) ) !=  INVALID_HANDLE_VALUE )
	{
		snprintf( pTheSink->outputBuffer, DOMAINLOGGER_SINK_CONSOLE_INTERFACE_OUTPUT_BUFFER_SIZE, "[%s][%d] \"", pMsg->lpDomain, pMsg->threadId );

		DomainLoggerSinkConsoleWriteWin32Colour( hStdOut, DomainLoggerConsoleLoggingLevels[ pMsg->level ], DomainLoggerConsoleLoggerLevelColours[ pMsg->level ] );		
		DomainLoggerSinkConsoleWrite( hStdOut, pTheSink->outputBuffer, 0 );
		DomainLoggerSinkConsoleWrite( hStdOut, pMsg->msg, pMsg->msgLength );
		DomainLoggerSinkConsoleWrite( hStdOut, "\"\n", 0 );
	}
}


static void DomainLoggerConsoleSinkDebugger( DomainLogSinkConsoleInterface *pTheSink, LogMessage *pMsg )
{
	snprintf( pTheSink->outputBuffer, DOMAINLOGGER_SINK_CONSOLE_INTERFACE_OUTPUT_BUFFER_SIZE - 1, "%s[%s][%d] \"%s\"\n", DomainLoggerConsoleLoggingLevels[ pMsg->level ], pMsg->lpDomain, pMsg->threadId, pMsg->msg );
	OutputDebugStringA( pTheSink->outputBuffer );
}

#else


static void DomainLoggerConsoleSinkMono( DomainLogSinkConsoleInterface *pTheSink, LogMessage *pMsg )
{
	 fprintf( stdout, "%s[%s][%d] \"%s\"\n", DomainLoggerConsoleLoggingLevels[ pMsg->level ], pMsg->lpDomain, pMsg->threadId, pMsg->msg );
}


static void DomainLoggerConsoleSinkColour( DomainLogSinkConsoleInterface *pTheSink, LogMessage *pMsg )
{
  static const char *levelsTextColoured[] = { "\e[101m\e[93m[Fatal]\e[39m\e[49m", "\e[91m[Error]\e[39m", "\e[94m[System]\e[39m", "\e[93m[Warning]\e[39m","\e[34m[Info]\e[39m", "\e[97m[Debug]\e[39m", "\e[97m[Verbose]\e[39m" };
  fprintf( stdout, "%s[%s][%d] \"%s\"\n", levelsTextColoured[ pMsg->level ], pMsg->lpDomain, pMsg->threadId, pMsg->msg );
}


static void DomainLoggerConsoleSinkDebugger( DomainLogSinkConsoleInterface *pTheSink, LogMessage *pMsg )
{
#if( DL_PLATFORM_IS_DARWIN == 1 )
  DomainLoggerConsoleNSLog( "%s[%s][%d] \"%s\"\n", DomainLoggerConsoleLoggingLevels[ pMsg->level ], pMsg->lpDomain, pMsg->threadId, pMsg->msg );
#endif
}

#endif

void DomainLoggerConsoleSinkEnable( DomainLogSinkInterface *pTheSink, int consoleOutputFlags )
{
	DomainLogSinkConsoleInterface *pX = ( DomainLogSinkConsoleInterface* )pTheSink->data;

	if( pX->useColourOutput == consoleOutputFlags )
	{
		return;
	}

	pX->useColourOutput = consoleOutputFlags;

	if( consoleOutputFlags == DomainLoggerConsoleOutputDisabled )
	{
		pTheSink->enabled = 0;
		pX->renderCb = NULL;
	}
	else
	{
		pTheSink->enabled = 1;

		if( consoleOutputFlags == DomainLoggerConsoleOutputMono )
		{
			pX->renderCb = &DomainLoggerConsoleSinkMono;
		}
		else if( consoleOutputFlags == DomainLoggerConsoleOutputColoured )
		{
		
			pX->renderCb = &DomainLoggerConsoleSinkColour;
		}
		else if( consoleOutputFlags == DomainLoggerConsoleOutputDebugger )
		{
			pX->renderCb = &DomainLoggerConsoleSinkDebugger;
		}
	}
}


