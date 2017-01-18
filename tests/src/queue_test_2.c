#include "queue.h"

typedef struct 
{
	int32_atomic stopAll;
	LogSpinLock theQueueProtection;
	LogMessageQueue theQueue;
	int32_atomic numberSent;
	int32_atomic numberRecved;
} Test_DomainLogger_Queue_1_Shared;

void Test_DomainLogger_Queue_1_ProducterThreadFunction( void *pUserData )
{
	Test_DomainLogger_Queue_1_Shared *pShared;
	LogMessage *pCurrent;
	
	int32_t hadOutgoing;
	uint32_t rateController;

	pShared = ( Test_DomainLogger_Queue_1_Shared* )pUserData;

	hadOutgoing = 0;
	rateController = 0;

	while( LogAtomicCompInt32( &pShared->stopAll, 0 ) )
	{
		++rateController;

		if( rateController == 200 )
		{
			pCurrent = ( LogMessage* )LogMemoryAlloc( sizeof( LogMessage ) );
			if( pCurrent != NULL )
			{
				LogMemoryZero( pCurrent, sizeof( LogMessage ) );
				++hadOutgoing;

				pCurrent->lineNumber = hadOutgoing;
				
				LogSpinLockCapture( &pShared->theQueueProtection );	
				LogMessageQueuePush( &pShared->theQueue, pCurrent );
				LogSpinLockRelease( &pShared->theQueueProtection );
			}
			rateController = 0;
		}
	}

	LogAtomicSetInt32( &pShared->numberSent, hadOutgoing );
}

// does the work of servicing the queue
void Test_DomainLogger_Queue_1_ConsumerThreadFunction( void *pUserData )
{
	Test_DomainLogger_Queue_1_Shared *pShared;
	LogMessage *pCurrent;
	int32_t hadIncoming;
	uint32_t isRunning;

	pShared = ( Test_DomainLogger_Queue_1_Shared* )pUserData;
	hadIncoming = 0;
	isRunning = 1;

	while( isRunning )
	{
		LogSpinLockCapture( &pShared->theQueueProtection );
		pCurrent = LogMessageQueuePop( &pShared->theQueue );
		LogSpinLockRelease( &pShared->theQueueProtection );

		if( pCurrent != NULL )
		{
			++hadIncoming;
			LogMemoryFree( pCurrent );
		}
		else
		{
			// nothing in the queue so exit out we go
			if( LogAtomicCompInt32( &pShared->stopAll, 999 ) )
			{
				isRunning = 0;
			}
			else
			{
				LogThreadYeild();
			}
		}
	}

	LogAtomicSetInt32( &pShared->numberRecved, hadIncoming );
}

int Test_DomainLogger_Queue_2()
{
	uint32_t counter;

	Test_DomainLogger_Queue_1_Shared theSharedData;
	LogThread theConsumerThread;
	LogThread theProducterThread;

	LogMemoryZero( &theSharedData, sizeof( Test_DomainLogger_Queue_1_Shared ) );

	LogSpinLockCreate( &theSharedData.theQueueProtection );
	LogMessageQueueCreate( &theSharedData.theQueue );

	LogThreadCreate( &theConsumerThread, &Test_DomainLogger_Queue_1_ConsumerThreadFunction, &theSharedData );
	LogThreadCreate( &theProducterThread, &Test_DomainLogger_Queue_1_ProducterThreadFunction, &theSharedData );

	LogThreadStart( &theConsumerThread );
	LogThreadStart( &theProducterThread );

	counter = 0;

	while( !( LogThreadRunning( &theConsumerThread ) == 1 && LogThreadRunning( &theProducterThread ) == 1 ) )
	{
		LogThreadYeild();

		++counter;
		if( counter >= 999999 )
		{
			// stop run aways!
			LogThreadKill( &theConsumerThread );
			LogThreadKill( &theProducterThread );

			return 1;
		}
	}

	LogThreadSleepSeconds( 5 );

	LogAtomicSetInt32( &theSharedData.stopAll, 1 );

	LogThreadSleepSeconds( 1 );

	LogAtomicSetInt32( &theSharedData.stopAll, 999 );

	LogThreadSleepSeconds( 1 );

	counter = 0;
	while( !( LogThreadStopped( &theConsumerThread ) == 1 && LogThreadStopped( &theProducterThread ) == 1 ) )
	{
		LogThreadYeild();
		++counter;
		if( counter >= 999999 )
		{
			// stop run aways!
			LogThreadKill( &theConsumerThread );
			LogThreadKill( &theProducterThread );

			return 2;
		}
	}

	if( theSharedData.numberRecved != theSharedData.numberSent )
	{
		return 3;
	}

	return 0;
}