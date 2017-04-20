#include "demo.h"


#if defined( _WIN32 )
	#define MySecondSleep( _x ) Sleep( _x * 1000 )
#else
	#define MySecondSleep( _x ) sleep( _x )
#endif


// The domains we are going to use
static const char *domainOne = "demo.domain.1";
static const char *domainTwo = "demo.domain.2";
static const char *otherDomain = "other";
static const char *theApplicationName = "Demo.DomainLogger.1";


int main( int argc, char **argv )
{
	int mode;
	int s,ss;
	float sss;

	mode = 0;
	s = 32;
	ss = 0x42;
	sss = 10.042f;

	/* Start the logger going */
	for( int i=1; i<argc; i++ )
	{
		if( strcmp( argv[ i ], "--colour" ) == 0 )
		{
			mode = 1;
		}
		else if( strcmp( argv[ i ], "--debug" ) == 0 )
		{
			mode = 2;
		}
	}
	
	if( mode == 0 )
	{
		DomainLoggerConsoleEnable( DomainLoggerConsoleOutputMono );
	}
	else if( mode == 1 )
	{
		DomainLoggerConsoleEnable( DomainLoggerConsoleOutputColoured );
	}
	else if( mode == 2 )
	{
		DomainLoggerConsoleEnable( DomainLoggerConsoleOutputDebugger );
	}
	
	DomainLoggerSetDefaultLevel( DomainLoggingLevelVerbose );
	
	if( DomainLoggerStart( theApplicationName, NULL ) )
	{
		printf( "Failed to start domain logging so aborting\n" );
		return 1;
	}

	/* First test run through all the different logging level */
	LogError( otherDomain, "I am an error = %d %d %f", s, ss, sss );

	LogFatal( otherDomain, "I am a fatal 1 =  %d-%d-%f", s, ss, sss  );
	LogFatal( otherDomain, "I am a fatal 2 =  %d-%d-%f", s, ss, sss  );
	
	LogSystem( otherDomain, "I am a system = %d-%d-%f", s, ss, sss  );
	LogWarning( otherDomain, "I am a warning = %d-%d-%f", s, ss, sss  );
	LogInfo( otherDomain, "I am an info = %d-%d-%f", s, ss, sss  );
	LogDebug( otherDomain, "I am a debug = %d-%x-%f", s, ss, sss  );
	LogVerbose( otherDomain, "I am a verbose = %d-%x-%f", s, ss, sss  );

	LogError( domainOne, "Bugger this i'm off" );
	LogError( domainTwo, "No you are not!" );

	/* Shutting this down. This will hold up the process while the logging thread is stopped
	*/
	DomainLoggerClose();

	return 0;
}

