PROJECT(aliyun_log_c_sdk)

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

option(ADD_LOG_KEY_VALUE_FUN "ON for generating add log key&value functions" OFF)
option(HTTP_USE_CURL "http use libcurl" ON)

if(HTTP_USE_CURL)
    add_definitions(-DLOG_HTTP_USE_CURL)
    aux_source_directory(src/curl_adapter CURL_ADAPTER_SRCS)
ENDIF(HTTP_USE_CURL)

IF(ADD_LOG_KEY_VALUE_FUN)
    add_definitions(-DLOG_KEY_VALUE_FLAG)
ENDIF(ADD_LOG_KEY_VALUE_FUN)

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
include_directories(${CURL_INCLUDE_DIR})
LINK_LIBRARIES(curl)

aux_source_directory(src SRC_LIST)

add_library(${CMAKE_PROJECT_NAME} STATIC ${SRC_LIST} ${CURL_ADAPTER_SRCS})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC src src/curl_adapter)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VERSION ${CMAKE_VERSION}  SOVERSION ${CMAKE_VERSION})

function(_TARGET_SAMPLE_LIBRARIES SAMPLE_BIN_NAME SOURCE_FILES)
    add_executable(${SAMPLE_BIN_NAME} ${SOURCE_FILES})
    target_link_libraries(${SAMPLE_BIN_NAME} ${CMAKE_PROJECT_NAME} pthread)
endfunction()

set(POST_LOGS_SOURCE_FILES
    sample/log_config.c
	sample/log_post_logs_sample.c)
set(PRODUCER_SAMPLE_SOURCE_FILES sample/log_producer_sample.c)
set(PRODUCER_BENCHMARK_SOURCE_FILES sample/log_producer_benchmark.c)
set(PRODUCER_PERSISTENT_SAMPLE_SOURCE_FILES sample/log_producer_persistent_sample.c)

_TARGET_SAMPLE_LIBRARIES(log_producer_sample "${PRODUCER_SAMPLE_SOURCE_FILES}")
_TARGET_SAMPLE_LIBRARIES(log_post_logs_sample "${POST_LOGS_SOURCE_FILES}")
_TARGET_SAMPLE_LIBRARIES(log_producer_persistent_sample "${PRODUCER_PERSISTENT_SAMPLE_SOURCE_FILES}")
_TARGET_SAMPLE_LIBRARIES(log_producer_benchmark "${PRODUCER_BENCHMARK_SOURCE_FILES}")