/*
  Power and screen control functions

  Some of this code is copied from frameworks/base/services/jni/com_android_server_LightsService.cpp
*/

#include "lights.h"

#include <hardware_legacy/power.h>
#include <hardware/hardware.h>
#include <hardware/lights.h>

#include <QMutex>

// -------------------------------------------------------------------

class Light {
public:
    Light(hw_module_t *module, const char *name) 
	: mDevice(0)
	, mColor(0)
	, mFlashingMode(0)
	, mBrightnessMode(0)
	, mOnMS(0)
	, mOffMS(0) {
	hw_device_t *device;
	int err = module->methods->open(module, name, &device);
	if (err) 
	    fprintf(stderr, "Unable to open light device %s, err=%d\n", name, err);
	else {
	    fprintf(stderr, "Opened light device %s\n", name);
	    mDevice = (light_device_t *)device;
	    light_state_t state;
	    memset(&state, 0, sizeof(light_state_t));
	    mDevice->set_light(mDevice, &state);
	}
    }

    void setBrightness(int brightness, int brightnessMode=BRIGHTNESS_MODE_USER) {
	QMutexLocker _l(&mLock);
	int color = brightness & 0x000000ff;
	color = 0xff000000  | (color << 16) | (color << 8) | color;
	setLightLocked(color, LIGHT_FLASH_NONE, 0, 0, brightnessMode);
    }

    void setColor(int color) {
	QMutexLocker _l(&mLock);
	setLightLocked(color, LIGHT_FLASH_NONE, 0, 0, 0);
    }

    void setFlashing(int color, int flashingMode, int onMS, int offMS) {
	QMutexLocker _l(&mLock);
	setLightLocked(color, flashingMode, onMS, offMS, BRIGHTNESS_MODE_USER);
    }

    void setLight(int color, int flashingMode, int onMS, int offMS, int brightnessMode) {
	QMutexLocker _l(&mLock);
	setLightLocked(color, flashingMode, onMS, offMS, brightnessMode);
    }

private:
    void setLightLocked(int color, int flashingMode, int onMS, int offMS, int brightnessMode) {
	if (color != mColor || flashingMode != mFlashingMode || onMS != mOnMS || 
	    offMS != mOffMS || brightnessMode != mBrightnessMode) {
	    mColor = color;
	    mFlashingMode  = flashingMode;
	    mOnMS  = onMS;
	    mOffMS = offMS;
	    mBrightnessMode = brightnessMode;
	    
	    light_state_t state;
	    memset(&state, 0, sizeof(light_state_t));
	    state.color          = color;
	    state.flashMode      = flashingMode;
	    state.flashOnMS      = onMS;
	    state.flashOffMS     = offMS;
	    state.brightnessMode = brightnessMode;
	    mDevice->set_light(mDevice, &state);
	}
    }

private:
    QMutex          mLock;
    light_device_t *mDevice;
    int             mColor;
    int             mFlashingMode; 
    int             mBrightnessMode;
    int             mOnMS, mOffMS;
};

// -------------------------------------------------------------------

Lights *Lights::instance()
{
    static Lights *_s_lights = 0;
    if (!_s_lights)
        _s_lights = new Lights;
    return _s_lights;
}

Lights::Lights()
{
    fprintf(stderr, "Creating Lights\n");
    memset(mLights, 0, sizeof(mLights));

    hw_module_t* module;
    int err = hw_get_module(LIGHTS_HARDWARE_MODULE_ID, (hw_module_t const**)&module);
    if (err == 0) {
	mLights[BACKLIGHT]     = new Light(module, LIGHT_ID_BACKLIGHT);
	mLights[KEYBOARD]      = new Light(module, LIGHT_ID_KEYBOARD);
	mLights[BUTTONS]       = new Light(module, LIGHT_ID_BUTTONS);
	mLights[BATTERY]       = new Light(module, LIGHT_ID_BATTERY);
	mLights[NOTIFICATIONS] = new Light(module, LIGHT_ID_NOTIFICATIONS);
	mLights[ATTENTION]     = new Light(module, LIGHT_ID_ATTENTION);
	mLights[BLUETOOTH]     = new Light(module, LIGHT_ID_BLUETOOTH);
	mLights[WIFI]          = new Light(module, LIGHT_ID_WIFI);
    }
}

Lights::~Lights()
{
    delete mLights;
}


void Lights::setBrightness( LightIndex index, int brightness )
{
    int color = brightness & 0x000000ff;
    color = 0xff000000  | (color << 16) | (color << 8) | color;
    setLight(index, color, FLASH_NONE, 0, 0, BRIGHTNESS_USER);
}

void Lights::setColor( LightIndex index, int color )
{
    setLight(index, color, FLASH_NONE, 0, 0, BRIGHTNESS_USER);
}

void Lights::setFlashing( LightIndex index, int color, int onMS, int offMS )
{
    setLight(index, color, FLASH_HARDWARE, onMS, offMS, BRIGHTNESS_USER);
}

void Lights::setLight( LightIndex index, int colorARGB, FlashingMode flashingMode, 
		  int onMS, int offMS, BrightnessMode brightnessMode )
{
    if (index >= 0 && index < _LIGHT_COUNT && mLights[index])
	mLights[index]->setLight(colorARGB, flashingMode, onMS, offMS, brightnessMode);
}


