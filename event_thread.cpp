#include "event_thread.h"
#include <input/EventHub.h>

#include <private/qguiapplication_p.h>
#include <QDebug>
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#define CODE_FIELD scanCode
#else
#define CODE_FIELD code
#endif

using namespace android;
// --------------------------------------------------------------------------------

class InputDevice 
{
public:
    InputDevice(const QByteArray& name)
	: mName(name) {}

    virtual ~InputDevice() {}

    virtual void processEvent(const RawEvent& event) = 0;
			       
protected:
    QByteArray mName;
};

// --------------------------------------------------------------------------------

class KeyboardDevice : public InputDevice
{
public: 
    KeyboardDevice(const QByteArray& name) : InputDevice(name) {}

    virtual void processEvent(const RawEvent& event) {
#ifdef NEXUS_INPUT_DEBUG
	qDebug("Keyboard event: time %lld type=%u code=%u value=%d ", 
	       event.when, event.type, event.CODE_FIELD, event.value);
#endif

	if (event.type != EV_KEY)
	    return;

	quint16 code = event.CODE_FIELD;
	qint32 value = event.value;

	int keycode = 0;
	switch (code) {
	case KEY_VOLUMEDOWN:
	    keycode = Qt::Key_VolumeDown;
	    break;
	case KEY_VOLUMEUP:
	    keycode = Qt::Key_VolumeUp;
	    break;
	case KEY_POWER:
	    keycode = Qt::Key_PowerOff;
	    break;
	default:
	    qWarning("Keyboard Device: unrecognized keycode=%d", code);
	    break;
	}

	if (keycode != 0)
	    QWindowSystemInterface::handleKeyEvent(0, (value ? QEvent::KeyPress : QEvent::KeyRelease),
						   keycode, 0, QString(), false);
    }
};

// --------------------------------------------------------------------------------

class TouchscreenDevice : public InputDevice
{
public:
    TouchscreenDevice(const QByteArray& name, const RawAbsoluteAxisInfo& axis_x, const RawAbsoluteAxisInfo& axis_y);
    virtual void processEvent(const RawEvent& event);

private:
    Qt::TouchPointStates calculateTouchPoints();
    void reportPoints();

private:

    struct Contact {
        int tracking_id, x, y, maj, pressure, true_x, true_y;
	Contact() : tracking_id(-1), x(0), y(0), maj(1), pressure(0), true_x(0), true_y(0) { }
    };

    QList<QWindowSystemInterface::TouchPoint> mTouchPoints;
    QHash<int, Contact> mContacts, mLastContacts; // Map from slot ID to Contact
    int                 mCurrentSlot;
    bool                mCurrentModified;
    Contact             mCurrentData;
    QTouchDevice       *mDevice;
    RawAbsoluteAxisInfo mAxisX, mAxisY;
    int                 mHwPressureMax;
};


TouchscreenDevice::TouchscreenDevice(const QByteArray& name, 
				     const RawAbsoluteAxisInfo& axis_x, 
				     const RawAbsoluteAxisInfo& axis_y)
    : InputDevice(name)
    , mCurrentSlot(0)
    , mCurrentModified(false)
    , mAxisX(axis_x)
    , mAxisY(axis_y)
    , mHwPressureMax(200)  // Should use "touch.pressure.scale
{
    mDevice = new QTouchDevice;
    mDevice->setName(name);
    mDevice->setType(QTouchDevice::TouchScreen);
    mDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure);
    QWindowSystemInterface::registerTouchDevice(mDevice);
}

/*
  Nexus Protocol:

  First event:
     EV_ABS ABS_MT_SLOT          (optional)
            ABS_MT_TRACKING_ID               Always incremented by one from last time
	    ABS_MT_TOUCH_MAJOR   (optional)
	    ABS_MT_PRESSURE
	    ABS_MT_POSITION_X
	    ABS_MT_POSITION_Y
     SYN_REPORT

  When ABS_MT_SLOT is specified, it is followed with a new ABS_MT_TRACKING_ID.
  If you get a new ABS_MT_TRACKING_ID, it goes to whatever slot was last specified.
  The ABS_MT_SLOT preceeds the touch/pressure/x/y data.

  Additional events

     EV_ABS ABS_MT_SLOT           (optional)
            ABS_MT_TRACKING_ID    (optional)
	    ABS_MT_PRESSURE       (optional)
	    ABS_MT_POSITION_X     (optional)
	    ABS_MT_POSITION_Y     (optional)

   Final event
   
     EV_ABS ABS_MT_TRACKING_ID   value -1
     SYN_REPORT

   
  A touch end event is indicated by setting the ABS_MT_TRACKING_ID to -1.
  (which says that the last ABS_MT_SLOT has been lifted).
 */

