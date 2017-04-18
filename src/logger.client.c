#include "logger.client.h"
#include "logger.sink.file.h"
#include "logger.sink.console.h"

static DomainLogSinkInterface *pTheConsoleLogSink = NULL;
static DomainLogSinkInterface *pTheFileLogSink = NULL;

#if( DL_PLATFORM_IS_UNIX == 1 )

static uint64_t GetTickCount64()
{
	uint64_t r;
	struct timespec x;

	clock_gettime( CLOCK_MONOTONIC, &x );

	r = x.tv_sec * 1000;
	if( x.tv_nsec != 0 )
	{
		r += ( x.tv_nsec / 1000000 ); 
	}

	return r;
}

#endif


void DomainLoggerConsoleEnable( int consoleOutputFlags )
{
	if( pTheConsoleLogSink == NULL )
	{
		pTheConsoleLogSink = DomainLoggerConsoleSinkCreate();
		DomainLoggerAddSink( pTheConsoleLogSink );
	}

	DomainLoggerConsoleSinkEnable( pTheConsoleLogSink, consoleOutputFlags );
}

void DomainLoggerFileSet( const char *path, const char *prefix )
{
	if( pTheFileLogSink == NULL )
	{
		pTheFileLogSink = DomainLoggerFileSinkCreate();
		pTheFileLogSink->enabled = 1;

		DomainLoggerAddSink( pTheFileLogSink );
	}	

	DomainLoggerFileSinkSetPath( pTheFileLogSink, path, prefix );
}

void DomainLoggerFileSetW( const wchar_t *path, const char *prefix )
{
	if( pTheFileLogSink == NULL )
	{
		pTheFileLogSink = DomainLoggerFileSinkCreate();
		DomainLoggerAddSink( pTheFileLogSink );
	}	

	DomainLoggerFileSinkSetPathW( pTheFileLogSink, path, prefix );
}

int DomainLoggerFileDirSet()
{
	if( pTheFileLogSink == NULL )
	{
		return 0;
	}

	return DomainLoggerFileSinkPathSet( pTheFileLogSink );
}

#if( DL_PLATFORM_IS_WIN32 == 1 )

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

static int theLoggerClientInited = 0;

/** Used to check the client log struct is inited */
static void DomainLoggerClientCheck();
/** Used to read what level do we have */
static int DomainLoggerClientCreateDomain( const char *whichDomain, DomainLoggingLevels testLevel );
static int DomainLoggerClientPattenMatchDomainToPreDomain( DomainLoggerClientDomainInfo *lpDomain, DomainLoggerClientDomainInfo *lpPreDomain );
static DomainLogSinkItem *DomainLoggerSinkItemCreate( DomainLogSinkInterface *theSink );
static void DomainLoggerSinkItemDestroy(  DomainLogSinkItem *theSinkItem );

static void DomainLoggerGetDateTime( DomainLoggerDateTime *toSet, int useUTC )
{
#if( DL_PLATFORM_IS_WIN32 == 1 )
	
	SYSTEMTIME theCurrentTime;

	if( useUTC )
	{
		GetSystemTime( &theCurrentTime );
	}
	else
	{
		GetLocalTime( &theCurrentTime );
	}

	toSet->year = theCurrentTime.wYear;
	toSet->month = theCurrentTime.wMonth;
	toSet->day = theCurrentTime.wDay;

	toSet->hours = theCurrentTime.wHour;
	toSet->minutes = theCurrentTime.wMinute;
	toSet->seconds = theCurrentTime.wSecond;

#elif( DL_PLATFORM_IS_UNIX == 1 )
	time_t nowIs;
	struct tm theCurrentTime;

	time( &nowIs );

	if( useUTC )
	{
		gmtime_r( &nowIs, &theCurrentTime );
	}
	else
	{
		localtime_r( &nowIs, &theCurrentTime );
	}

	toSet->year = theCurrentTime.tm_year;
	toSet->month = theCurrentTime.tm_mon;
	toSet->day = theCurrentTime.tm_mday;

	toSet->hours = theCurrentTime.tm_hour;
	toSet->minutes = theCurrentTime.tm_min;
	toSet->seconds = theCurrentTime.tm_sec;

#else
#error Failed to get time
#endif 
}


