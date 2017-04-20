#include "logger.sink.file.h"
#include "logger.client.h"


static void DomainLoggerFileSinkThreadProcessMsg( DomainLogSinkInterface *pSink, LogMessage *pMsg );
static void DomainLoggerFileSinkThreadInit( DomainLogSinkInterface *theSink );
static void DomainLoggerFileSinkThreadClose( DomainLogSinkInterface *theSink );
static void DomainLoggerFileSinkDestroy( DomainLogSinkInterface *theSink );

static void DomainLogSinkFileInterfaceLogOpen( DomainLogSinkFileInterface *pTheSink );
static void DomainLogSinkFileInterfaceLogRender( DomainLogSinkFileInterface *pTheFilkeSink, LogMessage *pTheLogMessage );
static void DomainLogSinkFileInterfaceLogClose( DomainLogSinkFileInterface *pTheSink );


DomainLogSinkInterface* DomainLoggerFileSinkCreate( void )
{
	DomainLogSinkFileInterface *r;
	size_t sz;

	sz = sizeof( DomainLogSinkFileInterface );

	r = ( DomainLogSinkFileInterface* )LogMemoryAlloc( sz );
	if( r == NULL )
	{
		return NULL;
	}	

	LogMemoryZero( r, sz );

	r->base.enabled = 0;
	r->base.data = r;

	r->loggingPath[ 0 ] = 0;

	r->base.OnLoggingShutdownCb = &DomainLoggerFileSinkDestroy;
	r->base.OnLoggingThreadClosedCb = &DomainLoggerFileSinkThreadClose;
	r->base.OnLoggingThreadInitCb = &DomainLoggerFileSinkThreadInit;
	r->base.OnLoggingThreadOnProcessMessageCb = &DomainLoggerFileSinkThreadProcessMsg;

#if( DL_PLATFORM_FILE_WIN32 == 1 )

	r->hCurrentLogFile = INVALID_HANDLE_VALUE;

#elif( DL_PLATFORM_FILE_POSIX == 1 )

	r->hCurrentLogFile = -1;

#endif
	return &r->base;
}

static void DomainLoggerFileSinkThreadInit( DomainLogSinkInterface *pTheSink )
{
	DomainLogSinkFileInterface *pTheFileSink;
	
	pTheFileSink = ( DomainLogSinkFileInterface* )pTheSink;

	if( pTheFileSink->base.enabled == 1 )
	{
		// we need to open the file.
	}
}


static void DomainLoggerFileSinkThreadProcessMsg( DomainLogSinkInterface *pTheSink, LogMessage *pTheMsg )
{
	DomainLogSinkFileInterface *pTheFileSink;
	
	pTheFileSink = ( DomainLogSinkFileInterface* )pTheSink;

#if( DL_PLATFORM_FILE_WIN32 == 1 )
	if( pTheFileSink->hCurrentLogFile == INVALID_HANDLE_VALUE )
#elif( DL_PLATFORM_FILE_POSIX == 1 )
	if( pTheFileSink->hCurrentLogFile == -1 )
#endif
	{
		if( pTheFileSink->base.enabled == 1 )
		{
			// the log file was enabled after logging was started so we need to open the file.
			DomainLogSinkFileInterfaceLogOpen( pTheFileSink );
			DomainLogSinkFileInterfaceLogRender( pTheFileSink, pTheMsg );
		}
	}
	else
	{
		if( pTheFileSink->base.enabled == 0 )
		{
			// someone wants the log file closed.
			DomainLogSinkFileInterfaceLogClose( pTheFileSink );
		}
		else
		{
			DomainLogSinkFileInterfaceLogRender( pTheFileSink, pTheMsg );
		}
	}
}


static void DomainLoggerFileSinkThreadClose( DomainLogSinkInterface *pTheSink )
{
	DomainLogSinkFileInterface *pTheFileSink;
	
	pTheFileSink = ( DomainLogSinkFileInterface* )pTheSink;

	DomainLogSinkFileInterfaceLogClose( pTheFileSink );
}


static int DomainLogSinkFileInterfaceLogWrite( DomainLogSinkFileInterface *pTheFilkeSink, char *data, uint32_t numberOfBytes )
{
#if( DL_PLATFORM_FILE_WIN32 == 1 )

	DWORD bytesWritten;
	char *d;

	bytesWritten = 0;
	d = data;

	while( numberOfBytes ) 
	{
		if( WriteFile( pTheFilkeSink->hCurrentLogFile, d, numberOfBytes, &bytesWritten, NULL ) == 0 )
		{
			return 1;
		}
		else
		{
			d += bytesWritten;
			numberOfBytes -= bytesWritten;
		}
	}

	FlushFileBuffers( pTheFilkeSink->hCurrentLogFile );

#elif(  DL_PLATFORM_FILE_POSIX == 1 )

	ssize_t bytesWritten;
	char *d;

	d = data;
	while( numberOfBytes ) 
	{
		if( ( bytesWritten = write( pTheFilkeSink->hCurrentLogFile, ( const void* )d, numberOfBytes ) ) == -1 )
		{
			return 1;
		}
		else
		{
			d += bytesWritten;
			numberOfBytes -= bytesWritten;
		}
	}

	fsync( pTheFilkeSink->hCurrentLogFile );
#endif	

	return 0;
}


