LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

SVERSION:=$(subst ., ,$(PLATFORM_VERSION))
SHORT_PLATFORM_VERSION=$(word 1,$(SVERSION))$(word 2,$(SVERSION))
LOCAL_CFLAGS += -DSHORT_PLATFORM_VERSION=$(SHORT_PLATFORM_VERSION)

ifeq ($(SHORT_PLATFORM_VERSION),40)
	QMAKE_ARGS += CONFIG+=KLAATU_OLDLIBS
else
	LOCAL_SHARED_LIBRARIES := libsuspend
endif
QMAKE_ARGS += DEFINES+=SHORT_PLATFORM_VERSION=$(SHORT_PLATFORM_VERSION)


ifeq (panda,$(TARGET_DEVICE))
QMAKE_ARGS += CONFIG+=TARGET_DEVICE_PANDA
endif

LOCAL_MODULE:= klaatu_qmlscene
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := EXECUTABLES

LOCAL_SHARED_LIBRARIES += \
	libQtCore \
	libQtV8 \
	libQtQml \
	libQtQuick \
	libklaatu_phone \
	libinput \
	libklaatu_wifi \
	libklaatu_sensors

ifeq ($(SHORT_PLATFORM_VERSION),44)
LOCAL_SHARED_LIBRARIES += libinputservice
QMAKE_ARGS += CONFIG+=KLAATU_INPUT_SERVICE
endif

ALL_DEFAULT_INSTALLED_MODULES += \
	sensorservice \
	rild \
	mediaserver \
	netd \
	wpa_supplicant \
	wpa_supplicant.conf \
	dhcpcd \
	dhcpcd-run-hooks \
	95-configured \
	20-dns.conf \
	audio.primary.default \
	audio_policy.default \
	local_time.default

include $(BUILD_SYSTEM)/binary.mk

file := $(LOCAL_PATH)/Makefile
$(file): where := $(LOCAL_PATH)
$(file) : $(all_libraries)
	cd $(where); $(ANDROID_HOST_OUT)/bin/qmake $(QMAKE_ARGS)

file := $(LOCAL_INSTALLED_MODULE)
$(file): where := $(LOCAL_PATH)
$(file): $(LOCAL_PATH)/Makefile $(all_libraries)
	make -C $(where)
	make -C $(where) install

# Force this file to get installed
ALL_DEFAULT_INSTALLED_MODULES += $(LOCAL_MODULE)
