#include "queue.h"

#define TEST_QUEUE_4_NUMBEROFPRODUCERS 25

typedef struct
{
	int32_atomic stopProducters;
	int32_atomic stopConsumer;
	int32_atomic recved[ TEST_QUEUE_4_NUMBEROFPRODUCERS ];
  uint32_t numIs;
	
	LogMessageQueue theQueue;

} Test_DomainLogger_Queue_4_Shared;

typedef struct
{
	LogThread theThread;
	uint32_t index;
	Test_DomainLogger_Queue_4_Shared *pShared;
	int32_atomic sent;
	uint32_t rateControl;
} Test_DomainLogger_Queue_4_Sender;


static LogMessage* SenderCreateMessge( uint32_t tag, void *pSender )
{
	LogMessage *msg;

	msg = ( LogMessage* )LogMemoryAlloc( sizeof( LogMessage ) );
	if( msg != NULL )
	{
		LogMemoryZero( msg, sizeof( LogMessage ) );

		msg->lineNumber = tag;
		msg->fileName = ( char* )pSender;
	}

	return msg;
}

static void SenderThreadFunction( void *pUserData )
{
	Test_DomainLogger_Queue_4_Sender *pSender;
	int32_t hasSent;
	uint32_t counter;

	pSender = ( Test_DomainLogger_Queue_4_Sender* )pUserData;
	hasSent = counter = 0;

	while( LogAtomicCompInt32( &pSender->pShared->stopProducters, 0 ) )
	{
		++counter;
		if( counter == pSender->rateControl )
		{
			LogMessage *msg;

			msg = SenderCreateMessge( pSender->index, ( void * )pSender );
			if( msg == NULL )
			{
				return;
			}
      
			++hasSent;

			LogMessageQueuePush( &pSender->pShared->theQueue, msg );

			counter = 0;
		}
	}

	LogAtomicSetInt32( &pSender->sent, hasSent );
}

void Test_DomainLogger_Queue_4_ConsumerThreadFunction( void *pUserData )
{
	Test_DomainLogger_Queue_4_Shared *pShared;
	LogMessage *msg;
	uint32_t isRunning;
	uint32_t recved[ TEST_QUEUE_4_NUMBEROFPRODUCERS ];
	uint32_t i;
  uint32_t qw=0;
  
	for( i=0; i<TEST_QUEUE_4_NUMBEROFPRODUCERS; ++i )
	{
		recved[ i ] = 0;
	}

	pShared = ( Test_DomainLogger_Queue_4_Shared* )pUserData;

	isRunning = 1;

	while( isRunning )
	{
		msg = LogMessageQueuePop( &pShared->theQueue );

		if( msg != NULL )
		{
			uint32_t j;
			
			if( msg->lineNumber >= TEST_QUEUE_4_NUMBEROFPRODUCERS )
			{
				fprintf( stdout, "BANG" );
			}
			
			j = recved[ msg->lineNumber ];
			++j;
			recved[ msg->lineNumber ] = j;

			LogMemoryFree( msg );
      ++qw;
		}
		else
		{
			if( LogAtomicCompInt32( &pShared->stopConsumer, 1 ) )
			{
				isRunning = 0;
			}
		}
	}
	uint32_t totRecv = 0;
	
	for( i=0; i<TEST_QUEUE_4_NUMBEROFPRODUCERS; ++i )
	{
		totRecv += recved[ i ];
		LogAtomicSetInt32( ( pShared->recved + i ), recved[ i ] );
	}
	
  pShared->numIs = qw;
}

static int Test_DomainLogger_Queue_4_TestProcutersAllRunning( Test_DomainLogger_Queue_4_Sender *pSenders, uint32_t num )
{
	uint32_t i;

	for( i=0; i<num; ++i )
	{
		if( LogThreadRunning( &( ( pSenders + i )->theThread ) ) == 0 )
		{
			return 1;
		}
	}
	return 0;
}

static int Test_DomainLogger_Queue_4_TestProcutersAllStopped( Test_DomainLogger_Queue_4_Sender *pSenders, uint32_t num )
{
	uint32_t i;

	for( i=0; i<num; ++i )
	{
		if( LogThreadStopped( &( ( pSenders + i )->theThread ) ) == 0 )
		{
			return 1;
		}
	}
	return 0;
}

