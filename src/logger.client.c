#include "logger.client.h"
#include "logger.sink.file.h"
#include "logger.sink.console.h"

#if( IS_WIN32 == 1 )

#include <Shlobj.h>
#include <Knownfolders.h>
#include <wchar.h>

void reportFatal( const char *msg )
{
	OutputDebugStringA( msg );
}

#else

void reportFatal( const char *msg )
{
	fprintf( stderr, "%s", msg );
}

#endif

static DomainLoggerClient theLoggerClient;

/** Used to check the client log struct is inited */
static void DomainLoggerClientCheck();

/** Used to read what level do we have */
static int DomainLoggerClientCreateDomain( const char *whichDomain, DomainLoggingLevels testLevel );
static int DomainLoggerOpenReadCMD( int argc, char **argv );
static int DomainLoggerClientPattenMatchDomainToPreDomain( DomainLoggerClientDomainInfo *lpDomain, DomainLoggerClientDomainInfo *lpPreDomain );

/** Use to register a new sink settings object to the client object 
* You only ever call this in the init function and any object added (not the created sinks but the settings objects) are freed.
*/
static void DomainLoggerClientAddSinkSettings( DomainLoggerSinkSettings *sinkSettings )
{
	sinkSettings->pNext = theLoggerClient.theHeadSinkSettings;
	theLoggerClient.theHeadSinkSettings = sinkSettings;
} 

static void DomainLoggerClientCheck()
{
	static int theLoggerClientInited = 0;
	
	if( theLoggerClientInited == 1 )
	{
		return;
	}

	theLoggerClientInited = 1;

	LogMemoryZero( &theLoggerClient, sizeof( DomainLoggerClient ) );


	theLoggerClient.theCommonSettings.defaultLogLevel = LogLevelWarning;

	theLoggerClient.theHeadSinkSettings = NULL;
	theLoggerClient.theSinksHead = NULL;

	LogSpinLockCreate( &theLoggerClient.theQueueProtection );
	LogSpinLockCreate( &theLoggerClient.theFreeQueueProtection );

	LogMessageQueueCreate( &theLoggerClient.theQueue );
	LogMessageQueueCreate( &theLoggerClient.theFreeQueue );

	DomainLoggerClientAddSinkSettings( DomainLoggerSinkConsoleCreateSettings() );
	DomainLoggerClientAddSinkSettings( DomainLoggerSinkFileCreateSettings() );
}

static int DomainLoggerOpenReadCMDOptionValid( char *v )
{
	char *p;
	char s[] = "--log.";
	int i=0;
	
	p = ( s + 0 );

	while( i<6 )
	{
		if( *v == 0 )
		{
			return 0;
		}

		if( *v != *p )
		{
			return 0;
		}

		++v;
		++p;
		++i;
	}

	return 1;
}

static int DomainLoggerOpenReadCMD( int argc, char **argv )
{
	char *item;

	for( int i=0; i<argc; ++i )
	{
		item = argv[ i ];

		if( DomainLoggerOpenReadCMDOptionValid( item ) )
		{
			item += 6;

			/* first we do common settings */
			if( strcasecmp( "level", item ) == 0 )  // --log.level %level%
			{
				++i;
				if( i == argc )
				{
					reportFatal( "Missing value for --log.level" );
					return 0;
				}
				theLoggerClient.theCommonSettings.defaultLogLevel = DomainLoggerReadLevelFromString( argv[ i ] );
			}
			else if( strcasecmp( "domain", item ) == 0 ) // --log.domain %domain% %level% - Allows the setting of a given domain (with whild cards)
			{
				++i;
				if( i == argc )
				{
					reportFatal( "Missing domain name for --log.domain" );
					return 0;
				}
				if( ( i + 1 ) == argc )
				{
					reportFatal( "Missing logging level for --log.domain" );
					return 0;
				}

				if( theLoggerClient.theCommonSettings.preDefinedDomainsNumber < DOMAINLOGGER_PREDOMAINS_MAXNUMBER )
				{
					theLoggerClient.theCommonSettings.preDefinedDomains[ theLoggerClient.theCommonSettings.preDefinedDomainsNumber ].domain = argv[ i ];
					theLoggerClient.theCommonSettings.preDefinedDomains[ theLoggerClient.theCommonSettings.preDefinedDomainsNumber ].domainLength = strlen( theLoggerClient.theCommonSettings.preDefinedDomains[ theLoggerClient.theCommonSettings.preDefinedDomainsNumber ].domain );
					++i;
					theLoggerClient.theCommonSettings.preDefinedDomains[ theLoggerClient.theCommonSettings.preDefinedDomainsNumber ].level = DomainLoggerReadLevelFromString( argv[ i ] );
					++theLoggerClient.theCommonSettings.preDefinedDomainsNumber;
				}
			}
			else
			{
				DomainLoggerSinkSettings *p;
				int hasR;

				// Now on to the registered sink settings objects.
				p = theLoggerClient.theHeadSinkSettings;

				while( p != NULL )
				{
					hasR = ( *p->onHandleCMDCb )( p, &theLoggerClient.theCommonSettings, item, i, argc, argv );
					if( hasR == -1 )
					{
						return 0;
					}
					else if( hasR == -2 )
					{
						p = p->pNext;
					}
					else
					{
						p = NULL;
						i += hasR;
					}
				}
			}
		}
	}
	return 0;
}