void TouchscreenDevice::processEvent(const RawEvent& event)
{
    if (event.type == EV_ABS) {
	if (event.CODE_FIELD == ABS_MT_SLOT) {
	    if (mCurrentModified)
		mContacts.insert(mCurrentSlot, mCurrentData);
	    mCurrentModified = false;
	    mCurrentSlot = event.value;
	    mCurrentData = mContacts.value(mCurrentSlot);  // Default constructor
	}
	else {
	    mCurrentModified = true;
	    if (event.CODE_FIELD == ABS_MT_POSITION_X) {
		mCurrentData.true_x = event.value;
		mCurrentData.x = qBound(mAxisX.minValue, event.value, mAxisX.maxValue);
	    } else if (event.CODE_FIELD == ABS_MT_POSITION_Y) {
		mCurrentData.true_y = event.value;
		mCurrentData.y = qBound(mAxisY.minValue, event.value, mAxisY.maxValue);
	    } else if (event.CODE_FIELD == ABS_MT_TRACKING_ID) {
		mCurrentData.tracking_id = event.value;  // Value of -1 would be a release
	    } else if (event.CODE_FIELD == ABS_MT_PRESSURE) {
		mCurrentData.pressure = qBound(0, event.value, mHwPressureMax);
		if (!mCurrentData.pressure && mCurrentData.tracking_id == 0)	// tracking_id 0 means that only one finger is touching
			mCurrentData.tracking_id = -1;  // Value of -1 would be a release
	    } else if (event.CODE_FIELD == ABS_MT_TOUCH_MAJOR) {
		mCurrentData.maj = event.value;
	    }
	}
    } else if (event.type == EV_SYN && event.CODE_FIELD == SYN_REPORT) {
#ifdef NEXUS_INPUT_DEBUG
	qDebug() << "SYN_REPORT modified=" << mCurrentModified;
#endif
	if (mCurrentModified)
	    mContacts.insert(mCurrentSlot, mCurrentData);
	mCurrentModified = false;

	Qt::TouchPointStates states = calculateTouchPoints();
	if (!mTouchPoints.isEmpty() && states != Qt::TouchPointStationary)
	    reportPoints();
    }
}

Qt::TouchPointStates TouchscreenDevice::calculateTouchPoints()
{
    mTouchPoints.clear();
    Qt::TouchPointStates combinedStates = 0;

    QMutableHashIterator<int, Contact> it(mContacts);
    while (it.hasNext()) {
	it.next();
	QWindowSystemInterface::TouchPoint tp;
	Contact& contact(it.value());

	int slot = it.key();
	tp.id    = slot;   // Use the slot as the ID
	tp.flags = 0;      // We are not a pen

	if (contact.tracking_id == -1) {
	    tp.state = Qt::TouchPointReleased;
	}
	else if (mLastContacts.contains(slot)) {
	    const Contact &prev(mLastContacts.value(slot));
	    tp.state = ((prev.x == contact.x && prev.y == contact.y)
			? Qt::TouchPointStationary : Qt::TouchPointMoved);
	} else {
	    tp.state = Qt::TouchPointPressed;
	}

	combinedStates |= tp.state;

	// Store the HW coordinates for now, will be updated later.
	tp.area = QRectF(0, 0, contact.maj, contact.maj);
	tp.area.moveCenter(QPoint(contact.x, contact.y));
	tp.pressure = contact.pressure;
	
	// Get a normalized position in range 0..1.
	tp.normalPosition = QPointF((contact.x - mAxisX.minValue) / qreal(mAxisX.maxValue - mAxisX.minValue),
				    (contact.y - mAxisY.minValue) / qreal(mAxisY.maxValue - mAxisY.minValue));
	mTouchPoints.append(tp);

	// Remote touch points that have been released
	if (tp.state == Qt::TouchPointReleased)
	    it.remove();
    }

    mLastContacts = mContacts;
    return combinedStates;
}

