#ifndef __DOMAIN_LOGGER_CLIENT_H__
#define __DOMAIN_LOGGER_CLIENT_H__

#include "domainlogger/domainlogger.h"

#include "logger.common.h"


#define DOMAINLOGGER_MAX_NUMBER_OF_SINKS 4

/** Hosts a sink interface object.
*/
typedef struct _DomainLogSinkItem
{
	/** The sink we are to use */
	DomainLogSinkInterface *theSink;

} DomainLogSinkItem;

typedef struct 
{
	/** When the client was created.
	*/
	DomainLoggerDateTime createdOnUTC;
	DomainLoggerDateTime createdOnLocal;
	/** the number of miliseconds from when logging was started relative form system start */
	uint64_t whenEpoc;

	/** The number of sinks we have active. */
	uint32_t numberOfSinks;
	/** Array of sink items that we are using */
	DomainLogSinkItem *theSinks[ DOMAINLOGGER_MAX_NUMBER_OF_SINKS ];

	/** The currently known domain on the system.
	* The char* in each item in the array are refs and should never be freed.
	*/
	DomainLoggerClientDomainInfo domains[ DOMAINLOGGER_DOMAINS_MAXNUMBER ];
	uint32_t domainsNumber;

	DomainLoggingLevels defaultLoggingLevel;

	/** The currently known domain on the system.
	* The char* in each item in the array are refs and should never be freed.
	*/
	DomainLoggerClientDomainInfo preDomains[ DOMAINLOGGER_DOMAINS_MAXNUMBER ];
	uint32_t preDomainsNumber;

	/** The logging thread event.
	* Why wake up all the time?
	*/
	LogThreadEvent theThreadEvent;

	/** Flag used to tell if the logging thread started or not.*/
	int32_atomic isRunning;
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

	char *theApplicationName;
	char *theApplicationIdentifier;

} DomainLoggerClient;


extern DomainLoggerClient *DomainLoggerClientGet();

#endif /*  __DOMAIN_LOGGER_CLIENT_H__ */
