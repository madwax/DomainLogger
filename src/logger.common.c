#include "logger.common.h"

const int32_t LogThreadStateStopped = 0;
const int32_t LogThreadStateStarting = 1;
const int32_t LogThreadStateRunning = 2;
const int32_t LogThreadStateStopping = 3;

/// TODO Turn off volatile qualifiers. I can't work out what I should be using.
#pragma warning( push )
#pragma warning( disable:4090 )

#if( DL_PLATFORM_NO_CLIB == 1 )

void *LogMemoryAlloc( size_t sizeInBytes )
{
	void *r;

	r = HeapAlloc( GetProcessHeap(), 0, sizeInBytes );

	return r;
}

void* LogMemoryCopy( void *to, void *from, size_t size )
{
	CopyMemory( to, from, size );
	return to;
}

void LogMemoryFree( void *pData )
{
	if( pData == NULL )
	{
		return;
	}
	HeapFree( GetProcessHeap(), 0, pData );
}

void LogMemoryZero( void *pMemory, size_t bytesToWipe )
{
	if( pMemory != NULL && bytesToWipe != 0 )
	{
		ZeroMemory( pMemory, bytesToWipe );
	}
}

struct tm* localtime_r( time_t *clock, struct tm *result )
{
	struct tm *globalOne;

	globalOne = localtime( clock );
	if( globalOne != NULL )
	{
		*result = *globalOne;
	}

	return result;
}

struct tm* gmtime_r( time_t *clock, struct tm *result )
{
	struct tm *globalOne;

	globalOne = gmtime( clock );
	if( globalOne != NULL )
	{
		*result = *globalOne;
	}
	return result;
}

#endif

void *LogMemoryAlloc( size_t sizeInBytes )
{
	if( sizeInBytes == 0 )
	{
		return NULL;
	}
	return malloc( sizeInBytes );
}

void* LogMemoryCopy( void *to, void *from, size_t size )
{
	return memcpy( to, from, size );
}

void LogMemoryFree( void *pData )
{
	if( pData == NULL )
	{
		return;
	}
	free( pData );
}

void LogMemoryZero( void *pMemory, size_t bytesToWipe )
{
	if( pMemory != NULL && bytesToWipe != 0 )
	{
		memset( pMemory, 0, bytesToWipe );
	}
}

#if( DL_PLATFORM_IS_WIN32 == 1 )
#else

void* __Log_GCC_SwapPointer( void * volatile* pTarget, void *pWith )
{
  // Looking around the interweb turned up what follows.  The Wine and Mono projects do this
  void *r;
  do
  {
    r = *pTarget;
  }while( !__sync_bool_compare_and_swap( pTarget, r, pWith ) );
  
  return r;
}

#endif


#if( DL_PLATFORM_IS_WIN32 == 1 )

int LogConvertCharToWChar( wchar_t *to, size_t toBufferSize, const char *from )
{
	if( to != NULL && toBufferSize != 0 && from != NULL )
	{
		return MultiByteToWideChar( CP_UTF8, 0, from, -1, to, ( int )toBufferSize );
	}
	return 0;
}

int LogConvertWCharToChar( char *to, size_t toBufferSize, const wchar_t *from )
{
	if( to != NULL && toBufferSize != 0 && from != NULL )
	{
		return WideCharToMultiByte( CP_UTF8, 0, from, -1, to, ( int )toBufferSize, NULL, NULL );
	}
	return 0;
}

/* Not the fastest way to do this I know! */
wchar_t *LogFindLastDirectorySeperatorWChar( const wchar_t *source )
{
	size_t sz;
	wchar_t *pEnd;
	wchar_t *path;

	path = ( wchar_t* )source;

	sz = wcslen( path );

	pEnd = path + sz;

	while( pEnd != path )
	{
		if( *pEnd == '/' )
		{
			return pEnd;
		}

		else if( *pEnd == '\\' )
		{
			return pEnd;
		}
		--pEnd;
	}
	return NULL;
}

