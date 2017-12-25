find_package (LibXml2)
find_package (OpenSSL)

#TODO - create a FindUriParser cmake module
#TODO - create a FindSqlite cmake module

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fpermissive -luriparser -lsqlite3 -luuid -ggdb")

include_directories (${LIBXML2_INCLUDE_DIR})

include_directories (${NetKit_SOURCE_DIR}/include ${NetKit_SOURCE_DIR}/ThirdParty/http-parser ${OPENSSL_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})

set(NETKIT_SOURCE 
		NKAddress.cpp
		NKBase64.cpp
		NKComponent.cpp
		NKDatabase_SQLite.cpp
		NKDatabase_SQLite.h
		NKEndpoint.cpp
		NKError.cpp
		NKHTTP.cpp
		NKJSON.cpp
		NKLog.cpp
		NKMIME.cpp
		NKNetworkInterface.cpp
		NKOAuth.cpp
		NKObject.cpp
		NKProxy.cpp
		NKSHA1.cpp
		NKSink.cpp
		NKSocket.cpp
		NKSource.cpp
		NKTLS.cpp
		NKURI.cpp
		NKUUID.cpp
		NKUnicode.cpp
		NKWebSocket.cpp
		NKXMPP.cpp
		Linux/NKLog_Linux.cpp
		Linux/NKPlatform_Linux.cpp
		Linux/NKRunLoop_Linux.cpp
		../ThirdParty/http-parser/http_parser.c
)

add_library (NetKit SHARED ${NETKIT_SOURCE})

target_link_libraries (NetKit ${LIBXML2_LIBRARIES} ${OPENSSL_LIBRARIES})
