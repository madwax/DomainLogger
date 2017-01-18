#ifndef __DOMAIN_LOGGER_CLIENT_H__
#define __DOMAIN_LOGGER_CLIENT_H__

#include "logger.common.h"

typedef struct 
{
	DomainLoggerCommonSettings theCommonSettings;
	
	/** registered sink settings objects */
	DomainLoggerSinkSettings *theHeadSinkSettings; 

	/** The runtime sink objects */
	DomainLoggerSinkBase *theSinksHead;

	/** The currently known domain on the system.
	*/
	DomainLoggerClientDomainInfo domains[ DOMAINLOGGER_DOMAINS_MAXNUMBER ];
	uint32_t domainsNumber;

	/** Flag used to stop the logging thread */
	int32_atomic shutdownFlag;
	/** The thread object what does all are work */
	LogThread theWorkerThread;

	/** The spin lock used to protect the queue holding messages to be logged */
	LogSpinLock theQueueProtection;
	/** The main queue holding all the log messages that need to be processed */
	LogMessageQueue theQueue;

	/** The spin lock used to protect the free queue and log message blocks */
	LogSpinLock theFreeQueueProtection;
	/** The free queue */
	LogMessageQueue theFreeQueue;

	/** The log message blocks */
	LogMessageBlock *theBlocksHead;
	uint32_t numberOfBlocks;
	/* the number of log messages in the system */
	uint32_t numberOfLogMessages;

} DomainLoggerClient;

#endif /*  __DOMAIN_LOGGER_CLIENT_H__ */
