#include "logger.sink.file.h"

#if( IS_WIN32 == 1 )

#include <Shlobj.h>
#include <Knownfolders.h>
#include <wchar.h>

#endif

static int DomainLoggerSinkFileGetBasePath( char *path, size_t maxPathLength, const char *applicationName );
static int DomainLoggerSinkFileFileName( char *logFilePath, size_t filePathBufferSize, const char *basePath, const char *appName, int useUTC );

static DomainLoggerSinkFile *s_TheFileSink = NULL;

#if( IS_WIN32 == 1 )

static int DomainLoggerSinkFileFileNameWChar( wchar_t *logFilePath, size_t filePathBufferSize, const char *basePath, const char *appName, int useUTC );


static int DomainLoggerSinkFileGetBasePath( char *path, size_t maxPathLength, const char *applicationName )
{
	int r = 1;
	wchar_t *toUsersLocalAppData;

	toUsersLocalAppData = NULL;

	if( SHGetKnownFolderPath( &FOLDERID_LocalAppData, 0, NULL, &toUsersLocalAppData ) == S_OK )
	{
		char *base = LogMemoryAlloc( DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH ); 
		if( base != NULL )
		{
			WideCharToMultiByte( CP_UTF8, 0, toUsersLocalAppData, -1, base, DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH, NULL, NULL );

			snprintf( path, ( int )maxPathLength, "%s\\%s\\logs", base, applicationName );

			LogMemoryFree( base );

			r = 0;
		}

		CoTaskMemFree( toUsersLocalAppData );
	}

	return r;
}

#elif( IS_OSX == 1 )

static int DomainLoggerSinkFileGetBasePath( char *path, size_t maxPathLength, const char *applicationName )
{
	snprintf( path, maxPathLength, "%s/Library/Application Support/%s/logs", getenv( "HOME" ), applicationName );
	return 0;
}

#elif( IS_IOS == 1 )

static int DomainLoggerSinkFileGetBasePath( char *path, size_t maxPathLength, const char *applicationName )
{
	#warning Is there a better way?
	snprintf( path, maxPathLength, "%s/Library/Documents/%s/Logs", getenv( "HOME" ), applicationName );
	return 0;
}

#elif( IS_ANDROID == 1 )

static int DomainLoggerSinkFileGetBasePath( char *path, size_t maxPathLength, const char *applicationName )
{
#error !FIX
	return 0;
}

#else

static int DomainLoggerSinkFileGetBasePath( char *path, size_t maxPathLength, const char *applicationName )
{
	snprintf( path, maxPathLength, "/var/tmp/%s/logs", applicationName );
	return 0;
}

#endif


#if( IS_WIN32 )
int DomainLoggerSinkFileFileNameWChar( wchar_t *logFilePath, size_t filePathBufferSize, const char *basePath, const char *appName, int useUTC )
{
	wchar_t basePathW[ DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH ];

	LogConvertCharToWChar( basePathW, DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH, basePath );

	filePathBufferSize = filePathBufferSize;

	SYSTEMTIME theCurrentTime;
	if( useUTC )
	{
		GetSystemTime( &theCurrentTime );
	}
	else
	{
		GetLocalTime( &theCurrentTime );
	}

	if( appName == NULL )
	{
		static wchar_t *localFormat = L"%s/%02d%02d%02d-%02d%02d%02d.log";
		static wchar_t *utcFormat = L"%s/%02d%02d%02d-%02d%02d%02d-UTC.log";

		wchar_t *usingFormat;

		if( useUTC )
		{
			usingFormat = utcFormat;
		}
		else
		{
			usingFormat = localFormat;
		}

		wsprintfW( logFilePath, usingFormat, basePathW, theCurrentTime.wYear, theCurrentTime.wMonth, theCurrentTime.wDay, theCurrentTime.wHour, theCurrentTime.wMinute, theCurrentTime.wSecond );
	}
	else
	{
		static wchar_t *localFormat = L"%s/%s-%02d%02d%02d-%02d%02d%02d.log";
		static wchar_t *utcFormat = L"%s/%s-%02d%02d%02d-%02d%02d%02d-UTC.log";
		wchar_t *usingFormat;
		wchar_t appNameW[ DOMAINLOGGER_APPLICATION_NAME_MAX_LENGTH ];

		LogConvertCharToWChar( appNameW, DOMAINLOGGER_APPLICATION_NAME_MAX_LENGTH, appName );

		if( useUTC )
		{
			usingFormat = utcFormat;
		}
		else
		{
			usingFormat = localFormat;
		}

		wsprintfW( logFilePath, usingFormat, basePathW, appNameW, theCurrentTime.wYear, theCurrentTime.wMonth, theCurrentTime.wDay, theCurrentTime.wHour, theCurrentTime.wMinute, theCurrentTime.wSecond );
	}

	return 0;
}
#endif