/* This was sourced from http://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux but things happened to it ;)  */
/** Only need this for Windows */
int LogCreateDirectoryTreeWChar( const wchar_t *path )
{
	DWORD errorCode;

	if( CreateDirectoryW( path, NULL ) )
	{
		return 1;
	}

	errorCode = GetLastError();
	if( errorCode == ERROR_ALREADY_EXISTS )
	{
		return 1;
	}

	if( errorCode == ERROR_PATH_NOT_FOUND )
	{
		wchar_t nextPath[ MAX_PATH ];
		wchar_t *last;
		
		nextPath[ 0 ] = 0;

		last = LogFindLastDirectorySeperatorWChar( path );
		if( last != NULL )
		{
			int ind;

			ind = ( int )( last - path );

			wcsncpy( nextPath, path, ind );
			nextPath[ ind ] = 0;

			if( LogCreateDirectoryTreeWChar( nextPath ) == 1 )
			{
				if( CreateDirectoryW( path, NULL ) )
				{
					return 1;
				}
				else if( GetLastError() == ERROR_ALREADY_EXISTS )
				{
					return 1;
				}
			}
		}
	}
	return 0;
}
#endif

int LogCreateDirectoryTreeChar( const char *path )
{
#if( DL_PLATFORM_IS_WIN32 == 1 )
	wchar_t tmpBuffer[ MAX_PATH ];

	LogConvertCharToWChar( tmpBuffer, MAX_PATH, path );

	return LogCreateDirectoryTreeWChar( tmpBuffer );

#elif( DL_PLATFORM_IS_UNIX == 1 )

	char nextPath[ MAX_PATH ];
	char *charAt; 
	size_t pathLength;

	pathLength = strlen( path );

	if( ( pathLength + 1 ) >= MAX_PATH )
	{
		return 0;
	}

	// don't go through stcpy as we will end up doing the strlen again!
	memcpy( nextPath, path, pathLength );
	nextPath[ pathLength ] = 0;

	charAt = nextPath + 1;

	while( *charAt != 0 )
	{
		char was;

		if( *charAt == '/' )
		{
			was = *charAt;
			*charAt = 0;

			if( mkdir( nextPath, S_IRWXU ) != 0 )
			{
				if( errno != EEXIST )
				{
					return 0;
				}
			}
			*charAt = was;
		}

		++charAt;
	}

	if( mkdir( nextPath, S_IRWXU ) != 0 )
	{
		if( errno != EEXIST )
		{
			return 0;
		}
	}
#endif
	return 1;
}

/** Used to create test that a directory exists, if not try to create it
*/
int LogValidateDirectory( const char *path )
{
#if( DL_PLATFORM_IS_WIN32 == 1 )
	wchar_t buf[ MAX_PATH ];  // Using MAX_PATH which is meaning less for NTFS but...

	MultiByteToWideChar( CP_UTF8, 0, path, -1, ( buf + 0 ), MAX_PATH );
	if( LogCreateDirectoryTreeWChar( ( buf + 0 ) ) == 1 )
	{
		return 0;
	}
#else

	if( LogCreateDirectoryTreeChar( path ) == 1 )
	{
		return 0;
	}

#endif
	return 1;
}

#if( DL_PLATFORM_IS_WIN32 == 1 )
	#define LOGSPINBUSY 1
	#define LOGSPINFREE 0
#else
	#define LOGSPINBUSY 1
	#define LOGSPINFREE 0
#endif


#if( DL_PLATFORM_IS_SPINLOCKS_MUTEXS == 1 )

void LogSpinLockCreate( LogSpinLock *lock )
{
  int er;
	
	er = pthread_mutex_init( lock, NULL );
	if( er != 0 )
	{
		fprintf( stdout, "Failed to do the unlock of mutex er:%d\n", er );
		fflush( stdout );
	}
}

int LogSpinLockTryCapture( LogSpinLock *lock )
{
  if( pthread_mutex_trylock( lock ) != 0 )
  {
    return 1;
  }
  
	return 0;
}

void LogSpinLockCapture( LogSpinLock *lock )
{
  int er;
	
	er = pthread_mutex_lock( ( pthread_mutex_t* )lock );
	if( er != 0 )
	{
		fprintf( stdout, "Failed to do the lock of mutex er:%d (%d)\n", er, EOWNERDEAD );
		fflush( stdout );
	}
}

void LogSpinLockRelease( LogSpinLock *lock )
{
	int er;
	
  if( ( er = pthread_mutex_unlock( ( pthread_mutex_t* )lock ) ) != 0 )
	{
		fprintf( stdout, "Failed to do the unlock of mutex er:%d\n", er );
		fflush( stdout );
	}
}

void LogSpinLockDestroy( LogSpinLock *lock )
{
  pthread_mutex_destroy( lock );
}

#else

void LogSpinLockCreate( LogSpinLock *lock )
{
	*lock = LOGSPINFREE;
}

