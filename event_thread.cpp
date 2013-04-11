#include "event_thread.h"
#include <EventHub.h>
#include <androidfw/KeycodeLabels.h>
#include <android/keycodes.h>
#include "screencontrol.h"
#include "sys/ioctl.h"
#include "linux/fb.h"

#include <QDebug>
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#define CODE_FIELD scanCode
#else
#define CODE_FIELD code
#endif

using namespace android;

class KlaatuInputListener: public DISPATCH_CLASS
{
private:
    ScreenControl *screen;
    QTouchDevice *mDevice;
    int xres, yres;

public:
    KlaatuInputListener(ScreenControl *s) : screen(s)
    {
        mDevice = new QTouchDevice;
        mDevice->setName("touchscreen");
        mDevice->setType(QTouchDevice::TouchScreen);
        mDevice->setCapabilities(QTouchDevice::Position | QTouchDevice::Area | QTouchDevice::Pressure);
        QWindowSystemInterface::registerTouchDevice(mDevice);

        struct fb_var_screeninfo fb_var;
        int fd = open("/dev/graphics/fb0", O_RDONLY);
        ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
        close(fd);
        qDebug("screen size is %d by %d\n", fb_var.xres, fb_var.yres);
        xres = fb_var.xres;
        yres = fb_var.yres;
    };

#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 23)
    void dump(String8&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void dispatchOnce()
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    int32_t injectInputEvent(const InputEvent*, int32_t, int32_t, int32_t, int32_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }
    void setInputWindows(const Vector<InputWindow>&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void setFocusedApplication(const InputApplication*)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void setInputDispatchMode(bool, bool)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    status_t registerInputChannel(const sp<InputChannel>&, bool)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }
    status_t unregisterInputChannel(const sp<InputChannel>&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }
    void notifyConfigurationChanged(nsecs_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifyKey(nsecs_t, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, int32_t, nsecs_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifyMotion(nsecs_t, int32_t, int32_t, uint32_t, int32_t, int32_t, int32_t, int32_t, uint32_t, const int32_t*, const PointerCoords*, float, float, nsecs_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifySwitch(nsecs_t, int32_t, int32_t, uint32_t)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
#else
    void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifyKey(const NotifyKeyArgs* args)
    {
#ifdef INPUT_DEBUG
        qDebug("Keyboard event: time %lld keycode=%s action=%d ",
               args->eventTime, KEYCODES[args->keyCode-1].literal,
               args->action);
#endif
	int keycode = 0;
	switch(args->keyCode) {
        case AKEYCODE_VOLUME_DOWN:
	    keycode = Qt::Key_VolumeDown;
	    break;
	case AKEYCODE_VOLUME_UP:
	    keycode = Qt::Key_VolumeUp;
	    break;
	case AKEYCODE_POWER:
            screen->powerKey(!args->action);
	    keycode = Qt::Key_PowerOff;
	    break;
	default:
	    qWarning("Keyboard Device: unrecognized keycode=%d", args->keyCode);
	    break;
	}
	if (keycode != 0)
	    QWindowSystemInterface::handleKeyEvent(0,
              (args->action == AKEY_EVENT_ACTION_DOWN ? QEvent::KeyPress
                                                      : QEvent::KeyRelease),
              keycode, 0, QString(), false);
    }

    void notifyMotion(const NotifyMotionArgs* args)
    {
        float x, y;
        QList<QWindowSystemInterface::TouchPoint> mTouchPoints;
	mTouchPoints.clear();
        QWindowSystemInterface::TouchPoint tp;

	for (unsigned int i = 0; i < args->pointerCount; i++) {
            const PointerCoords pc = args->pointerCoords[i];

            tp.id = args->pointerProperties[i].id;
            tp.flags = 0;  // We are not a pen, check pointerProperties.toolType
            tp.pressure = pc.getAxisValue(AMOTION_EVENT_AXIS_PRESSURE);

            screen->userActivity();
            x = pc.getX();
            y = pc.getY();

            // Store the HW coordinates for now, will be updated later.
            tp.area = QRectF(0, 0,
                             pc.getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR),
                             pc.getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR));
            tp.area.moveCenter(QPoint((int)(x * xres), (int)(y * yres)));
	
            // Get a normalized position in range 0..1.
            tp.normalPosition = QPointF(x, y);

            mTouchPoints.append(tp);
        }

        int id = (args->action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
            AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        switch (args->action & AMOTION_EVENT_ACTION_MASK)
	{
	case AMOTION_EVENT_ACTION_DOWN:
            mTouchPoints[id].state = Qt::TouchPointPressed;
	  break;
	case AMOTION_EVENT_ACTION_UP:
	  mTouchPoints[id].state = Qt::TouchPointReleased;
	  break;
	case AMOTION_EVENT_ACTION_MOVE:
          // err on the side of caution and mark all points as moved
          for (unsigned int i = 0; i < args->pointerCount; i++)
            mTouchPoints[i].state = Qt::TouchPointMoved;
	  break;
	case AMOTION_EVENT_ACTION_POINTER_DOWN:
	  mTouchPoints[id].state = Qt::TouchPointPressed;
	  break;
	case AMOTION_EVENT_ACTION_POINTER_UP:
  	  mTouchPoints[id].state = Qt::TouchPointReleased;
	  break;
	default:
	  qDebug("unrecognized touch event: %d, index %d\n",
                 args->action & AMOTION_EVENT_ACTION_MASK, id);
	  break;
	}

#ifdef INPUT_DEBUG
        qDebug() << "Reporting touch events" << mTouchPoints.count();
        for (int i = 0 ; i < mTouchPoints.count() ; i++) {
            QWindowSystemInterface::TouchPoint& tp(mTouchPoints[i]);
            qDebug() << "  " << tp.id << tp.area << (int) tp.state << tp.normalPosition;
        }
#endif
        QWindowSystemInterface::handleTouchEvent(0, mDevice, mTouchPoints);
    }
    void notifySwitch(const NotifySwitchArgs* args)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void notifyDeviceReset(const NotifyDeviceResetArgs* args)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
#endif // not 2.3
};

class KlaatuReaderPolicy: public InputReaderPolicyInterface {
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 23)
    bool getDisplayInfo(int32_t, int32_t*, int32_t*, int32_t*)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return true;
    }
    bool filterTouchEvents()
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return true;
    }
    bool filterJumpyTouchEvents()
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return true;
    }
    nsecs_t getVirtualKeyQuietTime()
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
        return 0;
    }
    void getVirtualKeyDefinitions(const String8&, Vector<VirtualKeyDefinition>&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void getInputDeviceCalibration(const String8&, InputDeviceCalibration&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
    void getExcludedDeviceNames(Vector<String8>&)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
    }
#else
    void getReaderConfiguration(InputReaderConfiguration* outConfig)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 42)
	static DisplayViewport vport;
        vport.displayId = 0;
        vport.orientation = 0;
        vport.logicalLeft = 0;
        vport.logicalTop = 0;
        vport.logicalRight = 1;
        vport.logicalBottom = 1;
        vport.physicalLeft = 0;
        vport.physicalTop = 0;
        vport.physicalRight = 1;
        vport.physicalBottom = 1;
        vport.deviceWidth = 1;
        vport.deviceHeight = 1;

        outConfig->setDisplayInfo(false, vport);
