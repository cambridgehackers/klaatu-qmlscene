/*
  Wrapper for an Android InputReaderThread object to
  dispatche events into the Qt event system
 */

#ifndef _EVENT_THREAD_H
#define _EVENT_THREAD_H

#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 23)
#include "ui/EventHub.h"
#include "ui/InputReader.h"
#define DISPATCH_CLASS InputDispatcherInterface
#else
#include "EventHub.h"
#include "InputReader.h"
#define DISPATCH_CLASS InputListenerInterface
#endif
#include <private/qguiapplication_p.h>

class ScreenControl;

class EventThread : public android::InputReaderThread {

public:
    EventThread(ScreenControl *s);

private:
    android::sp<android::EventHub> mHub;
};

#endif // _EVENT_THREAD_H
