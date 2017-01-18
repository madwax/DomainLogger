#include "logger.common.h"

const int32_t LogThreadStateStopped = 0;
const int32_t LogThreadStateStarting = 1;
const int32_t LogThreadStateRunning = 2;
const int32_t LogThreadStateStopping = 3;

#if( NO_CLIB_WIN32 == 1 )

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

#else

	#if( IS_WIN32 == 1 )

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

	globalOne = localtime( clock );
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

	#if( IS_WIN32 == 0 )

	#error bhw
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

#endif

#if( IS_WIN32 == 1 )

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
#endif

#if( IS_WIN32 == 1 )

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
#endif

#if( IS_WIN32 == 1 )
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

#if defined( _WIN32 )
	/** NOT TESTED */

	/* Under windows we don't start with '?:\' or '\\' then it's not a valid path */
	if( path[ 1 ] != ':' )
	{
		if( path[ 1 ] != '\\' )
		{
			return 0;
		}

		if( path[ 0 ] != '\\' )
		{
			return 0;
		}

		charAt = nextPath + 2;
	}
	else
	{
		if( !( path[ 2 ] == '\\' || path[ 2 ] == '/' ) )
		{
			return 0;
		}

		charAt = nextPath + 3;
	}

#else

	charAt = nextPath + 1;

#endif

	while( *charAt != 0 )
	{
		char was;

#if defined( _WIN32 )
		if( *charAt == '\\' || *charAt == '/' )
#else
		if( *charAt == '/' )
#endif
		{
			was = *charAt;
			*charAt = 0;

#if defined( _WIN32 ) 
			if( CreateDirectoryA( nextPath, NULL ) == 0 )
			{
				if( GetLastError() != ERROR_ALREADY_EXISTS )
				{
					return 0;
				}
			}
#else
			if( mkdir( nextPath, S_IRWXU ) != 0 )
			{
				if( errno != EEXIST )
				{
					return 0;
				}
			}
#endif

			*charAt = was;
		}

		++charAt;
	}

#if defined( _WIN32 )
	if( CreateDirectoryA( nextPath, NULL ) != 0 )
	{
		if( GetLastError() != ERROR_ALREADY_EXISTS )
		{
			return 0;
		}
	}
#else
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
#if( IS_WIN32 )
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

#if( IS_WIN32 == 1 )
	#define LOGSPINBUSY 1
	#define LOGSPINFREE 0
#else
	#define LOGSPINBUSY 1
	#define LOGSPINFREE 0
#endif

void LogSpinLockCreate( LogSpinLock *lock )
{
#if( __APPLE__ )
  
  pthread_mutex_init( lock, NULL );
  
#else
	*lock = LOGSPINFREE;
#endif
}

int LogSpinLockTryCapture( LogSpinLock *lock )
{
	int r;
	r = 0;

#if( __APPLE__ )
  
  if( pthread_mutex_trylock( lock ) == 0 )
  {
    r = 1;
  }
  
#else
	LogMemoryFullBarrier();

#if( IS_WIN32 == 1 )

	if( InterlockedCompareExchange( lock, LOGSPINBUSY, LOGSPINFREE ) != LOGSPINFREE )
	{
		r = 1;
	}

#elif ( IS_UNIX == 1 )

	if( __sync_bool_compare_and_swap( lock, LOGSPINFREE, LOGSPINBUSY ) != LOGSPINFREE )
	{
		r = 1;
	}
#endif

	LogMemoryFullBarrier();
#endif
  
	return r;
}

void LogSpinLockCapture( LogSpinLock *lock )
{
#if( __APPLE__ )
  
  pthread_mutex_lock( lock );
  
#else
	while( LogSpinLockTryCapture( lock ) )
	{
		LogThreadYeild();
	}
#endif
}

void LogSpinLockRelease( LogSpinLock *lock )
{
#if( __APPLE__ )
  
  pthread_mutex_unlock( lock );
  
#else
  
	LogMemoryFullBarrier();

#if( IS_WIN32 == 1 )

	InterlockedCompareExchange( lock, LOGSPINFREE, LOGSPINBUSY );

#elif( IS_UNIX == 1 )

	__sync_bool_compare_and_swap( lock, LOGSPINBUSY, LOGSPINFREE );

#endif
	LogMemoryFullBarrier();
#endif
  
}

void LogSpinLockDestroy( LogSpinLock *lock )
{
#if( __APPLE__ )
  
  pthread_mutex_destroy( lock );
  
#else
	*lock = 0;
#endif
}