static void DomainLoggerClientCheck()
{
	if( theLoggerClientInited == 1 )
	{
		return;
	}

	theLoggerClientInited = 1;

	LogMemoryZero( &theLoggerClient, sizeof( DomainLoggerClient ) );

	theLoggerClient.defaultLoggingLevel = DomainLoggingLevelWarning;
	theLoggerClient.domainsNumber = 0;
	theLoggerClient.preDomainsNumber = 0;
	theLoggerClient.numberOfSinks = 0;
	theLoggerClient.isRunning = 0;
	
	// when was this created.
	theLoggerClient.whenEpoc = GetTickCount64();

	DomainLoggerGetDateTime( &theLoggerClient.createdOnUTC, 1 );
	DomainLoggerGetDateTime( &theLoggerClient.createdOnLocal, 0 );
	
	LogThreadEventCreate( &theLoggerClient.theThreadEvent );

	LogSpinLockCreate( &theLoggerClient.theQueueProtection );
	LogSpinLockCreate( &theLoggerClient.theFreeQueueProtection );

	LogMessageQueueCreate( &theLoggerClient.theQueue );
	LogMessageQueueCreate( &theLoggerClient.theFreeQueue );
}

DomainLoggerClient* DomainLoggerClientGet()
{
	DomainLoggerClientCheck();
	return &theLoggerClient;
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
		// time stamp it. The number of miliseconds from system startup.
		pR->when = GetTickCount64();

		// which thread
		pR->threadId = LogThreadCurrentThreadId();

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


static void DomainLoggerWorkerThreadSendToSinks( LogMessage *pTarget )
{
	DomainLogSinkItem *pTheSinkItem;
	DomainLogSinkInterface *pTheSink;
	size_t i, sz;
	
	sz = theLoggerClient.numberOfSinks;

	// we have messages to pass onto the first sink
	for( i=0; i<sz; ++i )
	{
		pTheSinkItem = theLoggerClient.theSinks[ i ];
		pTheSink = pTheSinkItem->theSink;
		
		if( pTheSink->enabled == 1 )
		{
			if( pTheSink->OnLoggingThreadOnProcessMessageCb != NULL )
			{
				( *( pTheSink->OnLoggingThreadOnProcessMessageCb ) )( pTheSink, pTarget );
			}
		}
	}
}

/** The Loggers worker thread entry point 
* This bad boy does what we need it to do
*/
void DomainLoggerWorkerThreadEntryPoint( void *pUserData )
{
	LogMessage *firstMessages, *lastMessage;
	DomainLoggerClient *pTheClient;
	DomainLogSinkItem *pItem;
	uint32_t numberOfMessages;
	uint32_t sz,i;
	int inTimeout, shutdownNow;

	pTheClient = ( DomainLoggerClient* )pUserData;
	
	for( i=0, sz=theLoggerClient.numberOfSinks; i<sz; ++i )
	{
		pItem = theLoggerClient.theSinks[ i ];
		if( pItem->theSink->OnLoggingThreadInitCb != NULL )
		{
			( *( pItem->theSink->OnLoggingThreadInitCb ) )( pItem->theSink );
		}
	}
	
	pTheClient->isRunning = 1;

	shutdownNow = 0;

	// keep running untill shutdownFlag goes true.
	while( shutdownNow == 0 )
	{
		firstMessages = NULL;

		// Pull up to 10 messages from the incoming log queue
		// get this over and done with so we don't hold up worker threads.
		LogSpinLockCapture( &theLoggerClient.theQueueProtection );
		
		if( theLoggerClient.theQueue.numberIn )
		{
			firstMessages = LogMessageQueuePopChain( &theLoggerClient.theQueue, 10, &numberOfMessages, &lastMessage );
		}
		LogSpinLockRelease( &theLoggerClient.theQueueProtection );

		if( firstMessages == NULL )
		{
			if( theLoggerClient.shutdownFlag == 1 )
			{
				shutdownNow = 1;
			}
			else
			{
				LogThreadEventWait( &theLoggerClient.theThreadEvent, &inTimeout );
				if( inTimeout )
				{		
					// Any house keeping tasks.		
				}
			}
		}
		else
		{
			LogMessage *pTarget;

			pTarget = firstMessages;

			while( pTarget != NULL )
			{
				DomainLoggerWorkerThreadSendToSinks( pTarget );
				pTarget = pTarget->next;
			}

			// now return the messages to the free queue
			LogSpinLockCapture( &theLoggerClient.theFreeQueueProtection );
			LogMessageQueuePushChain( &theLoggerClient.theFreeQueue, firstMessages, lastMessage, numberOfMessages );
			LogSpinLockRelease( &theLoggerClient.theFreeQueueProtection );
		}
	}
	
	// We need to flush whats in the queue.
	if( i == sz )
	{
	}
}


/************************* Public API *************************/

DomainLoggingLevels DomainLoggerLevelIs( const char *whatLevel )
{
	const char *strVerbose = "verbose";
	const char *strDebug = "debug";
	const char *strInfo = "info";
	
	if( strncasecmp( strVerbose, whatLevel, strlen( strVerbose ) ) == 0 )
	{
		return DomainLoggingLevelVerbose;
	}

	if( strncasecmp( strDebug, whatLevel, strlen( strDebug ) ) == 0 )
	{
		return DomainLoggingLevelDebug;
	}

	if( strncasecmp( strInfo, whatLevel, strlen( strInfo ) ) == 0 )
	{
		return DomainLoggingLevelInfo;
	}

	return DomainLoggingLevelWarning;
}


void DomainLoggerSetDefaultLevel( DomainLoggingLevels newDefaultLevel )
{
	DomainLoggerClientCheck();

	theLoggerClient.defaultLoggingLevel = newDefaultLevel;
}

void DomainLoggerSetLevelToDomain( const char *domain, DomainLoggingLevels loggingLevel )
{
	DomainLoggerClientDomainInfo incoming, *pInfo, *pEnd;
	uint32_t sz, found, foundPre;

	DomainLoggerClientCheck();

	found = 0;
	foundPre = 0;

	incoming.domain = ( char* )domain;
	incoming.domainLength = strlen( domain );
	incoming.level = loggingLevel;

	if( ( sz = theLoggerClient.domainsNumber ) )
	{
		pInfo = theLoggerClient.domains + 0;
		pEnd = theLoggerClient.domains + sz;

		while( pInfo != pEnd )
		{
			if( DomainLoggerClientPattenMatchDomainToPreDomain( &incoming, pInfo ) )
			{
				found = 1;
				// we have a hit
				pInfo->level = loggingLevel;
			}
			++pInfo;
		}
	}

	// go after the predefined domains
	if( ( sz = theLoggerClient.preDomainsNumber ) )
	{
		pInfo = theLoggerClient.preDomains + 0;
		pEnd = theLoggerClient.preDomains + sz;

		while( pInfo != pEnd )
		{
			if( pInfo->domainLength == incoming.domainLength )
			{
				if( strcmp( pInfo->domain, incoming.domain ) == 0 )
				{
					foundPre = 1;
					pInfo->level = loggingLevel;
				}
			}
			++pInfo;
		}
	}
	
	if( foundPre == 0 )
	{
		// we need to add to pre.
		theLoggerClient.preDomains[ sz ].domainLength = incoming.domainLength;
		theLoggerClient.preDomains[ sz ].level = loggingLevel;

		theLoggerClient.preDomains[ sz ].domain = LogMemoryAlloc( incoming.domainLength + 1 );
		LogMemoryZero( theLoggerClient.preDomains[ sz ].domain, incoming.domainLength + 1 );
		strncpy( theLoggerClient.preDomains[ sz ].domain, incoming.domain, incoming.domainLength );

		++sz;
		theLoggerClient.preDomainsNumber = sz;
	}
}

int DomainLoggerStart( const char *applicationName, const char *applicationIdetifier )
{
	DomainLoggerClientCheck();
	
	/* Now it's time to start the loggers thread running! */
	theLoggerClient.shutdownFlag = 0;
	theLoggerClient.numberOfLogMessages = 0;
	theLoggerClient.numberOfBlocks = 0;
	theLoggerClient.theBlocksHead = NULL;

	theLoggerClient.theApplicationName = ( char* )applicationName;
	theLoggerClient.theApplicationIdentifier = ( char* )applicationIdetifier;

	LogThreadCreate( &theLoggerClient.theWorkerThread, &DomainLoggerWorkerThreadEntryPoint, ( void* )&theLoggerClient );

	LogThreadStart( &theLoggerClient.theWorkerThread );

	// we wait for the logging thread to start up and get running.
	while( theLoggerClient.isRunning == 0 )
	{
		LogThreadYeild();
	}

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
	return DomainLoggingLevelWarning;
}

static int DomainLoggerClientCreateDomain( const char *whichDomain, DomainLoggingLevels testLevel )
{
	uint32_t sz;
	int ret;
	DomainLoggerClientDomainInfo *pNewDomain;

	if( theLoggerClient.domainsNumber == DOMAINLOGGER_DOMAINS_MAXNUMBER )
	{
		return -1;
	}

	// now the fun bit starts.
	ret = 0;

	pNewDomain = ( theLoggerClient.domains + theLoggerClient.domainsNumber );

	// Every domain start off with the default level 
	pNewDomain->domain = ( char* )whichDomain;
	pNewDomain->domainLength = strlen( whichDomain );
	pNewDomain->level = theLoggerClient.defaultLoggingLevel;
	
	sz=theLoggerClient.preDomainsNumber;
	
	if( theLoggerClient.preDomainsNumber )
	{
		uint32_t i;
	
		for( i=0; i<sz; ++i )
		{
			if( DomainLoggerClientPattenMatchDomainToPreDomain( pNewDomain, ( theLoggerClient.preDomains + i ) ) == 1 )
			{
				pNewDomain->level = ( theLoggerClient.preDomains + i )->level;
			}
		}
	}

	if( testLevel <= pNewDomain->level )
	{
		ret = 1;
	}

	++theLoggerClient.domainsNumber;

	return ret;
}


int DomainLoggerTest( const char *whichDomain, DomainLoggingLevels toTestIfUsable )
{
	int ret;

	// We reuse the free queue protection lock as it's private and there is no way we can interlock
	LogSpinLockCapture( &theLoggerClient.theFreeQueueProtection );

	for( uint32_t i=0; i<theLoggerClient.domainsNumber; ++i )
	{
		if( whichDomain == theLoggerClient.domains[ i ].domain )
		{
			int x = theLoggerClient.domains[ i ].level;
			LogSpinLockRelease( &theLoggerClient.theFreeQueueProtection );

			if( toTestIfUsable <= x )
			{
				return 1;
			}
			else
			{
				return 0;
			}
		}
	}

	ret = -1;


	// we need to add the new domain.
	if( theLoggerClient.domainsNumber == DOMAINLOGGER_DOMAINS_MAXNUMBER )
	{
		reportFatal( "YOU HAVE MAXED OUT NUMBER OF DOMAINS ALLOWED" );
	}	
	else
	{
		DomainLoggerClientDomainInfo *pNextNewDomain;

		pNextNewDomain = theLoggerClient.domains + theLoggerClient.domainsNumber;

		LogSpinLockRelease( &theLoggerClient.theFreeQueueProtection );
	}

	return ret;
}

void DomainLoggerPost( const char *whichDomain, DomainLoggingLevels underLevel, const char *filename, int lineNumber, const char *functionName, const char *msg, ... )
{
	va_list va;

	int test;

	test = DomainLoggerTest( whichDomain, underLevel );
	if( test == -1 )
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

			lpToSend->msgLength = ( uint16_t )strlen( lpToSend->msg );

			lpToSend->level = underLevel;
			lpToSend->lpDomain = ( char* )whichDomain;

			lpToSend->fileName = ( char* )filename;
			lpToSend->lineNumber = lineNumber;
			lpToSend->functionName = ( char* )functionName;

			LogSpinLockCapture( &theLoggerClient.theQueueProtection );
			LogMessageQueuePush( &theLoggerClient.theQueue, lpToSend );
			LogSpinLockRelease( &theLoggerClient.theQueueProtection );

			LogThreadEventTrap( &theLoggerClient.theThreadEvent );
		}
	}
}

