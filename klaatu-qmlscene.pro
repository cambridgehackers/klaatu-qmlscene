TARGET = klaatu_qmlscene
QT += qml core-private gui-private quick
DESTDIR = ../bin

SOURCES = \
    qmlscene_main.cpp \
    screencontrol.cpp \
    event_thread.cpp \
    audiocontrol.cpp \
    lights.cpp \
    battery.cpp \
    inputcontext.cpp \
    power.cpp \
    command.cpp \
    settings.cpp \
    sensors.cpp \
    screenorientation.cpp \
    wifi.cpp \
    callmodel.cpp \
    klaatuapplication.cpp

HEADERS = \
    screencontrol.h \
    audiocontrol.h \
    event_thread.h \
    lights.h \
    battery.h \
    inputcontext.h \
    power.h \
    command.h \
    settings.h \
    sensors.h \
    screenorientation.h \
    wifi.h \
    sensor.h \
    callmodel.h \
    klaatuapplication.h \
    cursorsignal.h

ATOP=$$(ANDROID_BUILD_TOP)
isEmpty(ATOP) {
   error("Must define ANDROID_BUILD_TOP")
}

message("Using android build top $$(ANDROID_BUILD_TOP)")
INCLUDEPATH += ${ANDROID_BUILD_TOP}/frameworks/base/services/input
INCLUDEPATH += ${ANDROID_BUILD_TOP}/external/skia/include/core
INCLUDEPATH += ${ANDROID_BUILD_TOP}/external/klaatu-services/include
INCLUDEPATH += ${ANDROID_BUILD_TOP}/system/core/libsuspend/include
INCLUDEPATH += ${ANDROID_BUILD_TOP}/frameworks/av/include

LIBS += -lmedia -lklaatu_phone -lhardware -lhardware_legacy -linput -lnetutils -lklaatu_wifi -lklaatu_sensors

contains (CONFIG, KLAATU_OLDLIBS) {
    LIBS += -lui
} else {
    LIBS += -landroidfw -lsuspend
}

contains (CONFIG, KLAATU_INPUT_SERVICE) {
    LIBS += -linputservice
}

MOC_DIR=.moc
OBJECTS_DIR=.obj

mac: CONFIG -= app_bundle

target.path = $$[QT_INSTALL_BINS]

INSTALLS += target
