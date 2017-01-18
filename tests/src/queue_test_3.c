#include "queue.h"

#define TEST_QUEUE_3_NUMBEROFPRODUCERS 4

typedef struct
{
	int32_atomic stopProducters;
	int32_atomic stopConsumer;
	int32_atomic recved[ TEST_QUEUE_3_NUMBEROFPRODUCERS ];
  uint32_t numIs;
	
	LogMessageQueue theQueue;
	LogSpinLock theQueueProtection;

} Test_DomainLogger_Queue_3_Shared;

typedef struct
{
	LogThread theThread;
	uint32_t index;
	Test_DomainLogger_Queue_3_Shared *pShared;
	int32_atomic sent;
	uint32_t rateControl;
} Test_DomainLogger_Queue_3_Sender;


LogMessage* SenderCreateMessge( uint32_t tag, void *pSender )
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

void SenderThreadFunction( void *pUserData )
{
	Test_DomainLogger_Queue_3_Sender *pSender;
	int32_t hasSent;
	uint32_t counter;

	pSender = ( Test_DomainLogger_Queue_3_Sender* )pUserData;
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
      
      msg->count = hasSent;
			++hasSent;

			LogSpinLockCapture( &pSender->pShared->theQueueProtection );
			LogMessageQueuePush( &pSender->pShared->theQueue, msg );
			LogSpinLockRelease( &pSender->pShared->theQueueProtection );

			counter = 0;
		}
	}

	LogAtomicSetInt32( &pSender->sent, hasSent );
}

void Test_DomainLogger_Queue_3_ConsumerThreadFunction( void *pUserData )
{
	Test_DomainLogger_Queue_3_Shared *pShared;
	LogMessage *msg;
	uint32_t isRunning;
	uint32_t recved[ TEST_QUEUE_3_NUMBEROFPRODUCERS ];
	uint32_t i;
  uint32_t qw=0;
  
	for( i=0; i<TEST_QUEUE_3_NUMBEROFPRODUCERS; ++i )
	{
		recved[ i ] = 0;
	}

	pShared = ( Test_DomainLogger_Queue_3_Shared* )pUserData;

	isRunning = 1;

	while( isRunning )
	{
		LogSpinLockCapture( &pShared->theQueueProtection );
		msg = LogMessageQueuePop( &pShared->theQueue );
		LogSpinLockRelease( &pShared->theQueueProtection );

		if( msg != NULL )
		{
			uint32_t j;

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
			else
			{
				LogThreadYeild();
			}
		}
	}

	for( i=0; i<TEST_QUEUE_3_NUMBEROFPRODUCERS; ++i )
	{
		LogAtomicSetInt32( ( pShared->recved + i ), recved[ i ] );
	}
  pShared->numIs = qw;
}


static int Test_DomainLogger_Queue_3_TestProcutersAllRunning( Test_DomainLogger_Queue_3_Sender *pSenders, uint32_t num )
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

static int Test_DomainLogger_Queue_3_TestProcutersAllStopped( Test_DomainLogger_Queue_3_Sender *pSenders, uint32_t num )
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

uint32_t RandBetweenRange( uint32_t min, uint32_t max)
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

int Test_DomainLogger_Queue_3()
{
	Test_DomainLogger_Queue_3_Shared theShared;
	Test_DomainLogger_Queue_3_Sender theSenders[ TEST_QUEUE_3_NUMBEROFPRODUCERS ];
	LogThread theConsumerThread;	
	Test_DomainLogger_Queue_3_Sender *pSender;
	uint32_t i;
	
	LogMemoryZero( &theShared, sizeof( Test_DomainLogger_Queue_3_Shared ) );

	for( i=0; i<TEST_QUEUE_3_NUMBEROFPRODUCERS; ++i )
	{
		pSender = ( theSenders + i );

		LogMemoryZero( pSender, sizeof( Test_DomainLogger_Queue_3_Sender ) );
		pSender->pShared = &theShared;

		pSender->rateControl = RandBetweenRange( 10000, 20000 );
		pSender->index = i;

		LogThreadCreate( &pSender->theThread, SenderThreadFunction, pSender );
	}

	LogThreadCreate( &theConsumerThread, &Test_DomainLogger_Queue_3_ConsumerThreadFunction, ( void* )&theShared );

	for( i=0; i<TEST_QUEUE_3_NUMBEROFPRODUCERS; ++i )
	{
		pSender = ( theSenders + i );
		LogThreadStart( &pSender->theThread );
	}

	LogThreadStart( &theConsumerThread );

	LogThreadSleepSeconds( 1 );

	i = 0;
	while( Test_DomainLogger_Queue_3_TestProcutersAllRunning( ( theSenders + 0 ), TEST_QUEUE_3_NUMBEROFPRODUCERS ) != 0 && LogThreadRunning( &theConsumerThread ) == 0 )
	{
		++i;
		if( i == 999 )
		{
			for( i=0; i<TEST_QUEUE_3_NUMBEROFPRODUCERS; ++i )
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

	LogThreadSleepSeconds( 5 );

	LogAtomicSetInt32( &theShared.stopProducters, 1 );

	LogThreadSleepSeconds( 1 );

	i=0;
	while( Test_DomainLogger_Queue_3_TestProcutersAllStopped( ( theSenders + 0 ), TEST_QUEUE_3_NUMBEROFPRODUCERS ) != 0 )
	{
		++i;
		if( i == 999 )
		{
			for( i=0; i<TEST_QUEUE_3_NUMBEROFPRODUCERS; ++i )
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
  
	for( i=0; i<TEST_QUEUE_3_NUMBEROFPRODUCERS; ++i )
	{
    t1 += theShared.recved[ i ];
    t2 += theSenders[ i ].sent;
    
		if( theShared.recved[ i ] != theSenders[ i ].sent )
		{
      f = 1;
		}
	}

  
  if( f == 1 )
  {
    printf( "    total:%d t1:%d t2:%d\n", theShared.numIs, t2, t1 );
    return 4;
  }
  
	return 0;
}