static void DomainLogSinkFileInterfaceLogRender( DomainLogSinkFileInterface *pTheFilkeSink, LogMessage *pTheLogMessage )
{
	static char *theLevels[] = { "Fatal","Error","System","Warning","Info","Debug","Verbose" };

#if( DL_PLATFORM_FILE_WIN32 == 1 )

	if( pTheFilkeSink->hCurrentLogFile == INVALID_HANDLE_VALUE )
	{
		return;
	}
#elif( DL_PLATFORM_FILE_POSIX == 1 )
	if( pTheFilkeSink->hCurrentLogFile == -1 )
	{
		return;
	}
#endif
	else
	{
		uint64_t delta;
		uint32_t numberOf;
		uint32_t hours, mins, secs, mils;

		hours = mins = secs =  mils = 0;

		delta = pTheLogMessage->when - DomainLoggerClientGet()->whenEpoc;

		if( delta != 0 )
		{
			mils = ( uint32_t )( delta % 1000 );
			secs = ( uint32_t )( ( delta / 1000 ) % 60 );
			mins = ( uint32_t )( ( delta / ( 1000 * 60 ) ) % 60 );
			hours = ( uint32_t )( delta / ( 1000 * 60 * 60 ) );
		}
				
		numberOf = snprintf( pTheFilkeSink->outputBuffer, DOMAINLOGGER_SINK_FILE_INTERFACE_OUTPUT_BUFFER_SIZE, "%s~%d:%02d:%02d:%04d~%s~%d~'%s'\n", theLevels[ pTheLogMessage->level ], hours, mins, secs, mils, pTheLogMessage->lpDomain, pTheLogMessage->threadId, pTheLogMessage->msg );

		DomainLogSinkFileInterfaceLogWrite( pTheFilkeSink, pTheFilkeSink->outputBuffer, numberOf );
	}
}