static int DomainLoggerClientPattenMatchDomainToPreDomain( DomainLoggerClientDomainInfo *lpDomain, DomainLoggerClientDomainInfo *lpPreDomain )
{
	char *d;
	char *p;
	size_t len;
	size_t i;

	len = lpDomain->domainLength;

	if( lpPreDomain->domainLength < len )
	{
		len = lpPreDomain->domainLength;
	}

	d = lpDomain->domain;
	p = lpPreDomain->domain;
	i = 0;

	while( i<len )
	{
		if( *p == '*' )
		{
			// we have a match due to whild card
			return 1;
		}
		
		if( *p != *d )
		{
			// no match as the strings differ
			return 0;
		}
	
		++p;
		++d;
		++i;
	}

	// we have a string to string match.
	return 1;
}

static int DomainLoggerClientCreateDomain( const char *whichDomain, DomainLoggingLevels testLevel )
{
	int ret;
	DomainLoggerClientDomainInfo *lpNewDomain;

	if( theLoggerClient.domainsNumber == DOMAINLOGGER_DOMAINS_MAXNUMBER )
	{
		return -1;
	}

	// now the fun bit starts.
	ret = 0;

	lpNewDomain = theLoggerClient.domains + theLoggerClient.domainsNumber;

	// Every domain start off with the default level 
	lpNewDomain->domain = ( char* )whichDomain;
	lpNewDomain->domainLength = strlen( whichDomain );

	lpNewDomain->level = theLoggerClient.theCommonSettings.defaultLogLevel;

	// now scan through the list of predefined domains and see if we have a match and apply.
	// These are applied in order on the command line so you might get abit of a shock!
	if( theLoggerClient.theCommonSettings.preDefinedDomainsNumber )
	{
		uint32_t i;
		for( i=0; i<theLoggerClient.theCommonSettings.preDefinedDomainsNumber; ++i )
		{
			DomainLoggerClientDomainInfo *lpPre;
			
			lpPre = theLoggerClient.theCommonSettings.preDefinedDomains + i;
			if( DomainLoggerClientPattenMatchDomainToPreDomain( lpNewDomain, lpPre ) == 1 )
			{
				// we have a match
				lpNewDomain->level = lpPre->level;
			}
		}
	}

	if( testLevel <= theLoggerClient.domains[ theLoggerClient.domainsNumber ].level )
	{
		ret = 1;
	}

	++theLoggerClient.domainsNumber;

	return ret;
}

LogMessage* DomainLoggerClientGetFreeMessage()
{
	LogMessage *pR;

	LogSpinLockCapture( &theLoggerClient.theFreeQueueProtection );

	if( ( pR = LogMessageQueuePop( &theLoggerClient.theFreeQueue ) ) == NULL )
	{
		// need to create a block of log messages and return one of them.
		LogMessageBlock *newLogMessageBlock;

		newLogMessageBlock = LogMessageCreateBlock();
		if( newLogMessageBlock == NULL )
		{
			reportFatal( "Failed to allocate a log message block!" );
			return NULL;
		}

		newLogMessageBlock->next = theLoggerClient.theBlocksHead;
		theLoggerClient.theBlocksHead = newLogMessageBlock;
		++theLoggerClient.numberOfBlocks;

		pR = newLogMessageBlock->firstMessage;

		LogMessageQueuePushChain( &theLoggerClient.theFreeQueue, ( LogMessage* )pR->next, newLogMessageBlock->lastMessage, newLogMessageBlock->number );
	}

	LogSpinLockRelease( &theLoggerClient.theFreeQueueProtection );

	if( pR != NULL )
	{
		pR->next = NULL;
	}

	return pR;
}

