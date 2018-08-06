cmake_minimum_required(VERSION 2.8)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)

	set (CMAKE_CXX_STANDARD 17)
	set (CMAKE_CXX_STANDARD_REQUIRED ON)

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

endif()

if (TARGET nodeoze)

message( "nodeoze library is already defined" )

else()

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/botan)

add_custom_command(	OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/botan/botan_all.h ${CMAKE_CURRENT_BINARY_DIR}/botan/botan_all_internal.h ${CMAKE_CURRENT_BINARY_DIR}/botan/botan_all.cpp
					COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/ext/botan/configure.py --minimized-build --enable-modules=auto_rng,stateful_rng,system_rng,rdseed,md5,sha1,sha2_64,tls --single-amalgamation-file --amalgamation --disable-shared
					WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/botan )

include_directories( include ${CMAKE_CURRENT_BINARY_DIR}/botan src/win32 
	ext/libuv/include 
	ext/libuv/src 
	ext/rapidjson/include 
	ext/sqlite 
	ext/http-parser 
	ext/yajl/src/api 
	ext/uriparser/include 
	ext/msgpack-c/include 
	${CMAKE_CURRENT_BINARY_DIR}/build/include/external)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_ENABLE_SESSION -DSQLITE_ENABLE_PREUPDATE_HOOK -DSQLITE_ENABLE_JSON1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSQLITE_THREADSAFE=1 -DSQLITE_OMIT_LOAD_EXTENSION=1 -DSQLITE_ENABLE_SESSION -DSQLITE_ENABLE_PREUPDATE_HOOK -DSQLITE_ENABLE_JSON1")

