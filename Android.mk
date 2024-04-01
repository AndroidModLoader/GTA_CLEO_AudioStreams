LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPP_EXTENSION := .cpp .cc
LOCAL_MODULE    := CLEO_AudioStreams
LOCAL_SRC_FILES := main.cpp audiosystem.cpp mod/logger.cpp
LOCAL_CFLAGS += -O2 -mfloat-abi=softfp -DNDEBUG -std=c++17
LOCAL_LDLIBS += -llog # ARM64 library requires it so...
include $(BUILD_SHARED_LIBRARY)