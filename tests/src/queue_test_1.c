#include "queue.h"


LogMessage* Test_DomainLogger_Queue_1_CreateMessage( uint32_t c )
{	
	LogMessage *r;

	r = ( LogMessage* )LogMemoryAlloc( sizeof( LogMessage ) );
	//LogMemoryFree( r );

	r->next = NULL;
	r->lineNumber = c;

	return r;
}

int Test_DomainLogger_Queue_1_ReleaseMessage( LogMessage *msg, uint32_t watchNumber )
{
	int r;

	r = 1;

	if( msg->lineNumber == watchNumber )
	{
		r = 0;
	}

	LogMemoryFree( msg );

	return r;
}

int Test_DomainLogger_Queue_1()
{
	LogMessage *msg, *msgX;
	// We are using the queue in one thread so no point in protection.
	LogMessageQueue theQueue;
	// counters used to see what's what
	uint32_t itemsAdded;
	uint32_t itemsRemoved;

	itemsAdded = itemsRemoved = 0;

	LogMessageQueueCreate( &theQueue );

	msg = Test_DomainLogger_Queue_1_CreateMessage( itemsAdded ); ++itemsAdded;

	LogMessageQueuePush( &theQueue, msg );

	msgX = LogMessageQueuePop( &theQueue );

	if( msg != msgX )
	{
		return 1;
	}
	
	if( Test_DomainLogger_Queue_1_ReleaseMessage( msg, itemsRemoved ) == 1 )
	{
		return 2;
	}
	++itemsRemoved;

	LogMessageQueuePush( &theQueue, Test_DomainLogger_Queue_1_CreateMessage( itemsAdded ) );  ++itemsAdded;
	LogMessageQueuePush( &theQueue, Test_DomainLogger_Queue_1_CreateMessage( itemsAdded ) );  ++itemsAdded;

	if( Test_DomainLogger_Queue_1_ReleaseMessage( LogMessageQueuePop( &theQueue ), itemsRemoved ) == 1 ) { return 3; }; ++itemsRemoved;
	if( Test_DomainLogger_Queue_1_ReleaseMessage( LogMessageQueuePop( &theQueue ), itemsRemoved ) == 1 ) { return 4; }; ++itemsRemoved;

	LogMessageQueuePush( &theQueue, Test_DomainLogger_Queue_1_CreateMessage( itemsAdded ) );  ++itemsAdded;
	LogMessageQueuePush( &theQueue, Test_DomainLogger_Queue_1_CreateMessage( itemsAdded ) );  ++itemsAdded;

	if( Test_DomainLogger_Queue_1_ReleaseMessage( LogMessageQueuePop( &theQueue ), itemsRemoved ) == 1 ) { return 5; }; ++itemsRemoved;

	LogMessageQueuePush( &theQueue, Test_DomainLogger_Queue_1_CreateMessage( itemsAdded ) );  ++itemsAdded;

	if( Test_DomainLogger_Queue_1_ReleaseMessage( LogMessageQueuePop( &theQueue ), itemsRemoved ) == 1 ) { return 6; }; ++itemsRemoved;
	if( Test_DomainLogger_Queue_1_ReleaseMessage( LogMessageQueuePop( &theQueue ), itemsRemoved ) == 1 ) { return 7; }; ++itemsRemoved;

	return 0;
}