static void DomainLogSinkFileInterfaceLogOpen( DomainLogSinkFileInterface *pTheSink )
{
	if( !pTheSink->base.enabled )
	{
		// not enabled
		return;
	}

	if( pTheSink->loggingPath[ 0 ] == 0 )
	{
		// no path set.
		return;
	}

#if( DL_PLATFORM_FILE_WIN32 == 1 )

	if( pTheSink->hCurrentLogFile == INVALID_HANDLE_VALUE )
	{
		wchar_t bu[ 1024 ];
		LogMemoryZero( bu, ( 1024 * sizeof( wchar_t ) ) );

		LogConvertCharToWChar( bu, 1024, pTheSink->loggingPath );

		pTheSink->hCurrentLogFile = CreateFileW( bu, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	}

#elif( DL_PLATFORM_FILE_POSIX == 1 )

	if( pTheSink->hCurrentLogFile == -1 )
	{
		pTheSink->hCurrentLogFile = open( pTheSink->loggingPath, O_CREAT|O_APPEND|O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
	}
#endif

	{
		uint32_t numberOf;
		DomainLoggerClient *pTheClient = DomainLoggerClientGet();

		LogMemoryZero( pTheSink->outputBuffer, 256 );

		numberOf = snprintf( pTheSink->outputBuffer, 256, "#Application:%s\n", pTheClient->theApplicationName );
		DomainLogSinkFileInterfaceLogWrite( pTheSink, pTheSink->outputBuffer, numberOf );

		if( pTheClient->theApplicationIdentifier != NULL )
		{
			numberOf = snprintf( pTheSink->outputBuffer, 256, "#ApplicationIdentifier:%s", pTheClient->theApplicationName );
			DomainLogSinkFileInterfaceLogWrite( pTheSink, pTheSink->outputBuffer, numberOf );
		}

		numberOf = snprintf( pTheSink->outputBuffer, 256, "#OpenedUTC:%04d/%02d/%02d %02d:%02d:%02d\n", pTheClient->createdOnUTC.year, pTheClient->createdOnUTC.month, pTheClient->createdOnUTC.day, pTheClient->createdOnUTC.hours, pTheClient->createdOnUTC.minutes, pTheClient->createdOnUTC.seconds );
		DomainLogSinkFileInterfaceLogWrite( pTheSink, pTheSink->outputBuffer, numberOf );

		numberOf = snprintf( pTheSink->outputBuffer, 256, "#OpenedLocal:%04d/%02d/%02d %02d:%02d:%02d\n", pTheClient->createdOnLocal.year, pTheClient->createdOnLocal.month, pTheClient->createdOnLocal.day, pTheClient->createdOnLocal.hours, pTheClient->createdOnLocal.minutes, pTheClient->createdOnLocal.seconds );
		DomainLogSinkFileInterfaceLogWrite( pTheSink, pTheSink->outputBuffer, numberOf );
	
		/// TODO RHC need toget the process id.
		numberOf = snprintf( pTheSink->outputBuffer, 256, "#ProcessId %d\n", 0 );
		DomainLogSinkFileInterfaceLogWrite( pTheSink, pTheSink->outputBuffer, numberOf );
	}
}


static void DomainLogSinkFileInterfaceLogClose( DomainLogSinkFileInterface *pTheSink )
{
#if( DL_PLATFORM_FILE_WIN32 == 1 )

	if( pTheSink->hCurrentLogFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle( pTheSink->hCurrentLogFile );
		pTheSink->hCurrentLogFile = INVALID_HANDLE_VALUE;
	}

#elif( DL_PLATFORM_FILE_POSIX == 1 )

	if( pTheSink->hCurrentLogFile != -1 )
	{
		close( pTheSink->hCurrentLogFile );
		pTheSink->hCurrentLogFile = -1;
	}

#endif
}


static void DomainLoggerFileSinkBuildAndSetFilePath( DomainLogSinkFileInterface *pTheSink, const char *path, const char *prefix )
{
	DomainLoggerClient *pTheClient = DomainLoggerClientGet();

	snprintf( pTheSink->loggingPath, DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH, "%s/%s-%04d%02d%02d-%02d%02d%02d.log", path, prefix, pTheClient->createdOnUTC.year, pTheClient->createdOnUTC.month, pTheClient->createdOnUTC.day, pTheClient->createdOnUTC.hours, pTheClient->createdOnUTC.minutes, pTheClient->createdOnUTC.seconds );

#if( DL_PLATFORM_FILE_WIN32 == 1 )
	
	wchar_t tmp[ DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH ];
	LogMemoryZero( tmp, DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH );

	LogConvertCharToWChar( tmp, DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH, path );

	LogCreateDirectoryTreeWChar( tmp );

#elif( DL_PLATFORM_FILE_POSIX == 1 )

	LogCreateDirectoryTreeChar( path );

#else
#error missing
#endif
}


void DomainLoggerFileSinkSetPathW( DomainLogSinkInterface *pTheSink, const wchar_t *dirPath, const char *prefix )
{
#if( DL_PLATFORM_FILE_WIN32 == 1 )
	DomainLogSinkFileInterface *pTheFileSink;
	pTheFileSink = ( DomainLogSinkFileInterface* )pTheSink->data;

	if( dirPath == NULL )
	{
		pTheFileSink->loggingPath[ 0 ] = 0;
		DomainLogSinkFileInterfaceLogClose( pTheFileSink );
	}
	else
	{
		char holdingBuffer[ DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH ];
		LogMemoryZero( holdingBuffer, DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH );

		LogConvertWCharToChar( holdingBuffer, DOMAINLOGGER_SINK_FILE_INTERFACE_MAX_BASE_PATH_LENGTH, dirPath );
	
		DomainLoggerFileSinkBuildAndSetFilePath( pTheFileSink, holdingBuffer, prefix );
	}
	
#else
#endif	
}


void DomainLoggerFileSinkSetPath( DomainLogSinkInterface *pTheSink, const char *dirPath, const char *prefix )
{
	DomainLogSinkFileInterface *pTheFileSink;
	pTheFileSink = ( DomainLogSinkFileInterface* )pTheSink->data;

	if( dirPath == NULL )
	{
		pTheFileSink->loggingPath[ 0 ] = 0;
		DomainLogSinkFileInterfaceLogClose( pTheFileSink );
	}
	else
	{
		DomainLoggerFileSinkBuildAndSetFilePath( pTheFileSink, dirPath, prefix );
	}
}


int DomainLoggerFileSinkPathSet( DomainLogSinkInterface *pTheSink )
{
	DomainLogSinkFileInterface *pTheFileSink;
	pTheFileSink = ( DomainLogSinkFileInterface* )pTheSink->data;

	if( pTheFileSink->loggingPath[ 0 ] != 0 )
	{
		return 1;
	}

	return 0;	
}


void DomainLoggerFileSinkEnable( DomainLogSinkInterface *pTheSink, int toEnable )
{
	if( pTheSink->enabled == toEnable )
	{
		return;
	}

	pTheSink->enabled = toEnable;
}


static void DomainLoggerFileSinkDestroy( DomainLogSinkInterface *theSink )
{
}





