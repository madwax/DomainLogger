/** Domain Logger - A hopefully lightweight-ish logging frame work.
*
* Q) Why write another logging framework?
* A) I want my logging system to do 4 things 1) Log, 2) At runtime allow be change logging levels, 3) be able to do this but only in defined areas of the codebase & 4) not depend on anything other than std c/std platform APIs. No one seams to do this (or not without being every big/dependant on other stuff)
*
* When you log you log against a) A domain and b) A level.    A domain defines the area of code this logging is taking place in. A Level is anything between Fatal to verbose
* 
* How to use: 
*   In your app main()/WinMain()/startup:
*
*			// Start up logging
*			if( ( er = DomainLoggerOpen( argc, arv, "MyApplicationIs...", "./log/Path/Is" ) ) == 0 )
*			{
*				printf( "Failed to create logging system ..." );
*				abort()/exit()/return ?;
*			}
*
*			// Shutdown logging - This might wait for anything worker threads to finish
*			DomainLoggerClose()
*
*
*		In you app at some point:
*
*		  const char *MyFirstLoggingDomain = "Domain.Fred";
*
*     // Need to log trouble...
*			LogError( MyFirstLoggingDomain, "Something bad happened! x=%d", x );
*
*
* Some things to be mind full of:
*		1) For speed the logger does not parse the domain string passed to LogX()/GetLoggingLevel() and only does internal stuff based on the pointer. This means if you have to different string pointers for a domain things are going slow down.
*			Best idea is to define a const char * and use that.
*		2) The logger will create its own thread.  When you log you create an object with is then processed in the logging thread. This means logging has minimal impact on normal code, try write to stdout from 2 threads at the same time.
*		3) Use DomainLoggerGetLevel() or DomainLoggerTest() to test the current logging level for a domain if you have a block of logging. This again help the impact low on normal code paths 
*		4) The max number of chars the message you log is set to 1024.
*
*/
#ifndef __DOMAIN_LOGGER_H__
#define __DOMAIN_LOGGER_H__

#include <stdint.h>
#include <time.h>
#include <wchar.h>

#if( DL_PLATFORM_IS_WIN32 == 1 )
	#include <Windows.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	/* When something very bad happens */
	DomainLoggingLevelFatal = 0,
	DomainLoggingLevelError,
	/* If you want something in the log no matter what but it's not an error/warning e.g. X has started type messages*/
	DomainLoggingLevelSystem,
	DomainLoggingLevelWarning,
	/* Something a power user, system interator or devel might want to know about */
	DomainLoggingLevelInfo,
	/* Some dev has broken something and they want to know what going on inside the mind of the computer */
	DomainLoggingLevelDebug,
	/* Your just being silly now, all else has failed */
	DomainLoggingLevelVerbose
} DomainLoggingLevels;


#ifndef DOMAINLOGGER_MESSAGETEXTSIZE 
	#define DOMAINLOGGER_MESSAGETEXTSIZE 256
#endif 


/** The log event object.
* Its here with DomainLoggerSink so if you want to write your own sink 
*/
typedef struct _LogMessage
{
	char msg[ DOMAINLOGGER_MESSAGETEXTSIZE ];
	/* The next item in the queue */
	struct _LogMessage *next;
	/* Points to the domain name */
	char *lpDomain;
	/* When it happened We do this as number of miliseconds from startup */
	uint64_t when;
	/** Points to the file name */
	char *fileName;
	/** Points to the function name */
	char *functionName;
	/** The source file line number were the message is from */
	uint32_t lineNumber;
	/* the level */
	DomainLoggingLevels level;
	/** The threadId where the message came from */
	uint32_t threadId;
	/** The length of the formated message */
	uint16_t msgLength;
} LogMessage;


/** All messages are sent to a sink object
* This structure is used as the interface for every sink.
*/
typedef struct _DomainLogSinkInterface
{
	/** For you todo with as you please */
	void *data; 
	/** Is enabled or not
	*/
	uint32_t enabled;
	
	/** Callback called when the sink is inited in the log thread. 
	* e.g. You will be running in the loggers worker thread.
	*/
	void ( *OnLoggingThreadInitCb )( struct _DomainLogSinkInterface *pLogSink );
	
	/** Callback called when the sink needs to process a message in the loggers worker thread
	* 
	* \parma pLogSink Pointer to your sink object.
	* \param pMsg The log message to process.
	*
	* You DO NOT OWN pMsg AT ALL.  After this callback is called pMsg will be passed onto the next sink in the chain.
	*/
	void ( *OnLoggingThreadOnProcessMessageCb )( struct _DomainLogSinkInterface *pLogSink, LogMessage *pMsg );

	/** Callback called when the logging thread is shutdown, this is called from the loggers worker thread.
	*  
	*/
	void ( *OnLoggingThreadClosedCb )( struct _DomainLogSinkInterface *pLogSink );

	/** Callback called when the logging engine has shutdown.  This is called from the thread (normaly the main thread) which called DomainLoggerClose()
	* The idea is you delete your sink here
	*/
	void ( *OnLoggingShutdownCb )( struct _DomainLogSinkInterface *pLogSink );
} DomainLogSinkInterface;