set(NODEOZE_SRCS 
	${CMAKE_CURRENT_BINARY_DIR}/botan/botan_all.h 
	${CMAKE_CURRENT_BINARY_DIR}/botan/botan_all_internal.h 
	${CMAKE_CURRENT_BINARY_DIR}/botan/botan_all.cpp 
	ext/http-parser/http_parser.c 
	ext/uriparser/src/UriCommon.c 
	ext/uriparser/src/UriCompare.c 
	ext/uriparser/src/UriEscape.c 
	ext/uriparser/src/UriFile.c 
	ext/uriparser/src/UriIp4.c 
	ext/uriparser/src/UriIp4Base.c 
	ext/uriparser/src/UriNormalize.c 
	ext/uriparser/src/UriNormalizeBase.c 
	ext/uriparser/src/UriParse.c 
	ext/uriparser/src/UriParseBase.c 
	ext/uriparser/src/UriQuery.c 
	ext/uriparser/src/UriRecompose.c 
	ext/uriparser/src/UriResolve.c 
	ext/uriparser/src/UriShorten.c 
	ext/yajl/src/yajl.c 
	ext/yajl/src/yajl_alloc.c 
	ext/yajl/src/yajl_buf.c 
	ext/yajl/src/yajl_encode.c 
	ext/yajl/src/yajl_gen.c 
	ext/yajl/src/yajl_lex.c 
	ext/yajl/src/yajl_parser.c 
	ext/yajl/src/yajl_tree.c 
	ext/sqlite/sqlite3.c 
	ext/msgpack-c/src/objectc.c 
	ext/msgpack-c/src/vrefbuffer.c 
	ext/msgpack-c/src/zone.c 
	ext/msgpack-c/src/unpack.c 
	include/nodeoze/doctest.h 
	include/nodeoze/address.h 
	include/nodeoze/algorithm.h 
	include/nodeoze/any.h 
	include/nodeoze/arp.h 
	include/nodeoze/base64.h 
	include/nodeoze/buffer.h 
	include/nodeoze/compat.h 
	include/nodeoze/concurrent.h 
	include/nodeoze/database.h 
	include/nodeoze/deque.h 
	include/nodeoze/endian.h 
	include/nodeoze/endpoint.h 
	include/nodeoze/event.h 
	include/nodeoze/filesystem.h
	include/nodeoze/fs.h 
	include/nodeoze/http.h 
	include/nodeoze/json.h 
	include/nodeoze/location.h 
	include/nodeoze/mac.h 
	include/nodeoze/machine.h 
	include/nodeoze/macros.h 
	include/nodeoze/mime.h 
	include/nodeoze/msgpack.h 
	include/nodeoze/nodeoze.h 
	include/nodeoze/printf.h 
	include/nodeoze/process.h 
	include/nodeoze/promise.h 
	include/nodeoze/proxy.h 
	include/nodeoze/random.h 
	include/nodeoze/rpc.h 
	include/nodeoze/runloop.h 
	include/nodeoze/scoped_operation.h 
	include/nodeoze/sha1.h 
	include/nodeoze/shell.h 
	include/nodeoze/singleton.h 
	include/nodeoze/state_machine.h 
	include/nodeoze/stream.h 
	include/nodeoze/string.h 
	include/nodeoze/test.h 
	include/nodeoze/thread.h 
	include/nodeoze/timer.h 
	include/nodeoze/time.h 
	include/nodeoze/tls.h 
	include/nodeoze/types.h 
	include/nodeoze/typestring.h 
	include/nodeoze/unicode.h 
	include/nodeoze/uri.h 
	include/nodeoze/uuid.h 
	include/nodeoze/ws.h 
	include/nodeoze/zip.h
	include/nodeoze/traits.h
	src/address.cpp 
	src/any.cpp 
	src/base64.cpp 
	src/buffer.cpp 
	src/database.cpp 
	src/endpoint.cpp 
	src/fs.cpp 
	src/http.cpp 
	src/json.cpp 
	src/json.cpp 
	src/location.cpp 
	src/machine.cpp 
	src/mime.cpp 
	src/net.cpp
	src/promise.cpp 
	src/proxy.cpp 
	src/sha1.cpp 
	src/stream.cpp 
	src/time.cpp 
	src/unicode.cpp 
	src/uri.cpp 
	src/uuid.cpp 
	src/ws.cpp 
	src/rpc.cpp 
	src/mac.cpp 
	src/msgpack.cpp 
	src/process.cpp 
	src/state_machine.cpp 
	src/zip.cpp 
	src/apple/arp_apple.h 
	src/apple/machine_apple.h 
	src/net_utils.h 
	src/linux/arp_linux.h 
	src/linux/machine_linux.h 
	src/win32/crc32.h 
	src/win32/arp_win32.h 
	src/win32/machine_win32.h 
	src/event.cpp 
	include/nodeoze/bstream.h
	include/nodeoze/bstream/error_category_context.h
	include/nodeoze/bstream/base_classes.h
	include/nodeoze/bstream/error.h
	include/nodeoze/bstream/ibstream.h
	include/nodeoze/bstream/imbstream.h
	include/nodeoze/bstream/ifbstream.h
	include/nodeoze/bstream/macros.h
	include/nodeoze/bstream/msgpack.h
	include/nodeoze/bstream/obstream.h
	include/nodeoze/bstream/ombstream.h
	include/nodeoze/bstream/ofbstream.h
	include/nodeoze/bstream/typecode.h
	include/nodeoze/bstream/obstreambuf.h
	include/nodeoze/bstream/ibstreambuf.h
	include/nodeoze/bstream/obmembuf.h
	include/nodeoze/bstream/ibmembuf.h
	include/nodeoze/bstream/obfilebuf.h
	include/nodeoze/bstream/ibstream_traits.h
	include/nodeoze/bstream/numeric_deserializers.h
	include/nodeoze/bstream/context.h
	include/nodeoze/bstream/fwd_decls.h
	include/nodeoze/bstream/obstream_traits.h
	include/nodeoze/bstream/stdlib.h
	include/nodeoze/bstream/stdlib/chrono.h
	include/nodeoze/bstream/stdlib/deque.h
	include/nodeoze/bstream/stdlib/forward_list.h
	include/nodeoze/bstream/stdlib/list.h
	include/nodeoze/bstream/stdlib/map.h
	include/nodeoze/bstream/stdlib/pair.h
	include/nodeoze/bstream/stdlib/set.h
	include/nodeoze/bstream/stdlib/tuple.h
	include/nodeoze/bstream/stdlib/unordered_map.h
	include/nodeoze/bstream/stdlib/unordered_set.h
	include/nodeoze/bstream/stdlib/vector.h
	src/bstream/obfilebuf.cpp
	src/bstream/obmembuf.cpp
	src/bstream/ibmembuf.cpp
	src/bstream/ibstreambuf.cpp
	src/bstream/obstreambuf.cpp
	src/bstream/ibfilebuf.cpp
	src/bstream/ibstream.cpp
	src/bstream/obstream.cpp
	src/bstream/numstream.cpp
	src/bstream/error.cpp
	src/bstream/error_category_context.cpp
	include/nodeoze/bstream/ibfilebuf.h
	include/nodeoze/bstream/utils/preprocessor.h
	include/nodeoze/bstream/utils/traits.h
	include/nodeoze/bstream/utils/memory.h
	include/nodeoze/dump.h
	include/nodeoze/bstream/numstream.h
	src/umstream.cpp
	src/raft/error.cpp
	include/nodeoze/raft/error.h
	include/nodeoze/raft/log.h
	include/nodeoze/raft/log_frames.h
	include/nodeoze/raft/state_machine.h
	include/nodeoze/raft/types.h 
	)
    

set(NODEOZE_TEST_SRCS
	test/address.cpp
	test/buffer.cpp
	test/bstream/test0.cpp
	test/bstream/test1.cpp
	test/bstream/test2.cpp
	test/bstream/test3.cpp
	test/bstream/test4.cpp
	test/bstream/bstreambuf.cpp
	test/bstream/fbstream.cpp
	test/raft/log.cpp
	test/event.cpp
	test/fs.cpp
	test/json.cpp
	test/main.cpp
	test/msgpack.cpp)