static uint32_t RandBetweenRange( uint32_t min, uint32_t max)
{
	int r;
	uint32_t range = 1 + max - min;
	uint32_t buckets = RAND_MAX / range;
	uint32_t limit = buckets * range;

	do
	{
		r = rand();
	} while( ( uint32_t)r >= limit );

	return min + ( r / buckets );
}

int Test_DomainLogger_Queue_4()
{
	Test_DomainLogger_Queue_4_Shared theShared;
	Test_DomainLogger_Queue_4_Sender theSenders[ TEST_QUEUE_4_NUMBEROFPRODUCERS ];
	LogThread theConsumerThread;	
	Test_DomainLogger_Queue_4_Sender *pSender;
	uint32_t i;
	
	LogMemoryZero( &theShared, sizeof( Test_DomainLogger_Queue_4_Shared ) );
	
	LogMemoryZero( &theSenders, sizeof( theSenders ) );
	
	LogMessageQueueCreate( &theShared.theQueue );

	for( i=0; i<TEST_QUEUE_4_NUMBEROFPRODUCERS; ++i )
	{
		pSender = ( theSenders + i );

		pSender->pShared = &theShared;

		pSender->rateControl = RandBetweenRange( 10000, 40000 );
		pSender->index = i;

		LogThreadCreate( &pSender->theThread, SenderThreadFunction, pSender );
	}

	LogThreadCreate( &theConsumerThread, &Test_DomainLogger_Queue_4_ConsumerThreadFunction, ( void* )&theShared );

	for( i=0; i<TEST_QUEUE_4_NUMBEROFPRODUCERS; ++i )
	{
		pSender = ( theSenders + i );
		LogThreadStart( &pSender->theThread );
	}

	LogThreadStart( &theConsumerThread );

	LogThreadSleepSeconds( 1 );

	i = 0;
	while( Test_DomainLogger_Queue_4_TestProcutersAllRunning( ( theSenders + 0 ), TEST_QUEUE_4_NUMBEROFPRODUCERS ) != 0 && LogThreadRunning( &theConsumerThread ) == 0 )
	{
		++i;
		if( i == 999 )
		{
			for( i=0; i<TEST_QUEUE_4_NUMBEROFPRODUCERS; ++i )
			{
				LogThreadKill( &( ( theSenders + i )->theThread ) );
			}
			LogThreadKill( &theConsumerThread );
			return 1;
		}
		else
		{
			LogThreadYeild();
		}
	}

	LogThreadSleepSeconds( 50 );

	LogAtomicSetInt32( &theShared.stopProducters, 1 );

	LogThreadSleepSeconds( 1 );

	i=0;
	while( Test_DomainLogger_Queue_4_TestProcutersAllStopped( ( theSenders + 0 ), TEST_QUEUE_4_NUMBEROFPRODUCERS ) != 0 )
	{
		++i;
		if( i == 999 )
		{
			for( i=0; i<TEST_QUEUE_4_NUMBEROFPRODUCERS; ++i )
			{
				LogThreadKill( &( ( theSenders + i )->theThread ) );
			}
			LogThreadKill( &theConsumerThread );
			return 2;
		}
	
		LogThreadYeild();
	}

	LogAtomicSetInt32( &theShared.stopConsumer, 1 );

	LogThreadSleepSeconds( 1 );

	i=0;
	while( LogThreadStopped( &theConsumerThread ) == 0 )
	{
		++i;
		if( i == 999 )
		{
			LogThreadKill( &theConsumerThread );
      return 3;
		}

		LogThreadYeild();
	}
  
  int f = 0;
  uint32_t t1 = 0;
  uint32_t t2 = 0;
  
	for( i=0; i<TEST_QUEUE_4_NUMBEROFPRODUCERS; ++i )
	{
    t1 += theShared.recved[ i ];
    t2 += theSenders[ i ].sent;
    
		if( theShared.recved[ i ] != theSenders[ i ].sent )
		{
      f = 1;
		}
	}

	LogMessageQueueDestroy( &theShared.theQueue );
	
  if( f == 1 )
  {
    printf( "    total:%d t1:%d t2:%d\n", theShared.numIs, t2, t1 );
    return 4;
  }
  
	return 0;
}