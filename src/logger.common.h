#ifndef __DOMAIN_LOGGER_COMMON_H__
#define __DOMAIN_LOGGER_COMMON_H__

/* Change this if you want to build every thing using std c lib on windows */
#define NO_CLIB_WIN32 0

#ifndef DOMAINLOGGER_BLOCKSIZE 
	#define DOMAINLOGGER_BLOCKSIZE 100
#endif 

#ifndef DOMAINLOGGER_BLOCKSALLOWED
	#define DOMAINLOGGER_BLOCKSALLOWED 12
#endif 

#ifndef DOMAINLOGGER_PREDOMAINS_MAXNUMBER
	#define DOMAINLOGGER_PREDOMAINS_MAXNUMBER 20
#endif 

#ifndef DOMAINLOGGER_DOMAINS_MAXNUMBER
	#define DOMAINLOGGER_DOMAINS_MAXNUMBER 60
#endif

#ifndef DOMAINLOGGER_APPLICATION_NAME_MAX_LENGTH
	#define DOMAINLOGGER_APPLICATION_NAME_MAX_LENGTH 40
#endif

#if ( DL_PLATFORM_IS_WIN32 == 1 )

	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <wchar.h>

#elif( DL_PLATFORM_IS_UNIX == 1 )

	#include <pthread.h>
	#include <unistd.h>

	#if ( DL_PLATFORM_IS_DARWIN == 1 )
	#include "TargetConditionals.h"
	#endif
	
	#define DL_PLATFORM_IS_SPINLOCKS_MUTEXS 1

#else

	#error we have unknown platform.

#endif

/** Your normal set of micro to stop things going on about things */
#define DLOG_UNUSED( x ) ( void )( x )

#if( DL_PLATFORM_IS_WIN32 == 1 )

	#if( DL_PLATFORM_NO_CLIB == 0 )

		#include <stdint.h>
		#include <stdio.h>
		#include <stdlib.h>
		#include <time.h>

		/** The idea is that the Win32 version could be used without the C/C++ runtime but for development
		we keep the C base stuff around using SUPPORT_CLIB_WIN32 */
		extern struct tm* localtime_r( time_t *clock, struct tm *result );
		extern struct tm* gmtime_r( time_t *clock, struct tm *result );

		#define snprintf _snprintf
		#define vsnprintf _vsnprintf
		#define strcasecmp _stricmp
		#define strncasecmp _strnicmp

	#else

		#include "Shlwapi.h"
		#include "Strsafe.h"

		/* There are a few things we need to define */
		typedef char int8_t;
		typedef short int16_t;
		typedef int int32_t;
		typedef long long int64_t;
		typedef unsigned char uint8_t;
		typedef unsigned short uint16_t;
		typedef unsigned int uint32_t;
		typedef unsigned long long uint64_t;

		#define strcasecmp lstrcmpiA
		#define strncpy lstrcpynA
		#define strlen lstrlenA
		
		/** MSDN says floating point is not supported? */
		#define snprintf StringCchPrintf

	#endif

	extern int LogConvertCharToWChar( wchar_t *to, size_t toBufferSize, const char *from );
	extern int LogConvertWCharToChar( char *to, size_t toBufferSize, const wchar_t *from );

	#define LogAtomicExchangePointer InterlockedExchangePointer
	#define LogAtomicCompareExchangePointer( _target, _with, _compairedTo ) InterlockedCompareExchangePointer( _target, _with, _compairedTo )
	#define LogAtomicCompInt32( _target, _with ) ( InterlockedAdd( _target, 0 ) == _with )
	#define LogAtomicIncInt32 InterlockedIncrement 
	#define LogAtomicDecInt32 InterlockedDecrement
	#define LogAtomicSetInt32 InterlockedExchange

	#define LogThreadYeild SwitchToThread
	#define LogThreadSleepSeconds( _secs ) Sleep( _secs * 1000 )

	#define LogMemoryFullBarrier _ReadWriteBarrier

	typedef DWORD ThreadId;
	typedef volatile LONG int32_atomic;
	typedef int32_atomic LogSpinLock;