int DomainLoggerSinkFileFileName( char *logFilePath, size_t filePathBufferSize, const char *basePath, const char *appName, int useUTC )
{
#if( NO_CLIB_WIN32 == 1 )

	DLOG_UNUSED( logFilePath );
	DLOG_UNUSED( filePathBufferSize );
	DLOG_UNUSED( basePath );
	DLOG_UNUSED( appName );
	DLOG_UNUSED( useUTC );

	return 1;

#else

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

	if( appName == NULL )
	{
		static char *localFormat = "%s/%02d%02d%02d-%02d%02d%02d.log";
		static char *utcFormat = "%s/%02d%02d%02d-%02d%02d%02d-UTC.log";
		char *usingFormat;

		if( useUTC )
		{
			usingFormat = utcFormat;
		}
		else
		{
			usingFormat = localFormat;
		}

		snprintf( logFilePath, filePathBufferSize, usingFormat, basePath, theCurrentTime.tm_year, theCurrentTime.tm_mon, theCurrentTime.tm_mday, theCurrentTime.tm_hour, theCurrentTime.tm_min, theCurrentTime.tm_sec );
	}
	else
	{
		static char *localFormat = "%s/%s-%02d%02d%02d-%02d%02d%02d.log";
		static char *utcFormat = "%s/%s-%02d%02d%02d-%02d%02d%02d-UTC.log";
		char *usingFormat;

		if( useUTC )
		{
			usingFormat = utcFormat;
		}
		else
		{
			usingFormat = localFormat;
		}

		snprintf( logFilePath, filePathBufferSize, usingFormat, basePath, appName, theCurrentTime.tm_year, theCurrentTime.tm_mon, theCurrentTime.tm_mday, theCurrentTime.tm_hour, theCurrentTime.tm_min, theCurrentTime.tm_sec );
	}

	return 0;
#endif
}

static int DomainLoggerSinkFileOpenLogFile( DomainLoggerSinkFile *theSink )
{
// Now create the log file
#if( IS_WIN32 == 1 )
	wchar_t logFilePath[ DOMAINLOGGER_FILELOGGING_MAX_PATH_LENGTH ];

	if( DomainLoggerSinkFileFileNameWChar( logFilePath, DOMAINLOGGER_FILELOGGING_MAX_PATH_LENGTH, theSink->fileLoggingBasePath, theSink->base.pCommonSettings->applicationName, 1 ) == 1 )
	{
		reportFatal( "Failed to create filepath for log file" );
		return 1;
	}

	if( LogFileOpenForWritingFilePathWChar( &theSink->theLogFile, logFilePath ) == 1 )
	{
		reportFatal( "Failed to open the log file" );
		return 1;
	}

#else
	char logFilePath[ DOMAINLOGGER_FILELOGGING_MAX_PATH_LENGTH ];

	if( DomainLoggerSinkFileFileName( logFilePath, DOMAINLOGGER_FILELOGGING_MAX_PATH_LENGTH, theSink->fileLoggingBasePath, theSink->base.pCommonSettings->applicationName, 1 ) == 1 )
	{
		reportFatal( "Failed to create filepath for log file" );
		return 1;
	}

	if( LogFileOpenForWritingFilePath( &theSink->theLogFile, logFilePath ) == 1 )
	{
		reportFatal( "Failed to open the log file" );
		return 1;
	}

#endif
	return 0;
}

static int DomainLoggerSinkFileInit( DomainLoggerSinkBase *sink )
{
	DomainLoggerSinkFile *theSink;
	
	theSink = ( DomainLoggerSinkFile* )sink;

	// Did the user set the log path?
	if( theSink->fileLoggingBasePath[ 0 ]  == 0 )
	{
		// need to get the default logging path now.
		if( DomainLoggerSinkFileGetBasePath( theSink->fileLoggingBasePath, DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH, theSink->base.pCommonSettings->applicationName ) )
		{
			reportFatal( "Unable to workout default logging path" ); 
			return 1;
		}

		// now validate the path.
		if( LogValidateDirectory( theSink->fileLoggingBasePath ) )
		{
			reportFatal( "Unable to validate logging path" );
			reportFatal( theSink->fileLoggingBasePath );
			return 1;
		}
	}

	if( DomainLoggerSinkFileOpenLogFile( theSink ) )
	{
		reportFatal( "Failed to open log file" );
		return 1;
	}

	return 0;
}

