#include "tests.h"

int RunTest( const char *name, int (*func)() )
{
	int r;
	r = ( *func )();

	if( r == 0 )
	{
		printf( "[PASSED] - %s\n", name );	
	}
	else
	{
		printf( "[FAILED][@%d] - %s\n", r, name );	
		return 1;
	}
	return 0;
}

int main( int argc, char **argv )
{
	int i;
	int doGetChar;

	doGetChar = 0;

	for( i=0; i<argc; ++i )
	{
		if( strcasecmp( argv[ i ], "--pause" ) )
		{
			doGetChar = 1;
		}
	}

	printf( "Starting Tests\n" );

	printf( "Queue Tests\n" );

	RunTest( "Queue Test 1", &Test_DomainLogger_Queue_1 );
	RunTest( "Queue Test 2", &Test_DomainLogger_Queue_2 );
	RunTest( "Queue Test 3", &Test_DomainLogger_Queue_3 );

	printf( "Finished Tests\n" );

	if( doGetChar )
	{
		printf( "Press RETURN...\n" );
		getchar();
	}

	return 0;
}