#	test/tls.cpp)

set( NODEOZE_SRCS ${NODEOZE_SRCS} 
	ext/libuv/src/uv-common.c 
	ext/libuv/src/fs-poll.c 
	ext/libuv/src/inet.c 
	ext/libuv/src/threadpool.c 
	ext/libuv/src/version.c 
	src/tls.cpp 
	src/error_libuv.h 
	src/error_libuv.cpp 
	src/runloop.cpp)

if ( APPLE )

	find_library(COCOA_FRAMEWORK Cocoa)
	find_library(IOKIT_FRAMEWORK IOKit)
	find_library(SECURITY_FRAMEWORK Security)
	set(LIBRESOLV			"${CMAKE_OSX_SYSROOT_LIB}/libresolv.tbd") 
	list(APPEND LIB_LIST ${COCOA_FRAMEWORK})
	list(APPEND LIB_LIST ${IOKIT_FRAMEWORK})
	list(APPEND LIB_LIST ${SECURITY_FRAMEWORK})
	list(APPEND LIB_LIST ${LIBRESOLV})
endif()

if ( WIN32 )

	set(NODEOZE_SRCS ${NODEOZE_SRCS} 
	ext/libuv/src/win/async.c 
	ext/libuv/src/win/core.c 
	ext/libuv/src/win/detect-wakeup.c 
	ext/libuv/src/win/dl.c 
	ext/libuv/src/win/error.c 
	ext/libuv/src/win/fs.c 
	ext/libuv/src/win/fs-event.c 
	ext/libuv/src/win/getaddrinfo.c 
	ext/libuv/src/win/handle.c 
	ext/libuv/src/win/loop-watcher.c 
	ext/libuv/src/win/pipe.c 
	ext/libuv/src/win/poll.c 
	ext/libuv/src/win/process.c 
	ext/libuv/src/win/process-stdio.c 
	ext/libuv/src/win/req.c 
	ext/libuv/src/win/signal.c 
	ext/libuv/src/win/stream.c 
	ext/libuv/src/win/util.c 
	ext/libuv/src/win/winapi.c 
	ext/libuv/src/win/winsock.c 
	ext/libuv/src/win/tcp.c 
	ext/libuv/src/win/thread.c 
	ext/libuv/src/win/timer.c 
	ext/libuv/src/win/tty.c 
	ext/libuv/src/win/udp.c 
	src/win32/arp_win32.cpp 
	src/win32/machine_win32.cpp 
	src/win32/process_win32.cpp 
	src/win32/crc32.cpp)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOTAN_DLL=")

else()

	set(NODEOZE_SRCS ${NODEOZE_SRCS} 
	ext/libuv/src/unix/async.c 
	ext/libuv/src/unix/core.c 
	ext/libuv/src/unix/fs.c 
	ext/libuv/src/unix/getaddrinfo.c 
	ext/libuv/src/unix/loop-watcher.c 
	ext/libuv/src/unix/loop.c 
	ext/libuv/src/unix/pipe.c 
	ext/libuv/src/unix/poll.c 
	ext/libuv/src/unix/process.c 
	ext/libuv/src/unix/proctitle.c 
	ext/libuv/src/unix/signal.c 
	ext/libuv/src/unix/stream.c 
	ext/libuv/src/unix/tcp.c 
	ext/libuv/src/unix/thread.c 
	ext/libuv/src/unix/timer.c 
	ext/libuv/src/unix/udp.c)

	if ( APPLE )
		
		set(NODEOZE_SRCS ${NODEOZE_SRCS} 
			ext/libuv/src/unix/darwin.c 
			ext/libuv/src/unix/darwin-proctitle.c 
			ext/libuv/src/unix/kqueue.c 
			ext/libuv/src/unix/fsevents.c 
			src/apple/arp_apple.cpp 
			src/apple/machine_apple.mm 
			src/apple/process_apple.mm
			src/apple/filesystem_apple.mm)
    
	elseif (LINUX)
		
		set(NODEOZE_SRCS ${NODEOZE_SRCS} 
			ext/libuv/src/unix/linux-core.c 
			ext/libuv/src/unix/linux-inotify.c 
			ext/libuv/src/unix/linux-syscalls.c 
			src/linux/arp_linux.cpp 
			src/linux/machine_linux.cpp 
			src/linux/process_linux.cpp)
		
	endif()
		
endif()

add_library( nodeoze_objects OBJECT ${NODEOZE_SRCS})
add_library( nodeoze STATIC $<TARGET_OBJECTS:nodeoze_objects> ) 
add_dependencies( nodeoze nodeoze_objects )
add_dependencies( nodeoze nodeoze_test )
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
					COMMAND $<TARGET_FILE:nodeoze_test> --test-case=nodeoze/smoke/*)

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