void LogReadWriteLockCreate( LogReadWriteLock *rwLock )
{
	LogSpinLockCreate( &rwLock->lock );
	rwLock->readCounter = 0;
}

void LogReadWriteLockReadCapture( LogReadWriteLock *rwLock )
{
	LogSpinLockCapture( &rwLock->lock );

#if( IS_WIN32 == 1 )
	
	InterlockedIncrement( &rwLock->readCounter );

#elif ( IS_UNIX == 1 )
	LogAtomicIncInt32( &rwLock->readCounter );
#endif

	LogSpinLockRelease( &rwLock->lock );
}

void LogReadWriteLockReadRelease( LogReadWriteLock *rwLock )
{
	LogSpinLockCapture( &rwLock->lock );

#if( IS_WIN32 == 1 )

	InterlockedDecrement( &rwLock->readCounter );

#elif ( IS_UNIX ==	1 )

	LogAtomicDecInt32( &rwLock->readCounter );

#endif

	LogSpinLockRelease( &rwLock->lock );
}

void LogReadWriteLockReadToWritePromote( LogReadWriteLock *rwLock )
{
	LogSpinLockCapture( &rwLock->lock );

#if( IS_WIN32 == 1 )

	InterlockedDecrement( &rwLock->readCounter );

	while( InterlockedCompareExchange( &rwLock->readCounter, 0, 0 ) != 0 )
	{
		LogThreadYeild();
	}

#elif ( IS_UNIX ==	1 )

	LogAtomicDecInt32( &rwLock->readCounter );

	while( __sync_bool_compare_and_swap( &rwLock->readCounter, 0, 0 ) == 0 )
	{
		LogThreadYeild();
	}

#endif
}

void LogReadWriteLockWriteCapture( LogReadWriteLock *rwLock )
{
	LogSpinLockCapture( &rwLock->lock );
}

void LogReadWriteLockWriteRelease( LogReadWriteLock *rwLock )
{
	LogSpinLockRelease( &rwLock->lock );
}

void LogReadWriteLockDestroy( LogReadWriteLock *rwLock )
{
	LogSpinLockDestroy( &rwLock->lock );
	rwLock->readCounter = 0;
}

int LogMessageQueueCreate( LogMessageQueue *theQueue )
{
	if( theQueue == NULL )
	{
		return 1;
	}

	theQueue->numberIn = 0;
	theQueue->head = NULL;
	theQueue->tail = NULL;

#if( __APPLE__ )
  LogSpinLockCreate( &theQueue->lock );
#endif
  
	return 0;
}

LogMessage *LogMessageQueuePop( volatile LogMessageQueue *theQueue )
{
	volatile LogMessage *pR;

#if( __APPLE__ )
  
  LogMemoryFullBarrier();
  asm volatile("" ::: "memory");
  
  LogSpinLockCapture( &theQueue->lock );
  
  pR = theQueue->head;
  if( pR != NULL )
  {
    theQueue->head = pR->next;
    
    if( theQueue->tail == pR )
    {
      theQueue->tail = NULL;
    }
    
    pR->next = NULL;
   
    LogAtomicDecInt32( &theQueue->numberIn );
  }
 
  LogMemoryFullBarrier();
  asm volatile("" ::: "memory");
  
  LogSpinLockRelease( &theQueue->lock );
  
#else 

	LogMemoryFullBarrier();
 
	if( ( pR = LogAtomicCompareExchangePointer( ( void* volatile* ) &theQueue->head, NULL, NULL ) ) != NULL )
	{
		LogAtomicExchangePointer( ( void* volatile* ) &theQueue->head, pR->next );
		LogAtomicCompareExchangePointer( ( void* volatile* ) &theQueue->tail, pR->next, pR );
		
		LogAtomicDecInt32( &theQueue->numberIn );
    
    pR->next = NULL;
	}

	LogMemoryFullBarrier();
#endif
  
	return ( LogMessage* )pR;
}

