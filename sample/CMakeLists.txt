project (log_c_sdk_sample)

include_directories(${CMAKE_SOURCE_DIR})

set(POST_LOGS_SOURCE_FILES log_config.c
			    log_post_logs_sample.c)


set(PRODUCER_SAMPLE_SOURCE_FILES log_producer_sample.c)

set(PRODUCER_BENCHMARK_SOURCE_FILES log_producer_benchmark.c)

set(PRODUCER_FRAME_SOURCE_FILES video_frame_producer_sample.c)

include_directories (${CURL_INCLUDE_DIR})
include_directories ("${CMAKE_SOURCE_DIR}/src")

find_library(CURL_LIBRARY curl)
find_library(PTHREAD_LIBRARY pthread)
find_library(RT_LIBRARY rt)
find_library(M_LIBRARY m)
#find_library(SSL_LIBRARY ssl)
#find_library(CRYPTO_LIBRARY crypto)
find_library(ZLIB_LIBRARY z)
find_library(DL_LIBRARY dl)

function(_TARGET_SAMPLE_LIBRARIES SAMPLE_BIN_NAME SOURCE_FILES)
	 add_executable(${SAMPLE_BIN_NAME} ${SOURCE_FILES})
	 target_link_libraries(${SAMPLE_BIN_NAME} log_c_sdk_static)
	 target_link_libraries(${SAMPLE_BIN_NAME} ${CURL_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${PTHREAD_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${M_LIBRARY})
	 target_link_libraries(${SAMPLE_BIN_NAME} ${DL_LIBRARY})
     #target_link_libraries(${SAMPLE_BIN_NAME} ${SSL_LIBRARY})
     #target_link_libraries(${SAMPLE_BIN_NAME} ${CRYPTO_LIBRARY})
     target_link_libraries(${SAMPLE_BIN_NAME} ${ZLIB_LIBRARY})
endfunction()

_TARGET_SAMPLE_LIBRARIES(log_producer_sample "${PRODUCER_SAMPLE_SOURCE_FILES}")

_TARGET_SAMPLE_LIBRARIES(log_post_logs_sample "${POST_LOGS_SOURCE_FILES}")

_TARGET_SAMPLE_LIBRARIES(log_producer_benchmark "${PRODUCER_BENCHMARK_SOURCE_FILES}")