PROJECT(aliyun_log_c_sdk)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)  

set(CMAKE_VERSION 2.0.0)
add_definitions(-DWIN32)
# default C / CXX flags
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS "/W3 /D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS "/W3 /D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_DEBUG "/Zi /Od /D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG /D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG /D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_MINSIZEREL "/Os /DNDEBUG /D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_MINSIZEREL "/Os /DNDEBUG /D_LARGEFILE64_SOURCE")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "/Zi /O2 /DNDEBUG /D_LARGEFILE64_SOURCE")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/Zi /O2 /DNDEBUG /D_LARGEFILE64_SOURCE")

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_C_OUTPUT_EXTENSION_REPLACE 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

option(ADD_LOG_KEY_VALUE_FUN "ON for generating add log key&value functions" OFF)
option(HTTP_USE_CURL "http use libcurl" ON)

IF(ADD_LOG_KEY_VALUE_FUN)
    add_definitions(-DLOG_KEY_VALUE_FLAG)
ENDIF(ADD_LOG_KEY_VALUE_FUN)

if(HTTP_USE_CURL)
    add_definitions(-DLOG_HTTP_USE_CURL)
ENDIF(HTTP_USE_CURL)

# Compile and link lib_log_c_sdk
aux_source_directory(src SRC_LIST)
aux_source_directory(src/curl_adapter CURL_ADAPTER_SRCS)
add_library(${CMAKE_PROJECT_NAME} STATIC ${SRC_LIST} ${CURL_ADAPTER_SRCS})
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC src src/curl_adapter)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VERSION ${CMAKE_VERSION} SOVERSION ${CMAKE_VERSION})

# find curl by customized CURL_LIB_DIR & CURL_INCLUDE_DIRS
if(CURL_LIB_DIR)
    message(STATUS "Use custom libcurl, CURL_LIB_DIR: ${CURL_LIB_DIR}")
    message(STATUS "CURL_INCLUDE_DIRS: ${CURL_INCLUDE_DIRS}")
    find_library(CURL_LIBRARY
                NAMES curl libcurl libcurl_debug
                PATHS ${CURL_LIB_DIR}
                NO_DEFAULT_PATH)
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CURL_LIBRARY})
else()
    find_library(CURL CONFIG)
    if (CURL_FOUND)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE CURL::libcurl)
    else()
        find_library(CURL MODULE REQUIRED)
        target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${CURL_LIBRARIES})
    endif()
endif()
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CURL_INCLUDE_DIRS})


# For samples
set(POST_LOGS_SOURCE_FILES
    sample/log_config.c
	sample/log_post_logs_sample.c)

set(PRODUCER_SAMPLE_SOURCE_FILES sample/log_producer_sample.c)
set(PRODUCER_BENCHMARK_SOURCE_FILES sample/log_producer_benchmark.c)
set(PRODUCER_PERSISTENT_SAMPLE_SOURCE_FILES sample/log_producer_persistent_sample.c)

function(_TARGET_SAMPLE_LIBRARIES SAMPLE_BIN_NAME SOURCE_FILES)
        add_executable(${SAMPLE_BIN_NAME} ${SOURCE_FILES})
        target_link_libraries(${SAMPLE_BIN_NAME} ${CMAKE_PROJECT_NAME})
endfunction()

_TARGET_SAMPLE_LIBRARIES(log_producer_sample "${PRODUCER_SAMPLE_SOURCE_FILES}")
_TARGET_SAMPLE_LIBRARIES(log_post_logs_sample "${POST_LOGS_SOURCE_FILES}")
_TARGET_SAMPLE_LIBRARIES(log_producer_persistent_sample "${PRODUCER_PERSISTENT_SAMPLE_SOURCE_FILES}")
_TARGET_SAMPLE_LIBRARIES(unittest "sample/unittest.c")