void LogMessageQueuePush( volatile LogMessageQueue *theQueue, volatile LogMessage *item )
{
#if( __APPLE__ )
  item->next = NULL;
	  
  LogMemoryFullBarrier();
  asm volatile("" ::: "memory");
 
  LogSpinLockCapture( &theQueue->lock );
  
  /*
  int jf = 0;
  if( theQueue->head == NULL )
  {
    jf = 100;
  }
  */
  if( theQueue->tail == NULL )
  {
    /*
    ++jf;
    if( theQueue->head != NULL )
    {
      printf( "BAD" );
    }
    */
    theQueue->tail = item;
    theQueue->head = item;
  }
  else
  {
    theQueue->tail->next = item;
    theQueue->tail = item;
  }
  
  /**
  if( theQueue->head == NULL )
  {
    printf( " fd %d %d %d\n", jf, ( int )item->lineNumber, ( int )item->count );
  }
  */
  
  LogAtomicIncInt32( &theQueue->numberIn );
  
  LogMemoryFullBarrier();
  asm volatile("" ::: "memory");
  
  LogSpinLockRelease( &theQueue->lock );
  
#else

  volatile LogMessage *was;

	if( item == NULL )
	{
		return;
	}

  if( item->next != NULL )
  {
    return;
  }
  
	LogMemoryFullBarrier();
  
	was = LogAtomicCompareExchangePointer( ( void* volatile* )&( theQueue->tail ), item, NULL );
	if( was == NULL )
	{
		LogAtomicExchangePointer( ( void* volatile* )&theQueue->head, item );
	}
	else
	{
		LogAtomicExchangePointer( ( void* volatile* )&theQueue->tail->next, item );
		LogAtomicExchangePointer( ( void* volatile* )&theQueue->tail, item );
	}

	LogAtomicIncInt32( &theQueue->numberIn );

	LogMemoryFullBarrier();
#endif
}

void LogMessageQueuePushChain( LogMessageQueue *theQueue, LogMessage *theChainHead, LogMessage *theChainTail, uint32_t numberInChain )
{
	LogMemoryFullBarrier();
  
	if( LogAtomicCompareExchangePointer( ( void* volatile* )&theQueue->head, NULL, NULL ) == NULL )
	{
		LogAtomicExchangePointer( ( void* volatile* )&theQueue->head, theChainHead );
		LogAtomicExchangePointer( ( void* volatile* )&theQueue->tail, theChainTail );
	}
	else
	{
		LogAtomicExchangePointer( ( void* volatile* )&theQueue->tail->next, theChainHead );
		LogAtomicExchangePointer( ( void* volatile* )&theQueue->tail, theChainTail );
	}
		
	LogAtomicSetInt32( &theQueue->numberIn, numberInChain );
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
		return LogLevelWarning;
	}	

	if( strcasecmp( "info", str ) == 0 )
	{
		return LogLevelInfo;
	}
	else if( strcasecmp( "debug", str ) == 0 )
	{
		return LogLevelDebug;
	}
	else if( strcasecmp( "verbose", str ) == 0 )
	{
		return LogLevelVerbose;
	}
	return LogLevelWarning;
}



#if( IS_WIN32 == 1 )

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

	#if( IS_ANDROID == 1 )

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

#if( IS_ANDROID == 1 )
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

#if( IS_WIN32 == 1 )
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
#if( IS_WIN32 == 1 )

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
	#if( IS_ANDROID == 1 )
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

#if( IS_WIN32 == 1 )
	r = GetCurrentThreadId();
#elif( IS_OSX == 1 )
	r = pthread_mach_thread_np( pthread_self() );
#elif( IS_IOS == 1 )
	r = pthread_mach_thread_np( pthread_self() );
#elif( IS_LINUX == 1 )
	r = syscall( SYS_gettid );
#elif( IS_ANDROID == 1 )
	r = syscall( SYS_gettid );
#elif( IS_FREEBSD == 1 )
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

int LogFileCreate( LogFile *pFile )
{
	pFile->sourceType = LogFileSourceNone;

#if( IS_WIN32 == 1 )
	pFile->hFile = INVALID_HANDLE_VALUE;
#else
	pFile->hFile = -1;
#endif
	return 0;
}

