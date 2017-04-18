#ifndef __DOMAIN_LOGGER_SINK_FILE_H__
#define __DOMAIN_LOGGER_SINK_FILE_H__

#include "logger.common.h"

/** The Log file sink
* You application wants one log file which is created on startup and used for the lifetime of you appllication.
*
*/

#ifndef DOMAINLOGGER_SINK_FILE_INTERFACE_OUTPUT_BUFFER_SIZE 

	#define DOMAINLOGGER_SINK_FILE_INTERFACE_OUTPUT_BUFFER_SIZE 1024

#endif

#ifndef DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_PATH_LENGTH 

	#define DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_PATH_LENGTH 512

#endif

#ifndef DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH 

	#define DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH ( DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_PATH_LENGTH - 200 )

#endif

#if( DL_PLATFORM_IS_WIN32 == 1 )

	#define DL_PLATFORM_FILE_WIN32 1
	#define DL_PLATFORM_FILE_POSIX 0

#else

	#define DL_PLATFORM_FILE_WIN32 0
	#define DL_PLATFORM_FILE_POSIX 1

#endif

typedef struct _DomainLogSinkFileInterface
{
	DomainLogSinkInterface base;

	/// The where log files are to live.
	char loggingPath[ DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH ];

	/// The output buffer
	char outputBuffer[ DOMAINLOGGER_SINK_FILE_INTERFACE_OUTPUT_BUFFER_SIZE ];

	/// When the log file was openened.


#if( DL_PLATFORM_FILE_WIN32 == 1 )
	
	HANDLE hCurrentLogFile;
	
#elif( DL_PLATFORM_FILE_POSIX == 1 )

	int hCurrentLogFile;

#endif

} DomainLogSinkFileInterface;


extern DomainLogSinkInterface* DomainLoggerFileSinkCreate( void );

extern void DomainLoggerFileSinkSetPath( DomainLogSinkInterface *pTheSink, const char *dirPath, const char *prefix );
extern void DomainLoggerFileSinkSetPathW( DomainLogSinkInterface *pTheSink, const wchar_t *dirPath, const char *prefix );
extern int DomainLoggerFileSinkPathSet( DomainLogSinkInterface *pTheSink );


#endif /*  __DOMAIN_LOGGER_SINK_FILE_H__ */ 