int LogSpinLockTryCapture( LogSpinLock *lock )
{
	int r;
	r = 0;

	LogMemoryFullBarrier();

#if( DL_PLATFORM_IS_WIN32 == 1 )

	if( InterlockedCompareExchange( lock, LOGSPINBUSY, LOGSPINFREE ) != LOGSPINFREE )
	{
		r = 1;
	}

#elif ( DL_PLATFORM_IS_UNIX == 1 )

	if( __sync_bool_compare_and_swap( lock, LOGSPINFREE, LOGSPINBUSY ) != LOGSPINFREE )
	{
		r = 1;
	}
#endif

	LogMemoryFullBarrier();
  
	return r;
}

void LogSpinLockCapture( LogSpinLock *lock )
{
	while( 1 )
	{
		if( LogAtomicCompareExchange( lock, LOGSPINFREE, LOGSPINBUSY ) )
		{
			LogMemoryFullBarrier();
			return;
		}		
		LogThreadYeild();
	}
}

void LogSpinLockRelease( LogSpinLock *lock )
{
	LogAtomicSetInt32( lock, LOGSPINFREE );

	LogMemoryFullBarrier();
}

void LogSpinLockDestroy( LogSpinLock *lock )
{
	*lock = 0;
}

#endif

int LogMessageQueueCreate( LogMessageQueue *theQueue )
{
	if( theQueue == NULL )
	{
		return 1;
	}

	theQueue->numberIn = 0;
	theQueue->head = NULL;
	theQueue->tail = NULL;
	
	LogSpinLockCreate( &theQueue->protection );
	
	return 0;
}


int LogMessageQueueDestroy( LogMessageQueue *theQueue )
{
	if( theQueue == NULL )
	{
		return 1;
	}
	
	LogSpinLockDestroy( &theQueue->protection );

	theQueue->numberIn = 0;
	theQueue->head = NULL;
	theQueue->tail = NULL;

	return 0;
}

LogMessage *LogMessageQueuePop( LogMessageQueue *theQueue )
{
	LogMessage *pR;

	LogMemoryFullBarrier();
	
	LogSpinLockCapture( &theQueue->protection );

	pR = theQueue->head;
	if( pR != NULL )
	{
		if( pR == theQueue->tail )
		{
			if( theQueue->numberIn != 1 )
			{
				fprintf( stdout, "Clear %d \n", ( int )theQueue->numberIn );
				fflush( stdout );
			}
		
			theQueue->head = NULL;
			theQueue->tail = NULL;
			
			LogAtomicDecInt32( ( int32_t* )&theQueue->numberIn );				
		}
		else
		{
			theQueue->head = pR->next;
			LogAtomicDecInt32( ( int32_t* )&theQueue->numberIn );	
		}
	}
	else 
	{
		if( theQueue->tail != NULL )
		{
			fprintf( stdout, "Yara %d \n", ( int )theQueue->numberIn );
			fflush( stdout );
		}
	}
	
	LogMemoryFullBarrier();
	
	LogSpinLockRelease( &theQueue->protection );

	return pR;
}

LogMessage *LogMessageQueuePopChain( LogMessageQueue *theQueue, uint32_t maxNumberOf, uint32_t *numberReturned, LogMessage **pEnd )
{
	LogMessage *first;

	LogMemoryFullBarrier();

	first = NULL;

	LogSpinLockCapture( &theQueue->protection );
	if( theQueue->head == NULL )
	{
		*numberReturned = 0;
	}
	else
	{
		uint32_t count;
		LogMessage *end;		
		
		count = 0;
		first = theQueue->head;
		end = first;

		while( count < maxNumberOf && end->next != NULL )
		{
			++count;
			end = end->next;
		}

		*numberReturned = count;
		if( end->next == NULL )
		{
			theQueue->head = NULL;
			theQueue->tail = NULL;
			theQueue->numberIn = 0;
		}
		else
		{
			theQueue->head = end->next;
			theQueue->numberIn -= count;

			end->next = NULL;
		}

		*pEnd = end;
	}
	
	LogSpinLockRelease( &theQueue->protection );

	return first;
}