void TouchscreenDevice::reportPoints()
{
    QRect winRect = QGuiApplication::primaryScreen()->geometry();
    const int hw_w = mAxisX.maxValue - mAxisX.minValue;
    const int hw_h = mAxisY.maxValue - mAxisY.minValue;

    // Map the coordinates based on the normalized position. QPA expects 'area'
    // to be in screen coordinates.
    const int pointCount = mTouchPoints.count();
    for (int i = 0; i < pointCount; ++i) {
        QWindowSystemInterface::TouchPoint &tp(mTouchPoints[i]);

        // Generate a screen position that is always inside the active window
        // or the primary screen.
        const int wx = winRect.left() + int(tp.normalPosition.x() * (winRect.width()-1));
        const int wy = winRect.top() + int(tp.normalPosition.y() * (winRect.height()-1));
        const qreal sizeRatio = (winRect.width() + winRect.height()) / qreal(hw_w + hw_h);
        tp.area = QRect(0, 0, tp.area.width() * sizeRatio, tp.area.height() * sizeRatio);
        tp.area.moveCenter(QPoint(wx, wy));

        // Calculate normalized pressure.
	// ### TODO:  Use configuration scaling
	tp.pressure = tp.pressure / qreal(mHwPressureMax);
    }

#ifdef NEXUS_INPUT_DEBUG
    qDebug() << "Reporting touch events" << mTouchPoints.count();
    for (int i = 0 ; i < pointCount ; i++) {
	QWindowSystemInterface::TouchPoint& tp(mTouchPoints[i]);
	qDebug() << "  " << tp.id << tp.area << (int) tp.state << tp.normalPosition;
    }
#endif

    QWindowSystemInterface::handleTouchEvent(0, mDevice, mTouchPoints);
}

// --------------------------------------------------------------------------------

void EventThread::run()
{
    mHub = new EventHub();
    const int kRawBufferSize = 32;
    RawEvent buffer[kRawBufferSize];
    
    while (1) {
	size_t n = mHub->getEvents(10000, buffer, kRawBufferSize);
	for (size_t i = 0 ; i < n ; i++) {
	    const RawEvent& ev(buffer[i]);
	    // printf("%lld device_id=%d type=0x%x scanCode=%d keyCode=%d value=%d flags=%u\n", 
	    // ev.when, ev.deviceId, ev.type, ev.CODE_FIELD, ev.keyCode, ev.value, ev.flags);
	
	    if (buffer[i].type < EventHubInterface::DEVICE_ADDED) {
		InputDevice *d = mDeviceMap.value(buffer[i].deviceId);
		if (d)
		    d->processEvent(buffer[i]);
		if (ev.type != 0) {
		    if (ev.type == 1 && ev.CODE_FIELD == KEY_POWER)
			emit powerKey(ev.value);
		    else
			emit userActivity();
		}
	    } 
	    else { // Process change in available devices
		if (ev.type == EventHubInterface::DEVICE_ADDED) 
		    addDevice(ev.deviceId);
		else if (buffer[i].type == EventHubInterface::DEVICE_REMOVED)
		    removeDevice(ev.deviceId);
		// Should also check for configuration changes...
	    }
	}
    }
}

void EventThread::addDevice(int32_t id)
{
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
    InputDeviceIdentifier identifier;
#else
    InputDeviceIdentifier identifier = mHub->getDeviceIdentifier(id);
#endif
    uint32_t klass = mHub->getDeviceClasses(id);
    if (klass & INPUT_DEVICE_CLASS_TOUCH_MT) {
	RawAbsoluteAxisInfo axis_x, axis_y;
	if (mHub->getAbsoluteAxisInfo(id, ABS_MT_POSITION_X, &axis_x) != NO_ERROR ||
	    mHub->getAbsoluteAxisInfo(id, ABS_MT_POSITION_Y, &axis_y) != NO_ERROR)
	    qWarning() << "Unable to extract axis information for touchscreen";
	else
	    mDeviceMap.insert(id, new TouchscreenDevice(identifier.name.string(), axis_x, axis_y));
    }
    else if (klass & INPUT_DEVICE_CLASS_KEYBOARD)
	mDeviceMap.insert(id, new KeyboardDevice(identifier.name.string()));
    else
	qWarning() << "Unable to find suitable class for id=" << id 
		   << "name" << identifier.name.string();
}

void EventThread::removeDevice(int32_t id)
{
    InputDevice *device = mDeviceMap.take(id);
    if (device)
	delete device;
}

