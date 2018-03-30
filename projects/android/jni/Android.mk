LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := libaliyun_log_c_sdk

include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    /usr/local/include

MY_C_LIST := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_SRC_FILES := $(MY_C_LIST:$(LOCAL_PATH)/%=%)

LOCAL_MODULE := libaliyun_log_c_sdk

include $(BUILD_STATIC_LIBRARY)
