/*
  Public interface to QML applications for screen and power
  control.
*/

#include "screencontrol.h"
#include "event_thread.h"
#include "lights.h"

#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#include <hardware/hardware.h>
#else
#include <hardware/power.h>
#include <suspend/autosuspend.h>
#endif
#include <hardware_legacy/power.h>
//#include <hardware/hardware.h>
//#include <hardware/lights.h>

#include <QDebug>
#include <QTimer>

using namespace android;

// --------------------------------------------------------------------------------

/*
  The old set_screen_state() functions have been replaced with a device-specific
  power module interface.  See the code in "com_android_server_PowerManagerService.cpp"
 */

#ifdef POWER_HARDWARE_MODULE_ID
static struct power_module *gPowerModule;
#endif

static void power_module_init()
{
#ifdef POWER_HARDWARE_MODULE_ID
    status_t err = hw_get_module(POWER_HARDWARE_MODULE_ID, (hw_module_t const **) &gPowerModule);
    if (!err)
	gPowerModule->init(gPowerModule);
    else
	printf("Couldn't load the %s module (%s)\n", POWER_HARDWARE_MODULE_ID, strerror(-err));
#endif
}

static void set_screen_state(bool on)
{
#ifdef POWER_HARDWARE_MODULE_ID
    if (on) {
	autosuspend_disable();
	if (gPowerModule && gPowerModule->setInteractive)
	    gPowerModule->setInteractive(gPowerModule, true);
    } else {
	if (gPowerModule && gPowerModule->setInteractive)
	    gPowerModule->setInteractive(gPowerModule, false);
	autosuspend_enable();
    }
#endif
}

// --------------------------------------------------------------------------------

ScreenControl *ScreenControl::instance()
{
    static ScreenControl *_s_screen_control = 0;
    if (!_s_screen_control)
        _s_screen_control = new ScreenControl;
    return _s_screen_control;
}

ScreenControl::ScreenControl()
    : mDimTimeout(-1)
    , mSleepTimeout(3000)
    , mScreenLockOn(false)
    , mState(SLEEP)
{
    mTimer = new QTimer(this);
    mTimer->setSingleShot(true);
    connect(mTimer, SIGNAL(timeout()), SLOT(timeout()));

    EventThread *ethread = new EventThread;
    connect(ethread, SIGNAL(userActivity()), SLOT(userActivity()));
    connect(ethread, SIGNAL(powerKey(int)), SLOT(powerKey(int)));
    ethread->start();

    power_module_init();   // Must come before "setState"
    setState(NORMAL);
}

ScreenControl::~ScreenControl()
{
    // Could kill the event hub, but this should only run if everyone is dying
}

void ScreenControl::setDimTimeout(int timeout)
{
    if (timeout != mDimTimeout) {
	mDimTimeout = timeout;
	if (mState == NORMAL) {
	    mTimer->stop();
	    if (!mScreenLockOn && mDimTimeout > 0)
		mTimer->start(mDimTimeout);
	}
	emit dimTimeoutChanged();
    }
}

void ScreenControl::setSleepTimeout(int timeout)
{
    if (timeout != mSleepTimeout) {
	mSleepTimeout = timeout;
	if (mState == DIM && mTimer->isActive())
	    mTimer->start(mSleepTimeout);
	emit dimTimeoutChanged();
    }
}

void ScreenControl::setScreenLockOn(bool lockOn)
{
    if (lockOn != mScreenLockOn) {
	mScreenLockOn = lockOn;
	if (mScreenLockOn) {
	    mTimer->stop();
	    setState(NORMAL);
	}
	else if (mDimTimeout > 0)  // We must be in NORMAL state
	    mTimer->start(mDimTimeout);
	emit screenLockOnChanged();
    }
}

/*!  
  Poke the system to indicate user activity.
  The system will stay on at least "ms" milliseconds.
 */

void ScreenControl::userActivity(int ms)
{
//    qDebug() << Q_FUNC_INFO << ms;
    if (mState != SLEEP) {
	setState(NORMAL);
	if (!mScreenLockOn && mDimTimeout > 0)
	    mTimer->start(qMax(mDimTimeout, ms));
    }
}

/*!  
  Tell the system to go to dim immediately.
 */

void ScreenControl::goToSleep()
{
    if (!mScreenLockOn)
	setState(SLEEP);
}

void ScreenControl::timeout()
{
    switch (mState) {
    case NORMAL:
	setState(DIM);
	break;
    case DIM:
	setState(SLEEP);
	break;
    case SLEEP:
	break;
    }
}

/* 
   For now we'll toggle between power on/off states
   In the future, we need to distinguish cases where the key is held
   down for a long time and a power menu needs to be displayed
*/

void ScreenControl::powerKey(int value)
{
//    qDebug() << Q_FUNC_INFO << value << "current state=" << (int) mState;

    if (value) {  // Key dow
	switch (mState) {
	case NORMAL:
	    setState(SLEEP);
	    break;
	case DIM:
	    setState(NORMAL);   // This choice is debatable
	    break;
	case SLEEP:
	    setState(NORMAL);
	    break;
	}
    }
}

void ScreenControl::setState(SystemState state)
{
    if (state != mState) {
//	qDebug() << Q_FUNC_INFO << "Setting state" << (int) mState << "->" << (int) state;
	mState = state;
	mTimer->stop();
	switch (mState) {
	case NORMAL:
	    set_screen_state(1);
	    Lights::instance()->setBrightness( Lights::BACKLIGHT, 200 );
	    if (!mScreenLockOn && mDimTimeout > 0)
		mTimer->start(mDimTimeout);
	    break;
	case DIM:
	    set_screen_state(1);
	    Lights::instance()->setBrightness( Lights::BACKLIGHT, 20 );
	    mTimer->start(mSleepTimeout);
	    break;
	case SLEEP:
	    set_screen_state(0);
	    Lights::instance()->setBrightness( Lights::BACKLIGHT, 0 );
	    break;
	}
	emit stateChanged();
    }
}