void LogMessageQueuePush( LogMessageQueue *theQueue, LogMessage *item )
{
	LogSpinLockCapture( &theQueue->protection );
	
	LogMemoryFullBarrier();
	
	item->next = NULL;

	if( theQueue->tail == NULL )
	{		
		if( theQueue->numberIn != 0 )
		{
			fprintf( stdout, "  BUGGER 1 %d\n", ( int )theQueue->numberIn );
			fflush( stdout );
		}
	
		if( theQueue->head != NULL )
		{
			fprintf( stdout, "  BUGGER 2\n" );
			fflush( stdout );
		}
					
		theQueue->head = item;
		theQueue->tail = item;
	}
	else
	{
		theQueue->tail->next = item;
		theQueue->tail = item;
	}
		
	LogAtomicIncInt32( ( int32_t* )&theQueue->numberIn );		

	LogMemoryFullBarrier();

	LogSpinLockRelease( &theQueue->protection );
}

void LogMessageQueuePushChain( LogMessageQueue *theQueue, LogMessage *theChainHead, LogMessage *theChainTail, uint32_t numberInChain )
{
	LogMemoryFullBarrier();

	if( theChainHead != NULL && theChainTail != NULL && numberInChain != 0 )
	{
		LogSpinLockCapture( &theQueue->protection );
	
		if( theQueue->tail == NULL )
		{
			theQueue->head = theChainHead;
			theQueue->tail = theChainTail;
		}
		else
		{
			theQueue->tail->next = theChainHead;
			theQueue->tail = theChainTail;
		}

		theQueue->numberIn += numberInChain;

		LogSpinLockRelease( &theQueue->protection );	
	}
}

LogMessageBlock *LogMessageDestroyBlock( LogMessageBlock *theBlock )
{
	LogMessageBlock *next;

	next = NULL;

	if( theBlock != NULL )
	{
		next = theBlock->next;
		LogMemoryFree( theBlock );
	}
	return next;
}

LogMessageBlock* LogMessageCreateBlock()
{
	LogMessageBlock *ret;
	LogMessage *firstMsg, *msg;
	char *memBase;
	size_t allocSize;
	uint32_t i;

	ret = NULL;
	firstMsg = NULL;

	allocSize = sizeof( LogMessageBlock ) + ( sizeof( LogMessage ) * DOMAINLOGGER_BLOCKSIZE );

	memBase = LogMemoryAlloc( allocSize );
	if( memBase == NULL )
	{
		return NULL;
	}

	ret = ( LogMessageBlock* )memBase;

	ret->next = NULL;
	ret->number = DOMAINLOGGER_BLOCKSIZE;
	ret->numberReturned = 0;

	// to the first log message object
	firstMsg = ( LogMessage* )( memBase + sizeof( LogMessageBlock ) );
	ret->firstMessage = firstMsg;

	// looking at this thinking that's slow... Any better ideas?
	i = 1;

	msg = firstMsg;

	while( i<ret->number )
	{
		firstMsg = msg;
		++msg;
		firstMsg->next = msg;
		++i;
	}
	
	// now the last message in the block. back up
	firstMsg->next = NULL;
	ret->lastMessage = firstMsg;

	return ret;
}

const char *DomainLoggingGetStringForLevel( DomainLoggingLevels level )
{
	static const char *x[] = { "fatal", "error", "system", "warning", "info", "debug", "verbose" };
	return x[ level ];
}


DomainLoggingLevels DomainLoggerReadLevelFromString( const char *str )
{
	if( str == NULL )
	{
		return DomainLoggingLevelWarning;
	}	

	if( strcasecmp( "info", str ) == 0 )
	{
		return DomainLoggingLevelInfo;
	}
	else if( strcasecmp( "debug", str ) == 0 )
	{
		return DomainLoggingLevelDebug;
	}
	else if( strcasecmp( "verbose", str ) == 0 )
	{
		return DomainLoggingLevelVerbose;
	}
	return DomainLoggingLevelWarning;
}



#if( DL_PLATFORM_IS_WIN32 == 1 )

static DWORD LogThreadEntryPoint( void *pUserData )
{
	LogThread *pTheThread;

	pTheThread = ( LogThread* )pUserData;

	LogAtomicSetInt32( &pTheThread->state, LogThreadStateRunning );

	( *pTheThread->threadFunction )( pTheThread->pThreadFunctionUserData );

	LogAtomicSetInt32( &pTheThread->state, LogThreadStateStopped );

	return 0;
}

#else

	#if( DL_PLATFORM_IS_ANDROID == 1 )

static void LogThreadExitHandler( int sigId )
{
	pthread_exit( 0 );
}	

#endif