void DomainLoggerClientReturnFreeMessage( LogMessage *msg )
{
	LogSpinLockCapture( &theLoggerClient.theFreeQueueProtection );
	LogMessageQueuePush( &theLoggerClient.theFreeQueue, msg );
	LogSpinLockRelease( &theLoggerClient.theFreeQueueProtection );
}

static void DomainLoggerPrePassArgsOn( char *item, char **extra, int numberOf )
{
	DomainLoggerSinkSettings *p;

	p = theLoggerClient.theHeadSinkSettings;
	while( p != NULL )
	{
		// TODO clean up!
		( *p->onHandleCMDCb )( p, &theLoggerClient.theCommonSettings, item, 0, numberOf, extra );
		p = p->pNext;
	}
}

/** The Loggers worker thread entry point 
* This bad boy does what we need it to do
*/
void DomainLoggerWorkerThreadEntryPoint( void *pUserData )
{
	DomainLoggerClient *pTheClient;
	LogMessage *pCurrentMessage;
	DomainLoggerSinkBase *pSink;
	uint32_t isRunning;
	uint32_t numberOfMessagesPerTripAround;
	const uint32_t maxNumberOfMessagePreTripAround = 4;

	pTheClient = ( DomainLoggerClient* )pUserData;

	isRunning = 1;
	numberOfMessagesPerTripAround = 0;

	// need to call the thread init callbacks for what sinks we have 
	pSink = pTheClient->theSinksHead;
	
	while( pSink != NULL )
	{
		if( pSink->onThreadInitCb != NULL )
		{
			(*( pSink->onThreadInitCb ) )( pSink );
		}
		pSink = pSink->pNext;	
	}	

	while( isRunning )
	{
		numberOfMessagesPerTripAround = 0;

		while( numberOfMessagesPerTripAround <= maxNumberOfMessagePreTripAround )
		{
			LogSpinLockCapture( &pTheClient->theQueueProtection );
			pCurrentMessage = LogMessageQueuePop( &pTheClient->theQueue );
			LogSpinLockRelease( &pTheClient->theQueueProtection );

			if( pCurrentMessage != NULL )
			{
				// we have a message to process.
				pSink = pTheClient->theSinksHead;
				while( pSink != NULL )
				{
					( *( pSink->onProcessLogMessageCb ) )( pSink, pCurrentMessage );
					pSink = pSink->pNext;
				}

				LogSpinLockCapture( &pTheClient->theFreeQueueProtection );
				LogMessageQueuePush( &pTheClient->theFreeQueue, pCurrentMessage );
				LogSpinLockRelease( &pTheClient->theFreeQueueProtection );

				++numberOfMessagesPerTripAround;
			}
			else
			{
				numberOfMessagesPerTripAround = maxNumberOfMessagePreTripAround + 10;

				// As the queue is now empty check to see if we are to shutdown or not?
				if( LogAtomicCompInt32( &pTheClient->shutdownFlag, 1 ) )
				{
					// Today is a good day to die!
					isRunning = 0;				
				}
			}
		}
	
		if( numberOfMessagesPerTripAround == maxNumberOfMessagePreTripAround )
		{
			// well pulled the max number of messages allowed for this trip aroound the loop so there might be more, if so only yeild()
			LogThreadYeild();			
		}
		else
		{
			// We thing the queue is empty
			// TODO use something other than yeild()
			LogThreadYeild();			
		}
	}

	// clean up tile for the sinks
	pSink = pTheClient->theSinksHead;
	while( pSink != NULL )
	{
		//( *( pSink->onProcessLogMessageCb ) )( pSink, pCurrentMessage );
		pSink = pSink->pNext;
	}
}

/************************* Public API *************************/

int DomainLoggerPreSetDefaultLevel( DomainLoggingLevels newDefaultLevel )
{
	DomainLoggerClientCheck();

	theLoggerClient.theCommonSettings.defaultLogLevel = newDefaultLevel;

	return 0;
}

int DomainLoggerPreFileLoggingDisabled()
{
	DomainLoggerClientCheck();

	DomainLoggerPrePassArgsOn( "file.disabled", NULL, 0 );

	return 0;
}

int DomainLoggerPreFileLoggingOveridePath( const char *logDirectory )
{
	char *x[ 1 ];
	DomainLoggerClientCheck();

	x[ 0 ] = ( char* )logDirectory;

	DomainLoggerPrePassArgsOn( "file.path", x, 1 );

	return 0;
}

