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

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	/* When something very bad happens */
	LogLevelFatal = 0,
	LogLevelError,
	/* If you want something in the log no matter what but it's not an error/warning e.g. X has started type messages*/
	LogLevelSystem,
	LogLevelWarning,
	/* Something a power user, system interator or devel might want to know about */
	LogLevelInfo,
	/* Some dev has broken something and they want to know what going on inside the mind of the computer */
	LogLevelDebug,
	/* Your just being silly now, all else has failed */
	LogLevelVerbose
} DomainLoggingLevels;

#if 0
/** Used before you call DomainLoggerOpen() to tell the logging engine were to put files.
* If you don't call this and don't disable file logging your log files should be found:
*
* On Windows: Users local apps dir e.g. c:\Users\fred\AppData\Local\%AppName%\logs
* On Mac: ~/Library/Application Support/%AppName%/logs
* On iOS: 'Your applications documents folder\getenv("HOME")/Documents'/logs
* On Linux/FreeBSD/... /var/tmp/%AppName%/logs
*
* The log files will be formatted 'YYMMDD-HHMMSS-%AppName%.log'
*/
int DomainLoggerPreFileLoggingOveridePath( const char *pathToLogFiles );
#endif

/** Used before you call DomainLoggerOpen() if you want to turn off file based logging (Why I know...) 
* If you pass --log.file it will be enabled
*/
int DomainLoggerPreFileLoggingDisabled();

/** Used to set the default logging level */
int DomainLoggerPreSetDefaultLevel( DomainLoggingLevels newDefaultLevel );

/** Used before you call DomainLoggerOpen() to turn on console logging
* Using the command line arg --log.console will enable but this allows you to force it
* 
* \param enableColourOutput  If the console can kick out coloured text then do so. 
*/
int DomainLoggerPreConsoleLoggingEnable( int enableColourOutput );


/** Use to init the logging system.
* See DomainLoggerPrintCommandLineArgs() for what can be passed
*
* \param argc
* \param argv
* \param applicationName The name of your application
* \return 1 if everything started without any problems.
*/
int DomainLoggerOpen( int argc, char **argv, const char *applicaitonName );

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
  
#if( _WIN32 )
  #define LogFatal( domain, msg, ... ) DomainLoggerPost( domain, LogLevelFatal, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogError( domain, msg, ... ) DomainLoggerPost( domain, LogLevelError, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogSystem( domain, msg, ... ) DomainLoggerPost( domain, LogLevelSystem, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogWarning( domain, msg, ... ) DomainLoggerPost( domain, LogLevelWarning, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogInfo( domain, msg, ... ) DomainLoggerPost( domain, LogLevelInfo, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogDebug( domain, msg, ... ) DomainLoggerPost( domain, LogLevelDebug, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
  #define LogVerbose( domain, msg, ... ) DomainLoggerPost( domain, LogLevelVerbose, __FILE__, __LINE__, __FUNCTION__, msg, __VA_ARGS__ )
#else
  #define LogFatal( domain, msg... ) DomainLoggerPost( domain, LogLevelFatal, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogError( domain, msg... ) DomainLoggerPost( domain, LogLevelError, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogSystem( domain, msg... ) DomainLoggerPost( domain, LogLevelSystem, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogWarning( domain, msg... ) DomainLoggerPost( domain, LogLevelWarning, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogInfo( domain, msg... ) DomainLoggerPost( domain, LogLevelInfo, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogDebug( domain, msg... ) DomainLoggerPost( domain, LogLevelDebug, __FILE__, __LINE__, __FUNCTION__, ##msg )
  #define LogVerbose( domain, msg... ) DomainLoggerPost( domain, LogLevelVerbose, __FILE__, __LINE__, __FUNCTION__, ##msg )
#endif
  
  
#ifdef __cplusplus
}
#endif

#endif /* __DOMAIN_LOGGER_H__ */

