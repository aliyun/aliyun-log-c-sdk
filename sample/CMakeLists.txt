project (log_c_sdk_sample)

include_directories(${CMAKE_SOURCE_DIR})

set(POST_LOGS_SOURCE_FILES log_config.c 
			    log_post_logs_sample.c)

include_directories (${APR_INCLUDE_DIR})
include_directories (${APR_UTIL_INCLUDE_DIR})
include_directories (${CURL_INCLUDE_DIR})
include_directories ("${CMAKE_SOURCE_DIR}/src")

find_library(APR_LIBRARY apr-1 PATHS /usr/local/apr/lib/)
find_library(APR_UTIL_LIBRARY aprutil-1 PATHS /usr/local/apr/lib/)
find_library(CURL_LIBRARY curl)
find_library(PTHREAD_LIBRARY pthread)
find_library(RT_LIBRARY rt)
find_library(M_LIBRARY m)

function(_TARGET_SAMPLE_LIBRARIES SAMPLE_BIN_NAME SOURCE_FILES)
	 add_executable(${SAMPLE_BIN_NAME} ${SOURCE_FILES})
	 target_link_libraries(${SAMPLE_BIN_NAME} log_c_sdk)
	 target_link_libraries(${SAMPLE_BIN_NAME} ${APR_UTIL_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${APR_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${CURL_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${PTHREAD_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${RT_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${M_LIBRARY})
endfunction()

_TARGET_SAMPLE_LIBRARIES(log_post_logs_sample "${POST_LOGS_SOURCE_FILES}")