int LogFileOpenForWritingFilePathWChar( LogFile *pFile, const wchar_t *filePath )
{
	int r;

	r = 1;

#if( IS_WIN32 == 1 )
	// Lots of options!? Need to think about
	pFile->hFile = CreateFileW( filePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	if( pFile->hFile == INVALID_HANDLE_VALUE )
	{
		reportFatal( "Failed to open log file" );
	}
	else
	{
		r = 0;
		pFile->sourceType = LogFileSourceFile;
	}
#endif
	return r;
}

int LogFileOpenForWritingFilePath( LogFile *pFile, const char *filePath )
{
	int r;

	r = 1;

#if( IS_WIN32 == 1 )
	// Lots of options!? Need to think about
	pFile->hFile = CreateFileA( filePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	if( pFile->hFile == INVALID_HANDLE_VALUE )
	{
		reportFatal( "Failed to open log file" );
	}
	else
	{
		r = 0;
		pFile->sourceType = LogFileSourceFile;
	}
#else
	pFile->hFile = open( filePath, O_CREAT|O_APPEND|O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	if( pFile->hFile == -1 )
	{
		reportFatal( "Failed to open log file" );
	}
	else
	{
		r = 0;
		pFile->sourceType = LogFileSourceFile;
	}
#endif
	return r;
}

/** Use to open stdout so we can write to it using common code */
int LogFileOpenStdOut( LogFile *pFile )
{
  int r;
  
#if( IS_WIN32 == 1 )
	pFile->hFile = GetStdHandle( STD_OUTPUT_HANDLE );
	
	// if the handle is invalid its due to the application does not have a stdout which for a Window window's app is the default
	if( pFile->hFile != INVALID_HANDLE_VALUE )
	{
		r = 0;
		pFile->sourceType = LogFileSourceStdOut;
	}
  else
  {
    r = 1;
  }
  
#elif( IS_IOS == 1 )
	// iOS does not allow access to stdout
#else
	// Everyone else!	
	pFile->hFile = 1;
	pFile->sourceType = LogFileSourceStdOut;

	r = 0;
#endif
	
	return r;
}


int LogFileWrite( LogFile *pFile, const char *data, uint32_t dataSize )
{
	int r;
	r = 1;
	switch( pFile->sourceType )
	{
		case LogFileSourceFile:
		case LogFileSourceStdOut:
		{
#if( IS_WIN32 == 1 )
			if( pFile->hFile != INVALID_HANDLE_VALUE )
			{
				DWORD bytesWritten;
				char *d;

				d = ( char* )data;

				bytesWritten = 0;

				while( dataSize )
				{
					if( WriteFile( pFile->hFile, d, dataSize, &bytesWritten, NULL ) == 0 )
					{
						reportFatal( "Failed to wrtite to file" );
						return 1;
					}

					dataSize -= bytesWritten;
					d += bytesWritten;
				}

				r = 0;
			}
#else
			if( pFile->hFile != 0 )
			{
				if( write( pFile->hFile, ( const void* )data, dataSize ) != -1 )
				{
					r = 0;
				}
				else
				{
					reportFatal( "Failed to wrtite to file" );
				}
			}
#endif			
		}break;

		default:
		{
		}break;
	}

	return r;
}

int LogFileIsOpen( LogFile *pFile )
{
	int r;

	r = 0;
	switch( pFile->sourceType )
	{
		case LogFileSourceFile:
		case LogFileSourceStdOut:
		{
#if( IS_WIN32 == 1 )
			if( pFile->hFile != INVALID_HANDLE_VALUE )
			{
				r = 1;
			}
#else
			if( pFile->hFile != -1 )
			{
				r = 1;
			}
#endif
		}break;

		default:
		{
		}break;
	}

	return r;
}


int LogFileFlush( LogFile *pFile )
{
#if( IS_WIN32 == 1)
	if( pFile->hFile == INVALID_HANDLE_VALUE )
	{
		return 1;
	}

	FlushFileBuffers( pFile->hFile );
#else
	if( pFile->hFile == -1 )
	{
		return 1;
	}

	fsync( pFile->hFile );
#endif
	return 0;
}

int LogFileClose( LogFile *pFile )
{
	switch( pFile->sourceType )
	{
		case LogFileSourceFile:
		{
#if( IS_WIN32 == 1 )
			if( pFile->hFile != INVALID_HANDLE_VALUE )
			{
				CloseHandle( pFile->hFile );
				pFile->hFile = INVALID_HANDLE_VALUE;
			}
#else
			if( pFile->hFile != 0 )
			{
				close( pFile->hFile );
				pFile->hFile = -1;
			}
#endif
		}break;

		case LogFileSourceStdOut:
		{
#if( IS_WIN32 == 1 )
			pFile->hFile = INVALID_HANDLE_VALUE;
#else
			pFile->hFile = -1;
#endif
		}break;
      
    default:
    {
    }break;
	}

	pFile->sourceType = LogFileSourceNone;
	return 0;
}