int DomainLoggerClose()
{
	uint32_t i;
	LogMessageBlock *pMsgBlock;
	DomainLoggerClientDomainInfo *pDomainInfo;

	// flag the logging engine that's it.
	LogAtomicSetInt32( &theLoggerClient.shutdownFlag, 1 );
	
	LogThreadEventTrap( &theLoggerClient.theThreadEvent );

	while( LogThreadStopped( &theLoggerClient.theWorkerThread ) == 0 )
	{
		// not stopped yep.
		LogThreadYeild();
	}

	// need to clean up the domains
	pDomainInfo = theLoggerClient.domains + 0;
	for( i=0; i<theLoggerClient.domainsNumber; ++i, ++pDomainInfo )
	{
		pDomainInfo->domain = NULL;
		pDomainInfo->domainLength = 0;
		pDomainInfo->level = DomainLoggingLevelWarning;
	}

	// need to clean up the sink items.
	for( i=0; i<theLoggerClient.numberOfSinks; ++i )
	{
		DomainLoggerSinkItemDestroy( theLoggerClient.theSinks[ i ] );
	}
	theLoggerClient.numberOfSinks = 0;

	// now the locks.
	LogSpinLockDestroy( &theLoggerClient.theQueueProtection );
	LogSpinLockDestroy( &theLoggerClient.theFreeQueueProtection );

	LogMessageQueueDestroy( &theLoggerClient.theQueue );
	LogMessageQueueDestroy( &theLoggerClient.theFreeQueue );

	if( ( pMsgBlock = theLoggerClient.theBlocksHead ) != NULL )
	{
		theLoggerClient.theBlocksHead = NULL;

		while( pMsgBlock != NULL )
		{
			pMsgBlock = LogMessageDestroyBlock( pMsgBlock );
		}
	}

	pTheConsoleLogSink = NULL;
	pTheFileLogSink = NULL;

	theLoggerClientInited = 0;
	return 0;
}

