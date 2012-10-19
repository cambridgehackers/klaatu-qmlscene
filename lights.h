/*
  Control lights
  Some of this code is copied from frameworks/base/services/jni/com_android_server_LightsService.cpp
*/

#ifndef _LIGHTS_H
#define _LIGHTS_H

#include <QObject>

class Light;

class Lights : public QObject {
    Q_OBJECT
    Q_ENUMS(LightIndex)
    Q_ENUMS(BrightnessMode)
    Q_ENUMS(FlashingMode)

public:
    enum LightIndex { 
	BACKLIGHT = 0, 
	KEYBOARD = 1, 
	BUTTONS = 2, 
	BATTERY = 3,
	NOTIFICATIONS = 4,
	ATTENTION = 5,
	BLUETOOTH = 6,
	WIFI = 7,
	_LIGHT_COUNT
    };
    enum BrightnessMode { BRIGHTNESS_USER, BRIGHTNESS_SENSOR };
    enum FlashingMode { FLASH_NONE, FLASH_TIMED, FLASH_HARDWARE };

    static Lights * instance();
    virtual ~Lights();

    Q_INVOKABLE void setBrightness( LightIndex index, int brightness );
    Q_INVOKABLE void setColor( LightIndex index, int color );
    Q_INVOKABLE void setFlashing( LightIndex index, int color, int onMS, int offMS );
    Q_INVOKABLE void setLight( LightIndex index, int colorARGB, FlashingMode flashingMode, 
			       int onMS, int offMS, BrightnessMode brightnessMode );

private:
    Lights();

private:
    Light *mLights[_LIGHT_COUNT];
};

#endif // _LIGHTS_H
