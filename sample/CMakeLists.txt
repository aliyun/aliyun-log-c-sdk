project (log_c_sdk_sample)

include_directories(${CMAKE_SOURCE_DIR})

set(POST_LOGS_SOURCE_FILES log_config.c
		log_post_logs_sample.c
		hmac-sha.c
		md5.c
		sha1.c
		log_util_imp.c)


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

include_directories (${CURL_INCLUDE_DIR})

include_directories ("${CMAKE_SOURCE_DIR}/src")

find_library(CURL_LIBRARY curl)
find_library(PTHREAD_LIBRARY pthread)
find_library(RT_LIBRARY rt)
find_library(M_LIBRARY m)

function(_TARGET_SAMPLE_LIBRARIES SAMPLE_BIN_NAME SOURCE_FILES)
	 add_executable(${SAMPLE_BIN_NAME} ${SOURCE_FILES})
	 target_link_libraries(${SAMPLE_BIN_NAME} log_c_sdk_static)
	 target_link_libraries(${SAMPLE_BIN_NAME} ${CURL_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${PTHREAD_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${M_LIBRARY})
endfunction()

_TARGET_SAMPLE_LIBRARIES(log_post_logs_sample "${POST_LOGS_SOURCE_FILES}")