static DomainLogSinkItem *DomainLoggerSinkItemCreate( DomainLogSinkInterface *theSink )
{
	DomainLogSinkItem *r;
	size_t sz;

	sz = sizeof( DomainLogSinkItem );

	r = ( DomainLogSinkItem* )LogMemoryAlloc( sz );
	if( r == NULL )
	{
		return NULL;
	}

	LogMemoryZero( r, sz );

	r->theSink = theSink;

	return r;
}

static void DomainLoggerSinkItemDestroy(  DomainLogSinkItem *theSinkItem )
{
	if( theSinkItem == NULL )
	{
		return;
	}

	if( theSinkItem->theSink->OnLoggingShutdownCb != NULL )
	{
		( *( theSinkItem->theSink->OnLoggingShutdownCb ) )( theSinkItem->theSink );
		theSinkItem->theSink = NULL;
	}

	LogMemoryFree( theSinkItem );
}

int DomainLoggerAddSink( DomainLogSinkInterface *theSink )
{
	DomainLogSinkItem *newItem;

	DomainLoggerClientCheck();
	
	if( theSink == NULL )
	{
		return 1;
	}

	if( theLoggerClient.numberOfSinks >= DOMAINLOGGER_MAX_NUMBER_OF_SINKS )
	{
		return 1;
	}

	newItem = DomainLoggerSinkItemCreate( theSink );
	if( newItem == NULL )
	{
		return 1;
	}

	theLoggerClient.theSinks[ theLoggerClient.numberOfSinks ] = newItem;
	++theLoggerClient.numberOfSinks;

	return 0;
}