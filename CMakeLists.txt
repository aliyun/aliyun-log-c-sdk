PROJECT(log_c_sdk)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)  

set(CMAKE_VERSION 2.0.0)

# default C / CXX flags
set(CMAKE_C_FLAGS " -g -ggdb -O0 -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS " -g -ggdb -O0 -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_DEBUG " -g -ggdb -O0 -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_DEBUG " -g -ggdb -O0 -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_RELEASE " -O3 -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_RELEASE " -O3 -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_MINSIZEREF " -Os -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_MINSIZEREF " -Os -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_RELWITHDEBINFO " -O2 -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O2 -Wall -Werror -fpic -fPIC -D_LARGEFILE64_SOURCE")

set(CMAKE_C_FLAGS_COVERAGE " ${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_FLAGS_COVERAGE " ${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverag")

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/bin)
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

FIND_PROGRAM(APR_CONFIG_BIN NAMES apr-config apr-1-config PATHS /usr/bin /usr/local/bin /usr/local/apr/bin/)
FIND_PROGRAM(APU_CONFIG_BIN NAMES apu-config apu-1-config PATHS /usr/bin /usr/local/bin /usr/local/apr/bin/)

IF (APR_CONFIG_BIN)
    EXECUTE_PROCESS(
        COMMAND ${APR_CONFIG_BIN} --includedir
        OUTPUT_VARIABLE APR_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${APR_CONFIG_BIN} --cflags
        OUTPUT_VARIABLE APR_C_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${APR_CONFIG_BIN} --link-ld
        OUTPUT_VARIABLE APR_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ELSE()
    MESSAGE(FATAL_ERROR "Could not find apr-config/apr-1-config")
ENDIF()

IF (APU_CONFIG_BIN)
    EXECUTE_PROCESS(
        COMMAND ${APU_CONFIG_BIN} --includedir
        OUTPUT_VARIABLE APR_UTIL_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${APU_CONFIG_BIN} --cflags
        OUTPUT_VARIABLE APU_C_FLAGS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    EXECUTE_PROCESS(
        COMMAND ${APU_CONFIG_BIN} --link-ld
        OUTPUT_VARIABLE APU_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ELSE()
    MESSAGE(FATAL_ERROR "Could not find apu-config/apu-1-config")
ENDIF()

#curl-config
FIND_PROGRAM(CURL_CONFIG_BIN NAMES curl-config)
  
IF (CURL_CONFIG_BIN)
    EXECUTE_PROCESS(
        COMMAND ${CURL_CONFIG_BIN} --libs
        OUTPUT_VARIABLE CURL_LIBRARIES
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )
ELSE()
    MESSAGE(FATAL_ERROR "Could not find curl-config")
ENDIF()
  
# Compile and link lib_log_c_sdk
include_directories(${APR_INCLUDE_DIR})
include_directories(${APR_UTIL_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIR})

aux_source_directory(src SRC_LIST)

add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC_LIST})
add_library(${CMAKE_PROJECT_NAME}_static STATIC ${SRC_LIST})

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VERSION ${CMAKE_VERSION}  SOVERSION ${CMAKE_VERSION})

INSTALL(TARGETS ${CMAKE_PROJECT_NAME} LIBRARY DESTINATION lib)

INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_static ARCHIVE DESTINATION lib)

INSTALL(FILES
  src/cJSON.h
  src/lz4.h
  src/aos_buf.h
  src/aos_define.h
  src/aos_fstack.h
  src/aos_http_io.h
  src/aos_list.h
  src/aos_log.h
  src/aos_status.h
  src/aos_string.h
  src/aos_transport.h
  src/aos_util.h
  src/log_define.h
  src/log_util.h
  src/log_api.h
  src/log_http_cont.h
  src/log_builder.h
  src/protobuf-c.h
  src/log_logstores.pb-c.h
  DESTINATION include/log_c_sdk)

add_subdirectory(sample)
