# This handles some of the platform stuff when using SDL2 and OpenGL together
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

if( ${Base_Plaform} MATCHES "win32" )

elseif( ${Base_Plaform} MATCHES "osx" )

elseif( ${Base_Plaform} MATCHES "ios" )

elseif( ${Base_Plaform} MATCHES "linux" )

elseif( ${Base_Plaform} MATCHES "freeBSD" )

elseif( ${Base_Plaform} MATCHES "android" )

endif()

if( ${Base_Platform_UNIX} )
	
	message( "Using pthreads as were on a Unix platform" )
	
	find_package( Threads REQUIRED )

	list( APPEND TARGET_LINK Threads::Threads )
	list( APPEND TESTS_LINLK Threads::Threads )
	list( APPEND DEMO_LINK Threads::Threads )

else()

	message( "not using pthreads" )

endif()

list( APPEND TESTS_LINLK  DomainLogger )
list( APPEND DEMO_LINLK  DomainLogger )

add_library( DomainLogger STATIC ${TARGET_SOURCES} )

set_target_properties( DomainLogger PROPERTIES ARCHIVE_OUTPUT_NAME "DomainLogger" )

target_include_directories( DomainLogger PRIVATE ${TARGET_INCLUDE_DIR} ${TARGET_SOURCE_DIR} )
target_compile_definitions( DomainLogger PRIVATE "-DDOMAINLOGGER_DEFAULTLEVEL=3" )
target_link_libraries( DomainLogger PRIVATE ${TARGET_LINK} )

target_include_directories( DomainLogger INTERFACE $<BUILD_INTERFACE:${TARGET_INCLUDE_DIR}> )
target_compile_definitions( DomainLogger INTERFACE "-DDOMAINLOGGER_DEFAULTLEVEL=3" )
target_link_libraries( DomainLogger INTERFACE ${TARGET_LINK} )

if( ${DomainLoggerEnableTest} )

set( TESTS_INCLUDE_DIR ${DomainLogger_SOURCE_DIR}/tests/include )
set( TESTS_SOURCE_DIR "${DomainLogger_SOURCE_DIR}/tests/src" )

set( TEST_SOURCES ${TESTS_INCLUDE_DIR}/tests.h
	${TESTS_INCLUDE_DIR}/queue.h
	${TESTS_SOURCE_DIR}/queue_test_1.c
	${TESTS_SOURCE_DIR}/queue_test_2.c
	${TESTS_SOURCE_DIR}/queue_test_3.c
	${TESTS_SOURCE_DIR}/tests.c
)

add_executable( Test_DomainLogger ${TEST_SOURCES} )
set_target_properties( Test_DomainLogger PROPERTIES ARCHIVE_OUTPUT_NAME "Test_DomainLogger" )
target_include_directories( Test_DomainLogger PRIVATE ${TARGET_INCLUDE_DIR} ${TARGET_SOURCE_DIR} ${TESTS_INCLUDE_DIR} ${TESTS_SOURCE_DIR} )
target_link_libraries( Test_DomainLogger PRIVATE ${TESTS_LINLK} )

endif( ${DomainLoggerEnableTest} )

if( ${DomainLoggerEnableDemo} )

set( DEMO_INCLUDE_DIR ${DomainLogger_SOURCE_DIR}/demo/include )
set( DEMO_SOURCE_DIR "${DomainLogger_SOURCE_DIR}/demo/src" )

set( DEMO_SOURCES ${DEMO_INCLUDE_DIR}/demo.h
	${DEMO_SOURCE_DIR}/demo.c
)

add_executable( Demo_DomainLogger ${DEMO_SOURCES} )
set_target_properties( Demo_DomainLogger PROPERTIES ARCHIVE_OUTPUT_NAME "Demo_DomainLogger" )
target_include_directories( Demo_DomainLogger PRIVATE ${DEMO_INCLUDE_DIR} ${DEMO_SOURCE_DIR} )
target_link_libraries( Demo_DomainLogger PRIVATE ${TESTS_LINLK} )

endif( ${DomainLoggerEnableDemo} )
