/*
  Thread wrapper for an Android EventHub object which
  reads from /dev/input/X and dispatches the events into
  the Qt event system
 */

#ifndef _EVENT_THREAD_H
#define _EVENT_THREAD_H

#include <input/EventHub.h>
#include <QThread>
#include <QMap>

class InputDevice;

class EventThread : public QThread {
    Q_OBJECT

public:
    void run();

signals:
    void userActivity();
    void powerKey(int value);

private:
    void addDevice(int32_t id);
    void removeDevice(int32_t id);

private:
    QMap<int32_t, InputDevice *>   mDeviceMap;
    android::sp<android::EventHub> mHub;
};

#endif // _EVENT_THREAD_H