/** The default Console/Debugger Logging Sink */
/** @{ */

/** No output at all */
#define DomainLoggerConsoleOutputDisabled 0
/** A monocoloured console output */
#define DomainLoggerConsoleOutputMono 1
/** Prints at least parts of log message out using colours */
#define	DomainLoggerConsoleOutputColoured 2
/** Send log messages to the debuggers output window.
* Under Windows the message is send via OutputDebugString
* Under Darwin the message is send via NSLog
* Under Linux to console
*/
#define	DomainLoggerConsoleOutputDebugger 3

/** Turn on, off console logging 
* See DomainLoggerConsoleOutput
*/
void DomainLoggerConsoleEnable( int consoleOutputFlags );

/** @} */

/** The Single File Logging Sink */
/** @{ */

/** Set the dir were logs are to be written to.
*/
void DomainLoggerFileSet( const char *dirPath, const char *prefix );

/** Set the dir were logs are to be written to. wchar_t version.
*/
void DomainLoggerFileSetW( const wchar_t *dirPath, const char *prefix );

/** Used to see if we have set a dir were log files are to go.
*/
int DomainLoggerFileDirSet();

/** @} */


/** Used to add a sink interface to the logging system.
* WARNING: This is not thread protected so always call this before you call DomainLoggerStart() else bad things might happen.
*
* \return 0=good 1=bad
*/
int DomainLoggerAddSink( DomainLogSinkInterface *theSink );

/** Set the default logging level for all domains as they appear to the logger
*/
void DomainLoggerSetDefaultLevel( DomainLoggingLevels newDefaultLevel );

/** Sets the logging level for a given domain or domains.
* domain The logging domain to use. You can use whild cards to get a range of domians e.g.   'my.app.system.*' or 'common.libA*'
*/
void DomainLoggerSetLevelToDomain( const char *domain, DomainLoggingLevels loggingLevel );

/** Starts the logger running 
*
* The application identifier is used to identify the instance of you application.  
* E.g. If you write app which acts as a server or a commandline tool you would use the same application name but different identifiers ( e.g. name:"MyService" (service:id):"Service", (cli:id):"CLI" 
* You don't have to supply an app name and/or app identifier (pass NULL) but some logging backends might not work without it.
*/
int DomainLoggerStart( const char *applicationName, const char *applicationIdetifier );

/** Used to get a logging level from a string
*
*/
DomainLoggingLevels DomainLoggerLevelIs( const char *whatLevel );

/** Use this to get the current logging level of a domain
*/
DomainLoggingLevels DomainLoggerGetLevel( const char *whichDomain );

/** Use to see if the domain you want to know about is at or high logging level that that you passed in 
*/
int DomainLoggerTest( const char *whichDomain, DomainLoggingLevels toTestIfUsable );

/** Does the hard work. Best not to use this directly, use the following LogX micros
*/
void DomainLoggerPost( const char *whichDomain, DomainLoggingLevels underLevel, const char *filename, int lineNumber, const char *functionName, const char *msg, ... );

/** Use to shut down the logging system 
*/
int DomainLoggerClose();

/** Use these micros!!! */
/* Note To Test! gcc had msg... but msvc wants msg, ... don't know if it's a compiler thing */
  
#if defined( _MSC_VER )

  #define LogFatal( domain, msg, ... ) DomainLoggerPost( domain, DomainLoggingLevelFatal, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogError( domain, msg, ... ) DomainLoggerPost( domain, DomainLoggingLevelError, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogSystem( domain, msg, ... ) DomainLoggerPost( domain, DomainLoggingLevelSystem, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogWarning( domain, msg, ... ) DomainLoggerPost( domain, DomainLoggingLevelWarning, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogInfo( domain, msg, ... ) DomainLoggerPost( domain, DomainLoggingLevelInfo, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogDebug( domain, msg, ... ) DomainLoggerPost( domain, DomainLoggingLevelDebug, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogVerbose( domain, msg, ... ) DomainLoggerPost( domain, DomainLoggingLevelVerbose, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )

#else

  #define LogFatal( domain, msg... ) DomainLoggerPost( domain, DomainLoggingLevelFatal, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogError( domain, msg... ) DomainLoggerPost( domain, DomainLoggingLevelError, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogSystem( domain, msg... ) DomainLoggerPost( domain, DomainLoggingLevelSystem, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogWarning( domain, msg... ) DomainLoggerPost( domain, DomainLoggingLevelWarning, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogInfo( domain, msg... ) DomainLoggerPost( domain, DomainLoggingLevelInfo, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogDebug( domain, msg... ) DomainLoggerPost( domain, DomainLoggingLevelDebug, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogVerbose( domain, msg... ) DomainLoggerPost( domain, DomainLoggingLevelVerbose, __FILE__, __LINE__, __FUNCTION__, ##msg )

#endif

#ifdef __cplusplus
}
#endif

#endif /* __DOMAIN_LOGGER_H__ */

