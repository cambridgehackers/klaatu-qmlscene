LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	qtklaatu-qmlscene-dependencyForcer.c

SVERSION:=$(subst ., ,$(PLATFORM_VERSION))
SHORT_PLATFORM_VERSION=$(word 1,$(SVERSION))$(word 2,$(SVERSION))
LOCAL_CFLAGS += -DSHORT_PLATFORM_VERSION=$(SHORT_PLATFORM_VERSION)

ifeq ($(SHORT_PLATFORM_VERSION),40)
	QMAKE_ARGS=CONFIG+=KLAATU_OLDLIBS
else
	LOCAL_SHARED_LIBRARIES := libsuspend
endif

ifeq ($(TARGET_OS),linux)
	LOCAL_CFLAGS += -DXP_UNIX
#LOCAL_SHARED_LIBRARIES += librt
endif

ifeq (panda,$(TARGET_DEVICE))
QMAKE_ARGS += CONFIG+=KLAATU_MOUSE
QMAKE_ARGS += CONFIG+=TARGET_DEVICE_PANDA
endif

LOCAL_MODULE:= qtklaatu-qmlscene-dependencyForcer
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := EXECUTABLES
# this line makes sure we build AFTER the base Qt libraries
LOCAL_ADDITIONAL_DEPENDENCIES := $(TARGET_OUT_EXECUTABLES)/qtbase-dependencyForcer 
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_EXECUTABLES)/qtjsbackend-dependencyForcer
LOCAL_ADDITIONAL_DEPENDENCIES += $(TARGET_OUT_EXECUTABLES)/qtdeclarative-dependencyForcer

intermediates:= $(local-intermediates-dir)

GEN := $(LOCAL_PATH)/qtklaatu-qmlscene-dependencyForcer.c
$(GEN): PRIVATE_INPUT_FILE := $(LOCAL_PATH)/AndroidQtBuild.sh
$(GEN): PRIVATE_CUSTOM_TOOL = bash $(PRIVATE_INPUT_FILE) $@ $(QMAKE_ARGS)
$(GEN): $(LOCAL_PATH)/AndroidQtBuild.sh 
	$(transform-generated-source)
$(GEN):$(TARGET_OUT_EXECUTABLES)/qtdeclarative-dependencyForcer 
.PHONY: $(GEN)
#LOCAL_GENERATED_SOURCES += $(GEN)



include $(BUILD_EXECUTABLE)