static int DomainLoggerSinkFileThreadInit( DomainLoggerSinkBase *sink )
{
	DLOG_UNUSED( sink );

	return 0;
}

static int DomainLoggerSinkFileProcessLogMessage( DomainLoggerSinkBase *sink, LogMessage *pMsg )
{
	DomainLoggerSinkFile *theSink;

	theSink = ( DomainLoggerSinkFile* )sink;
	
	if( LogFileIsOpen( &theSink->theLogFile ) == 0 )
	{
		// log file is not open!
		return 1;
	}

///	snprintf( theSink->theOutputBuffer, DOMAINLOGGER_FILELOGGING_MAX_OUTPUTBUFFER_LENGTH, "%s,%s,%d,%d,\"%s\"\n", DomainLoggingGetStringForLevel( pMsg->level ), pMsg->lpDomain, pMsg->count, pMsg->msg );

	snprintf( theSink->theOutputBuffer, DOMAINLOGGER_FILELOGGING_MAX_OUTPUTBUFFER_LENGTH, "%s,%s,%lld,\'%s\'\n", DomainLoggingGetStringForLevel( pMsg->level ), pMsg->lpDomain, pMsg->count, pMsg->msg );

	LogFileWrite( &theSink->theLogFile, theSink->theOutputBuffer, strlen( theSink->theOutputBuffer ) );

	return 0;
}

static DomainLoggerSinkFile* DomainLoggerSinkFileCreate( DomainLoggerSinkSettings *thisIs )
{
	DomainLoggerSinkFile *r;

	r = ( DomainLoggerSinkFile* )LogMemoryAlloc( sizeof( DomainLoggerSinkFile ) );
	LogMemoryZero( r, sizeof( DomainLoggerSinkFile ) );

	r->base.onInitCb = &DomainLoggerSinkFileInit;
	r->base.onThreadInitCb = &DomainLoggerSinkFileThreadInit;
	r->base.onProcessLogMessageCb = &DomainLoggerSinkFileProcessLogMessage;

	thisIs->theSink = ( DomainLoggerSinkBase* )r;

	return r;
}

/** Used to create the file sink settings object if needed */
DomainLoggerSinkFile* DomainLoggerSinkFileSettingsCheck( DomainLoggerSinkSettings *thisIs )
{
	if( s_TheFileSink == NULL )
	{
		// Create the console sink and populate it with default settings
		s_TheFileSink = DomainLoggerSinkFileCreate( thisIs );
	}
	return s_TheFileSink;
}

int DomainLoggerSinkFileSettingsCMDHandler( DomainLoggerSinkSettings *thisIs, DomainLoggerCommonSettings *pCommonSettings, char *option, int currentIndex, int argc, char **argv )
{
	DLOG_UNUSED( pCommonSettings );

	if( strcasecmp( "file.path", option ) == 0 )
	{
		DomainLoggerSinkFile *pSink;

		/* depending on the app type open "log" window, if supported */
		pSink = DomainLoggerSinkFileSettingsCheck( thisIs );

		++currentIndex;
		if( currentIndex >= argc )
		{
			reportFatal( "A directroy was not passed after --log.file.path" );
			return -1;
		}

		option = argv[ currentIndex ];

		strncpy( s_TheFileSink->fileLoggingBasePath, option, DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH );

		return 1;
	}
	else if( strcasecmp( "file.disabled", option ) == 0 )
	{
		// user wants to disable the file logging sink
		if( s_TheFileSink != NULL )
		{
			thisIs->theSink = NULL;
			LogMemoryFree( s_TheFileSink );
			s_TheFileSink = NULL;
		}
		return 0;
	}
	else if( strcasecmp( "file", option ) == 0 )
	{
		/* enable the file sink if not already */
		DomainLoggerSinkFileSettingsCheck( thisIs );
		return 0;
	}
	return -2;
}

DomainLoggerSinkSettings* DomainLoggerSinkFileCreateSettings()
{
	DomainLoggerSinkSettings *pR;

	pR = ( DomainLoggerSinkSettings* )LogMemoryAlloc( sizeof( DomainLoggerSinkSettings ) );
	if( pR != NULL )
	{
		pR->pNext = NULL;
		pR->theSink = NULL;
		pR->onHandleCMDCb = &DomainLoggerSinkFileSettingsCMDHandler;

		s_TheFileSink = DomainLoggerSinkFileSettingsCheck( pR );
	}
	return pR;
}