#else

	#include <stdint.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <time.h>

	#include <unistd.h>
	#include <limits.h>
	#include <errno.h>
	#include <sched.h>
	#include <pthread.h>
	#include <sys/stat.h>
	#include <sys/types.h> 
	#include <sys/stat.h> 

	// TODO FreeBSD see atomic.h atomic_cmpset_char

	#if( DL_PLATFORM_IS_DARWIN == 1 )
		#include <mach/mach.h>
		#include <mach/mach_time.h>
		#include <mach/clock.h>	
		#include <CoreServices/CoreServices.h>
		#include <libkern/OSAtomic.h>

	#endif

  extern void* __Log_GCC_SwapPointer( void* volatile* pTarget, void *pWith );

	#define LogAtomicExchangePointer( _target, _with ) __Log_GCC_SwapPointer( _target, _with )
	#define LogAtomicCompareExchangePointer( _target, _with, _compairedTo )	 __sync_val_compare_and_swap( _target, _compairedTo, _with )

	#define LogThreadYeild sched_yield
	#define LogThreadSleepSeconds sleep


  typedef volatile long int32_atomic;

#if( DL_PLATFORM_IS_SPINLOCKS_MUTEXS == 1 )
  typedef pthread_mutex_t LogSpinLock;
#else
  /** The spin lock type */
  typedef int32_atomic LogSpinLock;
#endif

#if( DL_PLATFORM_IS_DARWIN == 1 )

  #define LogAtomicIncInt32( _t ) OSAtomicIncrement32Barrier( _t )
  #define LogAtomicDecInt32( _t ) OSAtomicDecrement32Barrier( _t )
  #define LogAtomicSetInt32( _t, _v ) OSAtomicCompareAndSwap32Barrier( ( volatile int32_t* )*_t, _v, ( volatile int32_t* )_t )

  #define LogAtomicCompInt32( _target, _with ) ( __sync_fetch_and_add( _target, 0 ) == _with )

  #define LogMemoryFullBarrier OSMemoryBarrier

#else

  #define LogAtomicIncInt32( _t ) __sync_add_and_fetch( _t, 1 )
  #define LogAtomicDecInt32( _t ) __sync_sub_and_fetch( _t, 1 )
  #define LogAtomicSetInt32( _t, _v ) __sync_lock_test_and_set( _t, _v )

  #define LogAtomicCompInt32( _target, _with ) ( __sync_fetch_and_add( _target, 0 ) == _with )

  #define LogMemoryFullBarrier __sync_synchronize

#endif

  #define MAX_PATH 260

	typedef pid_t ThreadId;

#endif

#include "domainlogger/domainlogger.h"

/** Call this to report something very very very bad happened in the logging system 
*/
extern void reportFatal( const char *msg );

/** Used for memory allocation. 
* As we want try and keep away from the C runtime on win32 we use are own mem functions 
*/
extern void *LogMemoryAlloc( size_t sizeInBytes );
extern void *LogMemoryCopy( void *to, void *from, size_t size );
extern void LogMemoryFree( void *pData );
extern void LogMemoryZero( void *pData, size_t numberOfBytesToWipe );

/** Used to test for a directory and if needs be create it.
* This does a mkdir -p type thing
*
* \param path - Where!
* \return 1 if exist or create or 0 if there is a big problem.
*/
extern int LogValidateDirectory( const char *path );

extern void LogSpinLockCreate( LogSpinLock *lock );
/* File system stuff 
* If the function is wchar_t then it only works on Windows.  If you don't know Windows  (post 200
*/
extern int LogCreateDirectoryTreeWChar( const wchar_t *path );
extern int LogCreateDirectoryTreeChar( const char *path );

/** Try to capture the spinlock 
* \return 1 for captured or 0 for not
*/
extern int LogSpinLockTryCapture( LogSpinLock *lock );
extern void LogSpinLockCapture( LogSpinLock *lock );
extern void LogSpinLockRelease( LogSpinLock *lock );
extern void LogSpinLockDestroy( LogSpinLock *lock );

/** The struct that defines a block of log messages 
* Note for future stuff.
* We keep a pointer to first and last message in the block. I don't know when I'll get around to it but 
* if we want to take this block out of action we can filter them out when they are returned to the main store e.g. is returnedMsg = ( >= firstMessage and <= lastMessage )
*/
typedef struct _LogMessageBlock
{
	struct _LogMessageBlock *next;

	// The number in this block
	uint32_t number;
	// Used when we want to release this block
	uint32_t numberReturned;

	// The first log message object do ++ to the next one if you want.
	LogMessage *firstMessage;
	// we keep the last one around as well
	LogMessage *lastMessage; 

} LogMessageBlock;


/** The message queue object 
* We don't use a spinlock/mutex etc etc but you the user should protect it!
*/
typedef struct 
{
	/// Protection.
	LogSpinLock protection;

	int32_atomic numberIn;

	LogMessage *head;
	LogMessage *tail;
} LogMessageQueue;