/** The thread entry point pthreads version 
*/
static void* LogThreadEntryPoint( void *pUserData )
{
	LogThread *pTheThread;

	pTheThread = ( LogThread* )pUserData;

#if( DL_PLATFORM_IS_ANDROID == 1 )
	// Google missed out pthread_cancel, WHY!? Never mind.
	// Anyway to get around this we need to use a signal handler to kill the thread.
	// I found some of this on Stackoverflow and the Android port of VLC has there own version using local thread storage.
	struct sigaction act;
	memset( &act, 0, sizeof( struct sigaction ) );

	sigemptyset( &act.sa_mask );
	act.sa_flags = 0;
	act.sa_handler = LogThreadExitHandler;

	int er = sigaction( SIGUSR1, &act, NULL )
	if( er == -1 )
	{
		// log it but carry on.
		reportFatal( "Failed to register thread cancel signal" );
		return NULL;
	}
#endif	

	LogAtomicSetInt32( &pTheThread->state, LogThreadStateRunning );

	( *pTheThread->threadFunction )( pTheThread->pThreadFunctionUserData );

	LogAtomicSetInt32( &pTheThread->state, LogThreadStateStopped );

	return NULL;
}

#endif

int LogThreadCreate( LogThread *pTheThread, void ( *threadFunction )( void *pUserData ), void *pThreadFunctionUserData )
{
	if( pTheThread == NULL )
	{
		reportFatal( "Unable to create thread as thread data is NULL" );
		return 1;
	}

	if( threadFunction == NULL )
	{
		reportFatal( "Unable to create thread as thread function is NULL" );
		return 1;
	}

	pTheThread->state = LogThreadStateStopped;
	
	pTheThread->threadFunction = threadFunction;
	pTheThread->pThreadFunctionUserData = pThreadFunctionUserData;

	return 0;
}

int LogThreadRunning( LogThread *pTheThread )
{
	if( LogAtomicCompInt32( &pTheThread->state, LogThreadStateRunning ) != 0 )
	{
		return 1;
	}
	return 0;
}

int LogThreadStopped( LogThread *pTheThread )
{
	if( LogAtomicCompInt32( &pTheThread->state, LogThreadStateStopped ) )
	{
		return 1;
	}
	return 0;
}

int LogThreadStart( LogThread *pTheThread )
{
	if( pTheThread == NULL )
	{
		reportFatal( "Unable to start thread as its NULL" );
		return 1;
	}

	// set the state to starting, this will be change in the created thread.
	pTheThread->state = LogThreadStateStarting;

#if( DL_PLATFORM_IS_WIN32 == 1 )
	// create the thread but suppended
	pTheThread->hThread = CreateThread( NULL, 0, &LogThreadEntryPoint, ( void* )pTheThread, CREATE_SUSPENDED, &pTheThread->threadId );
	if( pTheThread->hThread == NULL )
	{
		reportFatal( "The system failed to create thread" );
		return 1; 
	}

	// how resume the thread
	if( ResumeThread( pTheThread->hThread ) == -1 )
	{
		reportFatal( "Failed to resume new thread, aborting it" );
		CloseHandle( pTheThread->hThread );

		pTheThread->hThread = NULL;
		pTheThread->threadId = 0;

		return 1;
	}
#else
	pthread_attr_init( &pTheThread->threadAttributes );
	pthread_attr_setdetachstate( &pTheThread->threadAttributes, PTHREAD_CREATE_JOINABLE );

	if( pthread_create( &pTheThread->hThread, &pTheThread->threadAttributes, &LogThreadEntryPoint, ( void* )pTheThread ) != 0 )
	{
		reportFatal( "The system failed to create thread" );
		return 1;
	}
#endif

	return 0;
}

int LogThreadKill( LogThread *pTheThread )
{
#if( DL_PLATFORM_IS_WIN32 == 1 )

	if( pTheThread == NULL )
	{
		reportFatal( "Unable to kill thread as thread object is NULL" );
		return 1;
	}

	if( pTheThread->hThread == NULL )
	{
		reportFatal( "Unable to kill thread as it does not exist!" );
		return 1;
	}

	TerminateThread( pTheThread->hThread, 0 );
	CloseHandle( pTheThread->hThread );

	pTheThread->hThread = NULL;
	pTheThread->state = LogThreadStateStopped;

#else

	#if( DL_PLATFORM_IS_ANDROID == 1 )

	/** Send the signal that will inturn kill the thread */
	pthread_kill( pTheThread->hThread, SIGUSR1 );
	
	#else
	
	pthread_cancel( pTheThread->hThread );
	
	#endif
	
	pTheThread->hThread = 0;
	pTheThread->state = LogThreadStateStopped;

#endif

	return 0;
}