int DomainLoggerPreConsoleLoggingEnable( int enableColourOutput )
{
	DomainLoggerClientCheck();

	if( enableColourOutput )
	{
		DomainLoggerPrePassArgsOn( "console.colour", NULL, 0 );
	}
	else
	{
		DomainLoggerPrePassArgsOn( "console", NULL, 0 );
	}
	return 0;
}

int DomainLoggerOpen( int argc, char **argv, const char *userDefinedApplicaitonName )
{
	DomainLoggerSinkSettings *pSettings;

	DomainLoggerClientCheck();
	
	if( userDefinedApplicaitonName == NULL )
	{
		return 1;
	}

	if( DomainLoggerOpenReadCMD( argc, argv ) )
	{
		return 1;
	}
	
	strncpy( theLoggerClient.theCommonSettings.applicationName, userDefinedApplicaitonName, DOMAINLOGGER_APPLICATION_NAME_MAX_LENGTH );

	/* Now time to move the sinks into place and get them into a ready state. */
	pSettings = theLoggerClient.theHeadSinkSettings;
	while( pSettings != NULL )
	{
		DomainLoggerSinkSettings *was;

		was = pSettings;

		// add the active sinks and init them.
		if( pSettings->theSink != NULL )
		{
			pSettings->theSink->pCommonSettings = &theLoggerClient.theCommonSettings;

			pSettings->theSink->pNext = theLoggerClient.theSinksHead;
			theLoggerClient.theSinksHead = pSettings->theSink;
			( *( theLoggerClient.theSinksHead->onInitCb ) )( was->theSink );
		}

		pSettings = pSettings->pNext;
		
		LogMemoryFree( was );		
	}

	/* Now it's time to start the loggers thread running! */
	theLoggerClient.shutdownFlag = 0;
	theLoggerClient.numberOfLogMessages = 0;
	theLoggerClient.numberOfBlocks = 0;
	theLoggerClient.theBlocksHead = NULL;

	LogThreadCreate( &theLoggerClient.theWorkerThread, &DomainLoggerWorkerThreadEntryPoint, ( void* )&theLoggerClient );

	LogThreadStart( &theLoggerClient.theWorkerThread );

	return 0;
}

DomainLoggingLevels DomainLoggerGetLevel( const char *whichDomain )
{
	for( uint32_t i=0; i<theLoggerClient.domainsNumber; ++i )
	{
		if( whichDomain == theLoggerClient.domains[ i ].domain )
		{
			return theLoggerClient.domains[ i ].level;
		}
	}
	return LogLevelWarning;
}

int DomainLoggerTest( const char *whichDomain, DomainLoggingLevels toTestIfUsable )
{
	for( uint32_t i=0; i<theLoggerClient.domainsNumber; ++i )
	{
		if( whichDomain == theLoggerClient.domains[ i ].domain )
		{
			if( toTestIfUsable <= theLoggerClient.domains[ i ].level )
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
	}
	return -1;
}

void DomainLoggerPost( const char *whichDomain, DomainLoggingLevels underLevel, const char *filename, int lineNumber, const char *functionName, const char *msg, ... )
{
	va_list va;

	int test;

	test = DomainLoggerTest( whichDomain, underLevel );
	if( test != 1 )
	{
		test = DomainLoggerClientCreateDomain( whichDomain, underLevel );
	}

	// Are we able to send the message?
	if( test == 1 )
	{
		LogMessage *lpToSend;
 
		if( ( lpToSend = DomainLoggerClientGetFreeMessage() ) != NULL )
		{
			va_start( va, msg );
			vsnprintf( lpToSend->msg, DOMAINLOGGER_MESSAGETEXTSIZE - 1, msg, va );
			va_end( va );

			lpToSend->msgLength = strlen( lpToSend->msg );


			lpToSend->level = underLevel;
			lpToSend->lpDomain = ( char* )whichDomain;

			lpToSend->fileName = ( char* )filename;
			lpToSend->lineNumber = lineNumber;
			lpToSend->functionName = ( char* )functionName;

			LogSpinLockCapture( &theLoggerClient.theQueueProtection );
			LogMessageQueuePush( &theLoggerClient.theQueue, lpToSend );
			LogSpinLockRelease( &theLoggerClient.theQueueProtection );
		}
	}
}

int DomainLoggerClose()
{
	// flag the logging engine that's it.
	LogAtomicSetInt32( &theLoggerClient.shutdownFlag, 1 );

	while( LogThreadStopped( &theLoggerClient.theWorkerThread ) == 0 )
	{
		// not stopped yep.
		LogThreadYeild();
	}

	return 0;
}

