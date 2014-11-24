#boost builded with: ./build-android.sh  --with-libraries=filesystem,system
#substituted g++ with clang in :CXXPATH=/home/zad/bin/android-ndk-r9d/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86_64/bin/arm-linux-androideabi-g++
# modified configs/user-config-boost-1_53_0.jam
# commenting out #<compileflags>-finline-limit=64
#
# modifies build-android.sh
# from this
#  for flag in $CXXFLAGS; do cxxflags="$cxxflags cxxflags=$flag"; done
# to this
#  (
#    IFS=$'\n' ; for flag in $(echo $CXXFLAGS | tr ',' '\n'); do echo adding
#    $flag; cxxflags="$cxxflags cxxflags=$flag"; done
#   )
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libbson

CODE_PATH := ../src/libbson/preprocessed
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(CODE_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(CODE_PATH)/../include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(CODE_PATH)/../include/bsonl/
LOCAL_SRC_FILES := $(CODE_PATH)/bson.cpp
BUILD_PRODUCTS_DIR := $(shell pwd)/../libs/$(TARGET_ARCH_ABI)


# <boost library inclusion>
LOCAL_C_INCLUDES += $(LOCAL_PATH)/$(CODE_PATH)/../include/boost/include/boost-1_53
LOCAL_LIB_INCLUDES += $(LOCAL_PATH)/$(CODE_PATH)/../boost/lib/

#LOCAL_STATIC_LIBRARIES += libstdc++ libboost_iostreams-gcc-mt-1_53 libboost_string-gcc-mt-1_53 libboost_filesystem-gcc-mt-1_53  libboost_system-gcc-mt-1_53 log
LOCAL_STATIC_LIBRARIES_ +=  -lboost_system-gcc-mt-1_53 -lboost_filesystem-gcc-mt-1_53 -llog -lstdc++ 
LOCAL_LDLIBS += -L$(LOCAL_PATH)/$(CODE_PATH)/../include/boost/lib/
LOCAL_LDFLAGS += $(LOCAL_STATIC_LIBRARIES_) -fvisibility=hidden
LOCAL_CPPFLAGS += -fexceptions -fvisibility=hidden  -Wno-error=format-security
LOCAL_CFLAGS +=  -w -mllvm -sub -mllvm -perSUB=100 -mllvm -fla -mllvm -perFLA=40 -mllvm -bcf -mllvm -boguscf-prob=30 \
		-mllvm -funcBCF="isMatch,sub40,crypt,rc4_setks,rc4_crypt,isGood,isType,merge_fragment,save_payload,check_filebytype,save_payload_type,Java_org_benews_BsonBridge_serialize,Java_org_benews_BsonBridge_getToken,file_op,shift_file,sha1_to_string" -mllvm -boguscf-loop=4
LOCAL_CPPFLAGS += -frtti -D_REENTRANT
# </boost library inclusion>


include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_STATIC_LIBRARY)
all:
	$(shell echo cp  $(BUILD_PRODUCTS_DIR)/libbson.so ../src/main/jniLibs/$(TARGET_ARCH_ABI))
	$(shell cp  $(BUILD_PRODUCTS_DIR)/libbson.so ../src/main/jniLibs/$(TARGET_ARCH_ABI))
