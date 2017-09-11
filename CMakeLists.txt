cmake_minimum_required(VERSION 3.1.0)

project( DomainLogger )

option( DomainLoggerEnableTest "Build the DomainLoggers Test" ON )
option( DomainLoggerEnableDemo "Build the DomainLoggers Demo" ON )

set( TARGET_INCLUDE_DIR ${DomainLogger_SOURCE_DIR}/include )
set( TARGET_SOURCE_DIR "${DomainLogger_SOURCE_DIR}/src" )

set( TARGET_SOURCES ${TARGET_INCLUDE_DIR}/domainlogger/domainlogger.h
	${TARGET_SOURCE_DIR}/logger.common.c
	${TARGET_SOURCE_DIR}/logger.common.h
	${TARGET_SOURCE_DIR}/logger.client.c
	${TARGET_SOURCE_DIR}/logger.client.h
	${TARGET_SOURCE_DIR}/logger.sink.console.h
	${TARGET_SOURCE_DIR}/logger.sink.console.c
	${TARGET_SOURCE_DIR}/logger.sink.file.h
	${TARGET_SOURCE_DIR}/logger.sink.file.c
)

set( TARGET_LINK "" )
set( TESTS_LINK "" )
set( DEMO_LINK "" )

set( TARGET_DEFINES "" )
set( TARGET_DEFINES_PRIVATE "" )

if( MSVC )
	list( APPEND TARGET_DEFINES_PRIVATE 
		"-D_CRT_SECURE_NO_WARNINGS" 
		"/wd4068"
	)
endif()

if( DEFINED DomainLoggerPlatform )

	message( "User passed ${DomainLoggerPlatform}" )

