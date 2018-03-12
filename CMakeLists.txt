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

option(ADD_LOG_KEY_VALUE_FUN "ON for generating add log key&value functions" OFF)

IF(ADD_LOG_KEY_VALUE_FUN)
    message("[ON] open ADD_LOG_KEY_VALUE_FUN")
    add_definitions(-DLOG_KEY_VALUE_FLAG)
ENDIF(ADD_LOG_KEY_VALUE_FUN)

aux_source_directory(src SRC_LIST)

# add_definitions(-DUSE_LZ4_FLAG)
option(WITH_LZ4 "ON for generating lz4 log pack functions" OFF)
IF(WITH_LZ4)
    message("[ON] open WITH_LZ4")
    add_definitions(-DUSE_LZ4_FLAG)
ELSE(WITH_LZ4)
    list(REMOVE_ITEM SRC_LIST src/lz4.c)
ENDIF(WITH_LZ4)

# message('src list ' ${SRC_LIST})

add_library(${CMAKE_PROJECT_NAME} SHARED ${SRC_LIST})
add_library(${CMAKE_PROJECT_NAME}_static STATIC ${SRC_LIST})

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES VERSION ${CMAKE_VERSION}  SOVERSION ${CMAKE_VERSION})

INSTALL(TARGETS ${CMAKE_PROJECT_NAME} LIBRARY DESTINATION lib)

INSTALL(TARGETS ${CMAKE_PROJECT_NAME}_static ARCHIVE DESTINATION lib)

INSTALL(FILES
        src/lz4.h
        src/log_define.h
        src/log_api.h
        src/log_builder.h
        src/sds.h
        src/log_util.h
        DESTINATION include/log_c_sdk)

option(WITH_SAMPLE "ON for compile sample, this need libcurl" ON)
IF(WITH_SAMPLE)
    add_subdirectory(sample)
ENDIF(WITH_SAMPLE)