#ifndef _CURSOR_SIGNAL_H
#define _CURSOR_SIGNAL_H
#include "event_thread.h"
#include <EventHub.h>
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION < 41)
#include <ui/KeycodeLabels.h>
#else
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION < 44)
#include <androidfw/KeycodeLabels.h>
#else
#include <input/KeycodeLabels.h>
#endif
#endif
#include <android/keycodes.h>
#include "screencontrol.h"
#include "sys/ioctl.h"
#include "linux/fb.h"

class CursorSignal: public QObject {
    Q_OBJECT
    signals:
    void showMouse(bool show);
};
#endif // _CURSOR_SIGNAL_H
