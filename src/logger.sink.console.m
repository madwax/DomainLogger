#import <Foundation/Foundation.h>

#import "logger.sink.console.h"


void DomainLoggerConsoleNSLog( const char *message, ... )
{
  va_list args;
  va_start( args, message );
    
	NSLog( @"%@", [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:message] arguments:args] );
    
	va_end(args);
}

