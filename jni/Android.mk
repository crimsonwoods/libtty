LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE      := ptrb
LOCAL_CPPFLAGS    := -Werror -Wall -O2 -std=gnu++0x
LOCAL_SRC_FILES   := ptrb.cpp tty.cpp
LOCAL_LDLIBS      := -llog

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE      := tty
LOCAL_CPPFLAGS    := -Werror -Wall -O2
LOCAL_SRC_FILES   := tty.cpp
LOCAL_LDLIBS      := -llog

include $(BUILD_SHARED_LIBRARY)
