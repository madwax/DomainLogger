#ifndef __DOMAIN_LOGGER_SINK_FILE_H__
#define __DOMAIN_LOGGER_SINK_FILE_H__

#include "logger.common.h"

#ifndef DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH
	#define DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH 200
#endif

#ifndef DOMAINLOGGER_FILELOGGING_MAX_PATH_LENGTH
	#define DOMAINLOGGER_FILELOGGING_MAX_PATH_LENGTH 255
#endif

#ifndef DOMAINLOGGER_FILELOGGING_MAX_OUTPUTBUFFER_LENGTH
	#define DOMAINLOGGER_FILELOGGING_MAX_OUTPUTBUFFER_LENGTH 1024
#endif

/** The file sink object 
*/
typedef struct
{
	DomainLoggerSinkBase base;
	/** File sutff */
	/** The base file path */
	char fileLoggingBasePath[ DOMAINLOGGER_FILELOGGING_MAX_BASE_PATH_LENGTH ];

	/** The log file object*/
	LogFile theLogFile;

	/** The buffer used to write out */
	char theOutputBuffer[ DOMAINLOGGER_FILELOGGING_MAX_OUTPUTBUFFER_LENGTH ];

} DomainLoggerSinkFile;

extern DomainLoggerSinkSettings* DomainLoggerSinkFileCreateSettings();


#endif /*  __DOMAIN_LOGGER_SINK_FILE_H__ */ 