#else
#if 1
        // output the normalized coordinates
        outConfig->setDisplayInfo(0, false, 1, 1, 0);
#else
        struct fb_var_screeninfo fb_var;
        int fd = open("/dev/graphics/fb0", O_RDONLY);
        ioctl(fd, FBIOGET_VSCREENINFO, &fb_var);
        close(fd);
        qDebug("screen size is %d by %d\n", fb_var.xres, fb_var.yres);
        outConfig->setDisplayInfo(0, false, fb_var.xres, fb_var.yres, 0);
#endif
#endif
    }
    sp<PointerControllerInterface> obtainPointerController(int32_t deviceId)
    {
        qDebug("[%s:%d] %x\n", __FUNCTION__, __LINE__, deviceId);
        return NULL;
    }
    String8 getDeviceAlias(const InputDeviceIdentifier& identifier)
    {
        qDebug("[%s:%d] '%s'\n", __FUNCTION__, __LINE__, identifier.name.string());
        return String8("foo");
    }
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#else
    sp<KeyCharacterMap> getKeyboardLayoutOverlay(const String8& inputDeviceDescriptor)
    {
        qDebug("[%s:%d] '%s'\n", __FUNCTION__, __LINE__, inputDeviceDescriptor.string());
        return NULL;
    }
#endif
    void notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices)
    {
        qDebug("[%s:%d]\n", __FUNCTION__, __LINE__);
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#else
        for (unsigned int i = 0; i < inputDevices.size(); i++) {
            qDebug("name %s\n", inputDevices[i].getIdentifier().name.string());
#if 0
            qDebug("x axis %f - %f\n",
                   inputDevices[i].getMotionRange(AMOTION_EVENT_AXIS_X, 0)->min,
                   inputDevices[i].getMotionRange(AMOTION_EVENT_AXIS_X, 0)->max);
#endif
        }
#endif
    }
#endif // not 2.3
};

// --------------------------------------------------------------------------------
EventThread::EventThread(ScreenControl *s) :
        InputReaderThread(new InputReader(new EventHub(),
                           new KlaatuReaderPolicy(),
                           new KlaatuInputListener(s)))
{
}