ThreadId LogThreadCurrentThreadId()
{
	ThreadId r;

#if( DL_PLATFORM_IS_WIN32 == 1 )

	r = GetCurrentThreadId();

#elif( DL_PLATFORM_IS_DARWIN == 1 )

	r = pthread_mach_thread_np( pthread_self() );

#elif( DL_PLATFORM_IS_LINUX == 1 )

	r = syscall( SYS_gettid );

#elif( DL_PLATFORM_IS_ANDROID == 1 )

	r = syscall( SYS_gettid );

#elif( DL_PLATFORM_IS_FREEBSD == 1 )
	{
		long lwpid;
		thr_self( &lwpid );
		r = ( ThreadId )lwpid;
	}

#else

	r = 0;
#endif

	return r;
}

int LogThreadEventCreate( LogThreadEvent *pEvent )
{
#if( DL_PLATFORM_IS_WIN32 == 1 )

	pEvent->hEvent = CreateEventA( NULL, TRUE, FALSE, "LogFreeQueue" );
	if( pEvent->hEvent == NULL )
	{
		return 1;
	}

#endif

#if( DL_PLATFORM_IS_UNIX == 1 )

	int err;

	pEvent->created = 0;

	if( ( err = pthread_mutex_init( &pEvent->hMutex, NULL ) ) == 0 )
	{
		if( ( err = pthread_cond_init( &pEvent->hCondition, NULL ) ) == 0 )
		{
			pEvent->created = 1;
		}
		else
		{
			pthread_mutex_destroy( &pEvent->hMutex );
			return 1;
		}
	}
	else
	{
		// failed
		return 1;
	}

#endif
	
	return 0;
}

int LogThreadEventTrap( LogThreadEvent *pEvent )
{
#if( DL_PLATFORM_IS_WIN32 == 1 )
	if( pEvent->hEvent == NULL )
	{
		return 1;
	}

	SetEvent( pEvent->hEvent );
	
#endif

#if( DL_PLATFORM_IS_UNIX == 1 )

	if( pEvent->created == 0 )
	{
		return 1;
	}

	pthread_cond_signal( &pEvent->hCondition );

#endif

	return 0;
}

int LogThreadEventWait( LogThreadEvent *pEvent, int *isTimeout )
{
	int r;

	r = 1;

#if( DL_PLATFORM_IS_WIN32 == 1 )
	DWORD action;
	
	if( pEvent->hEvent != NULL )
	{
		// We wake up every second just incase something bad happens and we need to shutdown.
		action = WaitForSingleObjectEx( pEvent->hEvent, 1000, TRUE );
		if( action == WAIT_OBJECT_0 )
		{
			if( isTimeout != NULL )
			{
				*isTimeout = 0;
			}

			ResetEvent( pEvent->hEvent );
		
			r = 0;
		}
		else if( action == WAIT_TIMEOUT )
		{
			if( isTimeout != NULL )
			{
				*isTimeout = 1;
			}

			ResetEvent( pEvent->hEvent );
			r = 0;
		}
		else 
		{
			r = 1;
		}
		ResetEvent( pEvent->hEvent );
	}
#endif

#if( DL_PLATFORM_IS_UNIX == 1 )

	if( pEvent->created == 0 )
	{
		return 1;
	}

	pthread_mutex_lock( &pEvent->hMutex );

	pthread_cond_wait( &pEvent->hCondition, &pEvent->hMutex );

	pthread_mutex_unlock( &pEvent->hMutex );

#endif

	return 0;
}

void LogThreadEventDestroy( LogThreadEvent *pEvent )
{
#if( DL_PLATFORM_IS_WIN32 == 1 )

	if( pEvent->hEvent != NULL )
	{
		CloseHandle( pEvent->hEvent );
		pEvent->hEvent = NULL;
	}

#endif

#if( DL_PLATFORM_IS_UNIX == 1 )

	if( pEvent->created == 1 )
	{
    pthread_mutex_destroy( &pEvent->hMutex );
    pthread_cond_destroy( &pEvent->hCondition );		
	}

#endif
}


#if( DL_PLATFORM_NO_CLIB == 1 )
#parama warning( pop )
#endif