extern int LogMessageQueueCreate( LogMessageQueue *queue );
extern LogMessage *LogMessageQueuePop( LogMessageQueue *queue );
extern LogMessage *LogMessageQueuePopChain( LogMessageQueue *queue, uint32_t maxNumberOf, uint32_t *numberReturned, LogMessage **pEnd );

extern void LogMessageQueuePush( LogMessageQueue *queue, LogMessage *item );
extern void LogMessageQueuePushChain( LogMessageQueue *queue, LogMessage *chainHead, LogMessage *chainTail, uint32_t numberInChain ); 
extern int LogMessageQueueDestroy( LogMessageQueue *theQueue );

/** Used to get free log message to be used */
extern LogMessage* DomainLoggerClientGetFreeMessage();

/** Used to return free log message back to the free queue */
extern void DomainLoggerClientReturnFreeMessage( LogMessage *msg );

/* Used to create a block of log message objects 
* We create these in block to keep mem footprint down.  The block is created and all log message objects are chained together, head and tail are returned to the call so it's plainless to add into a queue
* 
* \param numberToCreate Need I say
* \return Pointer to a LogMessageBlock on the heap which you now own or NULL if there was a problem.
*/
extern LogMessageBlock* LogMessageCreateBlock();
/** Use to delete a block
* \param theBlock The block to delete.
* \return Pointer to the next block or NULL
*/
extern LogMessageBlock *LogMessageDestroyBlock( LogMessageBlock *theBlock );

typedef struct 
{
	char *domain;
	size_t domainLength;
	DomainLoggingLevels level;
} DomainLoggerClientDomainInfo;

typedef struct
{
	int year, month, day;
	int hours, minutes, seconds;
} DomainLoggerDateTime;

/** Misc utility functions */

/** Used to convert logging levels as strings into enums.
* If all else failes warning is returned.
*/
extern DomainLoggingLevels DomainLoggerReadLevelFromString( const char *str );

/** Used to return a string of a give logging level.
* You DO NOT OWN the returned string
*/
extern const char *DomainLoggingGetStringForLevel( DomainLoggingLevels level );

/*** Thread stuff */

/** Thread States */
extern const int32_t LogThreadStateStopped;
extern const int32_t LogThreadStateStarting;
extern const int32_t LogThreadStateRunning;
extern const int32_t LogThreadStateStopping;

/** The thread struct */
typedef struct _LogThread
{
	int32_atomic state;

#if( DL_PLATFORM_IS_WIN32 == 1 )
	HANDLE hThread;
	ThreadId threadId;
#else
	pthread_attr_t threadAttributes;
	pthread_t hThread;
#endif

	/** The users thread entry point */
	void *pThreadFunctionUserData;
	void ( *threadFunction )( void *pUserData );

} LogThread;

/** Use to create a new thread object & set the users thread function 
*/
int LogThreadCreate( LogThread *pTheThread, void (*threadFunction )( void *pUserData ), void *pThreadFunctionUserData );

/** Start the thread running 
*/
int LogThreadStart( LogThread *pTheThread );

/** Use to test if the thread has started or not
* \return 1 if started
*/
int LogThreadRunning( LogThread *pTheThread );

/** Used to test if a thread has stopped 
* \return 1 if stopped 
*/
int LogThreadStopped( LogThread *pTheThread );

/** We don't have a stop thread function, if you want your threads to end you should be managing that at a higher level.  Saying that 
* we do have a kill function.  You might want a thread DEAD
* Use this only if you have as it KILLS the thread and do you want the repitation as a mass thread killer?
*/
int LogThreadKill( LogThread *pTheThread );

/** Use to return the threadId of the current thread 
* It turns out the UNIX platforms are all different!!!
*/
ThreadId LogThreadCurrentThreadId();

typedef struct _LogThreadEvent
{
#if( DL_PLATFORM_IS_WIN32 == 1 )
	HANDLE hEvent;
#endif

#if( DL_PLATFORM_IS_UNIX == 1 )

	int32_t created;
	pthread_mutex_t hMutex;
	pthread_cond_t hCondition;

#endif

} LogThreadEvent;

int LogThreadEventCreate( LogThreadEvent *pEvent );

int LogThreadEventTrap( LogThreadEvent *pEvent );

int LogThreadEventWait( LogThreadEvent *pEvent, int *isTimeout );

void LogThreadEventDestroy( LogThreadEvent *pEvent );

#endif /*  __DOMAIN_LOGGER_COMMON_H__ */