cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE)

	set(PLATFORM				"mac-x86-64")
	if ( "${CMAKE_OSX_SYSROOT}" STREQUAL "" )
		set(CMAKE_OSX_SYSROOT "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk")
	endif()
	set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
	set(CMAKE_C_FLAGS_DEBUG		"${CMAKE_C_FLAGS_DEBUG} -O0 -fsanitize=address -D_LIBCPP_HAS_NO_ASAN")
	set(CMAKE_CXX_FLAGS_DEBUG	"${CMAKE_CXX_FLAGS_DEBUG} -O0 -fsanitize=address -D_LIBCPP_HAS_NO_ASAN")
	set(CMAKE_C_FLAGS_RELEASE	"-O3 -DNDEBUG=1")
	set(CMAKE_CXX_FLAGS_RELEASE	"-O3 -DNDEBUG=1")
	set(CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_ENABLE_SESSION -DSQLITE_ENABLE_PREUPDATE_HOOK -DSQLITE_ENABLE_JSON1 -std=gnu99")
    set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -stdlib=libc++ -arch x86_64 -isysroot ${CMAKE_OSX_SYSROOT} -fmessage-length=0 -fdiagnostics-show-note-include-stack -fmacro-backtrace-limit=0 -Wno-trigraphs -fpascal-strings -Wall -Wextra -Weffc++ -Wno-gnu-zero-variadic-macro-arguments -pedantic -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_ENABLE_SESSION -DSQLITE_ENABLE_PREUPDATE_HOOK -DSQLITE_ENABLE_JSON1 -fasm-blocks -fstrict-aliasing -Wdeprecated-declarations -mmacosx-version-min=10.13 -Wno-sign-conversion -MMD -MT dependencies -Qunused-arguments ")
	set(CMAKE_OSX_SYSROOT_LIB	"${CMAKE_OSX_SYSROOT}/usr/lib")
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
	else()
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O3")
	endif()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
	set(CMAKE_SKIP_RPATH TRUE)
endif()


if (TARGET nodeoze)

message( "nodeoze library is already defined" )

else()

include_directories( include src/win32 ext/libuv/include ext/libuv/src ext/rapidjson/include ext/botan ../openssl/include-darwin ext/sqlite ext/http-parser ext/yajl/src/api ext/uriparser/include ext/msgpack-c/include ../boost )


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_ENABLE_SESSION -DSQLITE_ENABLE_PREUPDATE_HOOK -DSQLITE_ENABLE_JSON1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_ENABLE_SESSION -DSQLITE_ENABLE_PREUPDATE_HOOK -DSQLITE_ENABLE_JSON1")


set(NODEOZE_SRCS ../ext/http-parser/http_parser.c ../ext/uriparser/src/UriCommon.c ../ext/uriparser/src/UriCompare.c ../ext/uriparser/src/UriEscape.c ../ext/uriparser/src/UriFile.c ../ext/uriparser/src/UriIp4.c ../ext/uriparser/src/UriIp4Base.c ../ext/uriparser/src/UriNormalize.c ../ext/uriparser/src/UriNormalizeBase.c ../ext/uriparser/src/UriParse.c ../ext/uriparser/src/UriParseBase.c ../ext/uriparser/src/UriQuery.c ../ext/uriparser/src/UriRecompose.c ../ext/uriparser/src/UriResolve.c ../ext/uriparser/src/UriShorten.c ../ext/yajl/src/yajl.c ../ext/yajl/src/yajl_alloc.c ../ext/yajl/src/yajl_buf.c ../ext/yajl/src/yajl_encode.c ../ext/yajl/src/yajl_gen.c ../ext/yajl/src/yajl_lex.c ../ext/yajl/src/yajl_parser.c ../ext/yajl/src/yajl_tree.c ../ext/sqlite/sqlite3.c ../ext/msgpack-c/src/objectc.c ../ext/msgpack-c/src/vrefbuffer.c ../ext/msgpack-c/src/zone.c ../ext/msgpack-c/src/unpack.c ../include/nodeoze/doctest.h ../include/nodeoze/naddress.h ../include/nodeoze/nalgorithm.h ../include/nodeoze/nany.h ../include/nodeoze/napplication.h ../include/nodeoze/narp.h ../include/nodeoze/nasync_repeater.h ../include/nodeoze/nbarrier.h ../include/nodeoze/nbase64.h ../include/nodeoze/nbuffer.h ../include/nodeoze/ncompat.h ../include/nodeoze/nconcurrent.h ../include/nodeoze/nconnection.h ../include/nodeoze/ndatabase.h ../include/nodeoze/ndeque.h ../include/nodeoze/nencrypt.h ../include/nodeoze/nendian.h ../include/nodeoze/nendpoint.h ../include/nodeoze/nenum.h ../include/nodeoze/nerror.h ../include/nodeoze/nevent.h ../include/nodeoze/nexpected.h ../include/nodeoze/nfs.h ../include/nodeoze/nhttp.h ../include/nodeoze/njson.h ../include/nodeoze/nkeychain.h ../include/nodeoze/nlimiter.h ../include/nodeoze/nlocation.h ../include/nodeoze/nlog.h ../include/nodeoze/nmac.h ../include/nodeoze/nmachine.h ../include/nodeoze/nmacros.h ../include/nodeoze/nmarkers.h ../include/nodeoze/nmime.h ../include/nodeoze/nmsgpack.h ../include/nodeoze/nnotification.h ../include/nodeoze/noauth.h ../include/nodeoze/nodeoze.h ../include/nodeoze/noid.h ../include/nodeoze/npath.h ../include/nodeoze/nprintf.h ../include/nodeoze/nprocess.h ../include/nodeoze/npromise.h ../include/nodeoze/nproxy.h ../include/nodeoze/nrandom.h ../include/nodeoze/nrpc.h ../include/nodeoze/nrunloop.h ../include/nodeoze/nscoped_operation.h ../include/nodeoze/nsema.h ../include/nodeoze/nsha1.h ../include/nodeoze/nshell.h ../include/nodeoze/nsingleton.h ../include/nodeoze/nsocket.h ../include/nodeoze/nstackwalk.h ../include/nodeoze/nstate_machine.h ../include/nodeoze/nstream.h ../include/nodeoze/nstring.h ../include/nodeoze/ntest.h ../include/nodeoze/nthread.h ../include/nodeoze/ntimer.h ../include/nodeoze/ntime.h ../include/nodeoze/ntls.h ../include/nodeoze/ntypes.h ../include/nodeoze/ntypestring.h ../include/nodeoze/nunicode.h ../include/nodeoze/nuri.h ../include/nodeoze/nusername.h ../include/nodeoze/nuuid.h ../include/nodeoze/nws.h ../include/nodeoze/nzip.h ../src/naddress.cpp ../src/nany.cpp ../src/napplication.cpp ../src/nasync_repeater.cpp ../src/nbarrier.cpp ../src/nbase64.cpp ../src/nbuffer.cpp ../src/nconnection.cpp ../src/ndatabase.cpp ../src/ndatabase_sqlite.cpp ../src/nencrypt.cpp ../src/nendpoint.cpp ../src/nerror.cpp ../src/nfs.cpp ../src/nhttp.cpp ../src/njson.cpp ../src/njson.cpp ../src/nlimiter.cpp ../src/nlocation.cpp ../src/nlog.cpp ../src/nmachine.cpp ../src/nmarkers.cpp ../src/nmime.cpp ../src/nnotification.cpp ../src/noauth.cpp ../src/npromise.cpp ../src/nproxy.cpp ../src/nsha1.cpp ../src/nsocket.cpp ../src/nstream.cpp ../src/ntime.cpp ../src/nunicode.cpp ../src/nuri.cpp ../src/nusername.cpp ../src/nuuid.cpp ../src/nws.cpp ../src/nrpc.cpp ../src/nmac.cpp ../src/nmsgpack.cpp ../src/nprocess.cpp ../src/nstate_machine.cpp ../src/nzip.cpp ../src/android/nfs_android.h ../src/android/nkeychain_android.h ../src/android/nmachine_android.h ../src/apple/narp_apple.h ../src/apple/nfs_apple.h ../src/apple/nkeychain_apple.h ../src/apple/nmachine_apple.h ../src/libuv/nendpoint_libuv.h ../src/libuv/nsocket_libuv.h ../src/linux/narp_linux.h ../src/linux/nfs_linux.h ../src/linux/nkeychain_linux.h ../src/linux/nmachine_linux.h ../src/ndatabase_sqlite.h ../src/win32/crc32.h ../src/win32/event_log.h ../src/win32/narp_win32.h ../src/win32/nfs_win32.h ../src/win32/nkeychain_win32.h ../src/win32/nlog_win32.h ../src/win32/nmachine_win32.h )

set(NODEOZE_TEST_SRCS ../src/test.cpp)

set( NODEOZE_SRCS ${NODEOZE_SRCS} ../ext/libuv/src/uv-common.c ../ext/libuv/src/fs-poll.c ../ext/libuv/src/inet.c ../ext/libuv/src/threadpool.c ../ext/libuv/src/version.c ../src/ntls.cpp ../src/libuv/nerror_libuv.h ../src/libuv/nerror_libuv.cpp ../src/libuv/nrunloop_libuv.cpp ../src/libuv/nprocess_libuv.cpp ../src/libuv/naddress_libuv.cpp)

if ( APPLE )

	find_library(FOUNDATION_FRAMEWORK Foundation)
	find_library(OPEN_DIRECTORY_FRAMEWORK OpenDirectory)
	find_library(QUARTZ_FRAMEWORK Quartz)
	find_library(QUARTZ_CORE_FRAMEWORK QuartzCore)
	find_library(APPKIT_FRAMEWORK AppKit)
	find_library(COCOA_FRAMEWORK Cocoa)
	find_library(IOKIT_FRAMEWORK IOKit)
	find_library(SYSTEM_CONFIGURATION_FRAMEWORK SystemConfiguration)
	find_library(SECURITY_FOUNDATION_FRAMEWORK SecurityFoundation)
	find_library(SECURITY_FRAMEWORK Security)
	find_library(OPEN_DIRECTORY_FRAMEWORK OpenDirectory)
	set(LIBRESOLV			"${CMAKE_OSX_SYSROOT_LIB}/libresolv.tbd") 
	set(LIBSSL				"${CMAKE_CURRENT_SOURCE_DIR}/../openssl/lib/mac/libssl.a")
	set(LIBCRYPTO			"${CMAKE_CURRENT_SOURCE_DIR}/../openssl/lib/mac/libcrypto.a")
	list(APPEND OPENSSL_LIBRARIES ${LIBSSL})
	list(APPEND OPENSSL_LIBRARIES ${LIBCRYPTO})
	list(APPEND LIB_LIST ${FOUNDATION_FRAMEWORK})
	list(APPEND LIB_LIST ${QUARTZ_FRAMEWORK})
	list(APPEND LIB_LIST ${QUARTZ_CORE_FRAMEWORK})
	list(APPEND LIB_LIST ${APPKIT_FRAMEWORK})
	list(APPEND LIB_LIST ${COCOA_FRAMEWORK})
	list(APPEND LIB_LIST ${IOKIT_FRAMEWORK})
	list(APPEND LIB_LIST ${SYSTEM_CONFIGRUATION_FRAMEWORK})
	list(APPEND LIB_LIST ${SECURITY_FOUNDATION_FRAMEWORK})
	list(APPEND LIB_LIST ${SECURITY_FRAMEWORK})
	list(APPEND LIB_LIST ${OPEN_DIRECTORY_FRAMEWORK})
	list(APPEND LIB_LIST ${LIBRESOLV})
	list(APPEND LIB_LIST ${LIBSSL})
	list(APPEND LIB_LIST ${LIBCRYPTO})
endif()

if ( WIN32 )

	set(NODEOZE_SRCS ${NODEOZE_SRCS} ../ext/libuv/src/win/async.c ../ext/libuv/src/win/core.c ../ext/libuv/src/win/dl.c ../ext/libuv/src/win/error.c ../ext/libuv/src/win/fs.c ../ext/libuv/src/win/fs-event.c ../ext/libuv/src/win/getaddrinfo.c ../ext/libuv/src/win/handle.c ../ext/libuv/src/win/loop-watcher.c ../ext/libuv/src/win/pipe.c ../ext/libuv/src/win/poll.c ../ext/libuv/src/win/process.c ../ext/libuv/src/win/process-stdio.c ../ext/libuv/src/win/req.c ../ext/libuv/src/win/signal.c ../ext/libuv/src/win/stream.c ../ext/libuv/src/win/util.c ../ext/libuv/src/win/winapi.c ../ext/libuv/src/win/winsock.c ../ext/libuv/src/win/tcp.c ../ext/libuv/src/win/thread.c ../ext/libuv/src/win/timer.c ../ext/libuv/src/win/tty.c ../ext/libuv/src/win/udp.c ../src/win32/narp_win32.cpp ../src/win32/nfs_win32.cpp ../src/win32/nkeychain_win32.cpp ../src/win32/nlog_win32.cpp ../src/win32/nmachine_win32.cpp ../src/win32/npath_win32.cpp ../src/win32/nprocess_win32.cpp ../src/win32/nuser.cpp ../src/win32/crc32.cpp)

else()

	set(NODEOZE_SRCS ${NODEOZE_SRCS} ../ext/libuv/src/unix/async.c ../ext/libuv/src/unix/core.c ../ext/libuv/src/unix/fs.c ../ext/libuv/src/unix/getaddrinfo.c ../ext/libuv/src/unix/loop-watcher.c ../ext/libuv/src/unix/loop.c ../ext/libuv/src/unix/pipe.c ../ext/libuv/src/unix/poll.c ../ext/libuv/src/unix/process.c ../ext/libuv/src/unix/proctitle.c ../ext/libuv/src/unix/signal.c ../ext/libuv/src/unix/stream.c ../ext/libuv/src/unix/tcp.c ../ext/libuv/src/unix/thread.c ../ext/libuv/src/unix/timer.c ../ext/libuv/src/unix/udp.c)

	if ( APPLE )
		
		set(NODEOZE_SRCS ${NODEOZE_SRCS} ../ext/libuv/src/unix/darwin.c ../ext/libuv/src/unix/darwin-proctitle.c ../ext/libuv/src/unix/kqueue.c ../ext/libuv/src/unix/fsevents.c ../src/apple/narp_apple.cpp ../src/apple/nfs_apple.mm ../src/apple/nkeychain_apple.mm ../src/apple/nlog_apple.mm ../src/apple/nmachine_apple.mm ../src/apple/nprocess_apple.mm ../src/apple/npath_apple.mm)
    
	elseif (LINUX)
		
		set(NODEOZE_SRCS ${NODEOZE_SRCS} ../ext/libuv/src/unix/linux-core.c ../ext/libuv/src/unix/linux-inotify.c ../ext/libuv/src/unix/linux-syscalls.c ../src/linux/narp_linux.cpp ../src/linux/nfs_linux.cpp ../src/linux/nkeychain_linux.cpp ../src/linux/nlog_linux.cpp ../src/linux/nmachine_linux.cpp ../src/linux/npath_linux.cpp ../src/linux/nprocess_linux.cpp)
		
	endif()
		
endif()

add_library( nodeoze_objects OBJECT ${NODEOZE_SRCS})
add_library( nodeoze STATIC $<TARGET_OBJECTS:nodeoze_objects> ../src/dummy.cpp ) 
add_dependencies( nodeoze nodeoze_objects )
set_target_properties(nodeoze PROPERTIES PREFIX "")

add_executable(nodeoze_test ${NODEOZE_TEST_SRCS} $<TARGET_OBJECTS:nodeoze_objects>)
target_link_libraries(nodeoze_test ${LIB_LIST})

if ( APPLE )

	add_custom_command( TARGET nodeoze
						PRE_BUILD
						COMMAND rm -fr $<TARGET_FILE:nodeoze> )
							
endif()

add_custom_command(	TARGET nodeoze_test
					POST_BUILD
					COMMAND $<TARGET_FILE:nodeoze_test>)

endif()

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
