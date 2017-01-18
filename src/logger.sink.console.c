#include "logger.sink.console.h"

static int DomainLoggerSinkConsoleInit( DomainLoggerSinkBase *sink )
{
	DomainLoggerSinkConsole *theSink;

	theSink = ( DomainLoggerSinkConsole* )sink;

#if( IS_WIN32 == 1 )

	if( GetConsoleWindow() == NULL )
	{
		if( IsDebuggerPresent() )
		{
			theSink->toDebugger = 1;
		}
	}

#endif

#if !( ( ( IS_WIN32 == 1 ) || ( IS_OSX == 1 ) || ( IS_BSD == 1 ) || ( IS_LINUX == 1 ) ) )

	// the platform does not allow colouring 
	theSink->isColoured = 0;

#endif

	/// do we display function name, line number etc etc...
	theSink->fullOutput = 0;

#if( IS_WIN32 == 1 )
	// now the fun bit begins
	if( theSink->toDebugger == 1 )
	{
		theSink->toDebuggerBuffer = LogMemoryAlloc( 1024 );
		if( theSink->toDebuggerBuffer == NULL )
		{
			return 1;
		}
	}
#endif

	return 0;
}

static int DomainLoggerSinkConsoleThreadInit( DomainLoggerSinkBase *sink )
{
	// We don't need to do anything.
	DLOG_UNUSED( sink );

	return 0;
}

#if( IS_WIN32 == 1 )

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

static void DomainLoggerSinkConsoleWriteOutMessage( DomainLoggerSinkConsole *theSink, LogMessage *pMsg )
{
	static char *a1[] = { "[Fatal]","[Error]","[System]","[Warning]","[Info]","[Debug]","[Verbose]" };
	static WORD a2[] = { 206, 12, 10, 14, 7, 6, 4 };

	HANDLE hStdOut;
	hStdOut = GetStdHandle( STD_OUTPUT_HANDLE );
	
	if( theSink->isColoured )
	{
		DomainLoggerSinkConsoleWriteWin32Colour( hStdOut, a1[ pMsg->level ], a2[ pMsg->level ] );						
	}
	else
	{
		DomainLoggerSinkConsoleWrite( hStdOut, a1[ pMsg->level ], 0 );
	}

	char outbu[ 100 ];

	wsprintfA( outbu, "[%s][%d] \"", pMsg->lpDomain, pMsg->threadId );
	DomainLoggerSinkConsoleWrite( hStdOut, outbu, 0 );
	DomainLoggerSinkConsoleWrite( hStdOut, pMsg->msg, pMsg->msgLength );
	DomainLoggerSinkConsoleWrite( hStdOut, "\"\n", 0 );
}

#else

static void DomainLoggerSinkConsoleWriteOutMessage( DomainLoggerSinkConsole *theSink, LogMessage *pMsg )
{
	if( theSink->isColoured )
	{
    static const char *levelsTextColoured[] = { "\e[101m\e[93m[Fatal]\e[39m\e[49m", "\e[91m[Error]\e[39m", "\e[94m[System]\e[39m", "\e[93m[Warning]\e[39m","\e[34m[Info]\e[39m", "\e[97m[Debug]\e[39m", "\e[97m[Verbose]\e[39m" };

    fprintf( stdout, "%s[%s][%d] \"%s\"\n", levelsTextColoured[ pMsg->level ], pMsg->lpDomain, pMsg->threadId, pMsg->msg );
 	}
	else
	{
    static const char *levelsToText[] = { "[Fatal]", "[Error]", "[System]", "[Warning]","[Info]", "[Debug]", "[Verbose]" };

    fprintf( stdout, "%s[%s][%d] \"%s\"\n", levelsToText[ pMsg->level ], pMsg->lpDomain, pMsg->threadId, pMsg->msg );
	}
}

#endif

static int DomainLoggerSinkConsoleProcessLogMessage( DomainLoggerSinkBase *sink, LogMessage *pMsg )
{
	DomainLoggerSinkConsole *theSink = ( DomainLoggerSinkConsole* )sink;

#if( IS_WIN32 == 1 )
	if( theSink->toDebugger == 1 )
	{

		wsprintfA( theSink->toDebuggerBuffer, "[%s][%s][%d] %s\n", DomainLoggingGetStringForLevel( pMsg->level ), pMsg->lpDomain, pMsg->threadId, pMsg->msg );
		OutputDebugStringA( theSink->toDebuggerBuffer );
	}
	else
	{
    DomainLoggerSinkConsoleWriteOutMessage( theSink, pMsg );
	}
#else
  DomainLoggerSinkConsoleWriteOutMessage( theSink, pMsg );
#endif
 
	return 0;
}

/** Used to create the consoles sink settings object if needed */
DomainLoggerSinkConsole* DomainLoggerSinkConsoleSettingsCheck( DomainLoggerSinkSettings *thisIs )
{
	static DomainLoggerSinkConsole *s_TheConsoleSink = NULL;

	if( s_TheConsoleSink == NULL )
	{
		// Create the console sink and populate it with default settings
		s_TheConsoleSink = ( DomainLoggerSinkConsole* )LogMemoryAlloc( sizeof( DomainLoggerSinkConsole ) );
		LogMemoryZero( s_TheConsoleSink, sizeof( DomainLoggerSinkConsole ) );

		s_TheConsoleSink->base.onInitCb = &DomainLoggerSinkConsoleInit;
		s_TheConsoleSink->base.onThreadInitCb = &DomainLoggerSinkConsoleThreadInit;
		s_TheConsoleSink->base.onProcessLogMessageCb = &DomainLoggerSinkConsoleProcessLogMessage;

		s_TheConsoleSink->isColoured = 0;

#if( IS_WIN32 == 1 )
		s_TheConsoleSink->toDebugger = 0;
		s_TheConsoleSink->toDebuggerBuffer = NULL;
#endif 
	
		thisIs->theSink = ( DomainLoggerSinkBase* )s_TheConsoleSink;
	}

	return s_TheConsoleSink;
}

int DomainLoggerSinkConsoleSettingsCMDHandler( DomainLoggerSinkSettings *thisIs, DomainLoggerCommonSettings *pCommonSettings, char *option, int currentIndex, int argc, char **argv )
{
	DLOG_UNUSED( argc );
	DLOG_UNUSED( argv );
	DLOG_UNUSED( currentIndex );
	DLOG_UNUSED( pCommonSettings );

	if( strcasecmp( "console.colour", option ) == 0 )
	{
		DomainLoggerSinkConsoleSettingsCheck( thisIs )->isColoured = 1;
		return 0;
	}
#if( IS_WIN32 == 1 )
	else if( strcasecmp( "console.debug", option ) == 0 )
	{
		DomainLoggerSinkConsoleSettingsCheck( thisIs )->toDebugger = 1;
		DomainLoggerSinkConsoleSettingsCheck( thisIs )->isColoured = 0;
		return 0;
	}
#endif
	else if( strcasecmp( "console", option ) == 0 )
	{
		/* enables the console. */
		DomainLoggerSinkConsoleSettingsCheck( thisIs );
		return 0;
	}
	return -1;
}

DomainLoggerSinkSettings* DomainLoggerSinkConsoleCreateSettings()
{
	DomainLoggerSinkSettings *pR;

	pR = ( DomainLoggerSinkSettings* )LogMemoryAlloc( sizeof( DomainLoggerSinkSettings ) );
	if( pR != NULL )
	{
		pR->pNext = NULL;
		pR->theSink = NULL;
		pR->onHandleCMDCb = &DomainLoggerSinkConsoleSettingsCMDHandler;
	}
	return pR;
}