else()

	message( "Working out what platform we have" )

	set( DomainLoggerSubPlatform "" )

	# need to work out what we have.
	if( ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )

		set( DomainLoggerPlatform "Win32" )

	elseif( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )

		set( DomainLoggerPlatform "Darwin" )

		set( DomainLoggerSubPlatform "OSX" )

	elseif( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )

		set( DomainLoggerPlatform "Linux" )

		if( ANDROID_NDK )

			set( DomainLoggerSubPlatform "Android" )

		else()

			set( DomainLoggerSubPlatform "Desktop" )

		endif()

	elseif( ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" )

		set( DomainLoggerPlatform "FreeBSD" )
		set( DomainLoggerSubPlatform "" )

	endif()

endif()


if( ${DomainLoggerPlatform} MATCHES "Win32" )
	
	list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_WIN32=1" )

	# If you want to build without replying on a C Runtime under Win32
	if( DEFINED DomainLoggerPlatformNoCLib )

		list( APPEND TARGET_DEFINES "DL_PLATFORM_NO_CLIB=1" )
	else()

		list( APPEND TARGET_DEFINES "DL_PLATFORM_NO_CLIB=0" )

	endif()

else()

	list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_UNIX=1" )

	if( ${DomainLoggerPlatform} MATCHES "Darwin" )

		list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_BSD=1" )
		list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_DARWIN=1" )

		list( APPEND  TARGET_SOURCES 
			${TARGET_SOURCE_DIR}/logger.sink.console.m 
			${TARGET_SOURCE_DIR}/logger.sink.file.m 
		)
		
		find_library( foundation Foundation REQUIRED )
		find_library( coreFoundation CoreFoundation REQUIRED )
		
		list( APPEND TARGET_LINK ${foundation} ${coreFoundation} )

		if( ${DomainLoggerSubPlatform} MATCHES "OSX" )
			
			list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_OSX=1" )

		elseif( ${DomainLoggerSubPlatform} MATCHES "iOS" )

			list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_IOS=1" )

		endif()

	elseif( ${DomainLoggerPlatform} MATCHES "Linux" )

		list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_LINUX=1" )

		if( ${DomainLoggerSubPlatform} MATCHES "Android" )

			list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_ANDROID=1" )

		endif()

	elseif( ${DomainLoggerPlatform} MATCHES "FreeBSD" )

		list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_BSD=1" )
		list( APPEND TARGET_DEFINES "DL_PLATFORM_IS_FREEBSD=1" )

	else()

	endif()
	
endif()

# TODO Remove when we switch backend changes.
list( APPEND TARGET_DEFINES "DOMAINLOGGER_OLD_API=0" )

if( ${Base_Platform_UNIX} )
	
	message( "Using pthreads as were on a Unix platform" )
	
	find_package( Threads REQUIRED )

	list( APPEND TARGET_LINK Threads::Threads )
	list( APPEND TESTS_LINLK Threads::Threads )
	list( APPEND DEMO_LINK Threads::Threads )

else()

endif()

list( APPEND TESTS_LINLK  DomainLogger )
list( APPEND DEMO_LINLK  DomainLogger )

add_library( DomainLogger STATIC ${TARGET_SOURCES} )

set_target_properties( DomainLogger PROPERTIES ARCHIVE_OUTPUT_NAME "DomainLogger" )

target_include_directories( DomainLogger PRIVATE ${TARGET_INCLUDE_DIR} ${TARGET_SOURCE_DIR} )
target_compile_definitions( DomainLogger PRIVATE ${TARGET_DEFINES} ${TARGET_DEFINES_PRIVATE} )
target_link_libraries( DomainLogger PRIVATE ${TARGET_LINK} )

target_include_directories( DomainLogger INTERFACE ${TARGET_INCLUDE_DIR} )
target_compile_definitions( DomainLogger INTERFACE ${TARGET_DEFINES} )
target_link_libraries( DomainLogger INTERFACE ${TARGET_LINK} )

if( ${DomainLoggerEnableTest} )

set( TESTS_INCLUDE_DIR ${DomainLogger_SOURCE_DIR}/tests/include )
set( TESTS_SOURCE_DIR "${DomainLogger_SOURCE_DIR}/tests/src" )

set( TEST_SOURCES ${TESTS_INCLUDE_DIR}/tests.h
	${TESTS_INCLUDE_DIR}/queue.h
	${TESTS_SOURCE_DIR}/queue_test_1.c
	${TESTS_SOURCE_DIR}/queue_test_2.c
	${TESTS_SOURCE_DIR}/queue_test_3.c
	${TESTS_SOURCE_DIR}/queue_test_4.c
	
	${TESTS_SOURCE_DIR}/tests.c
)

add_executable( DomainLogger.UnitTests ${TEST_SOURCES} )
set_target_properties( DomainLogger.UnitTests PROPERTIES ARCHIVE_OUTPUT_NAME "Test_DomainLogger" )
target_include_directories( DomainLogger.UnitTests PRIVATE ${TARGET_INCLUDE_DIR} ${TARGET_SOURCE_DIR} ${TESTS_INCLUDE_DIR} ${TESTS_SOURCE_DIR} )
target_link_libraries( DomainLogger.UnitTests PRIVATE ${TESTS_LINLK} )

endif( ${DomainLoggerEnableTest} )

if( ${DomainLoggerEnableDemo} )

set( DEMO_INCLUDE_DIR ${DomainLogger_SOURCE_DIR}/demo/include )
set( DEMO_SOURCE_DIR "${DomainLogger_SOURCE_DIR}/demo/src" )

set( DEMO_SOURCES ${DEMO_INCLUDE_DIR}/demo.h
	${DEMO_SOURCE_DIR}/demo.c
)

add_executable( DomainLogger.Demo ${DEMO_SOURCES} )
set_target_properties( DomainLogger.Demo PROPERTIES ARCHIVE_OUTPUT_NAME "Demo_DomainLogger" )
target_include_directories( DomainLogger.Demo PRIVATE ${DEMO_INCLUDE_DIR} ${DEMO_SOURCE_DIR} )
target_link_libraries( DomainLogger.Demo PRIVATE ${TESTS_LINLK} )

endif( ${DomainLoggerEnableDemo} )
