/*
A front-end for the sensortest-ndk application
 */

#include <QDebug>
#include <QTimer>
#include <QGuiApplication>
#include <private/qguiapplication_p.h>

#include "screenorientation.h"
#include "sensors/sensor.h"
#include <cutils/properties.h>

using namespace android;

static QProcess *mProcess = NULL;

android::KlaatuSensor *k=NULL;

void sensor_handler(int type, int rotation)
{
    QScreen *qtscreen = QGuiApplication::primaryScreen();
    qDebug("%s: rotation=%d\n", __FUNCTION__, rotation);
    switch (rotation) {
        case 0:
            QWindowSystemInterface::handleScreenOrientationChange(qtscreen, Qt::PortraitOrientation);
            break;
        case 1:
            QWindowSystemInterface::handleScreenOrientationChange(qtscreen, Qt::LandscapeOrientation);
            break;
        case 2:
            QWindowSystemInterface::handleScreenOrientationChange(qtscreen, Qt::InvertedPortraitOrientation);
            break;
        case 3:
            QWindowSystemInterface::handleScreenOrientationChange(qtscreen, Qt:: InvertedLandscapeOrientation);
            break;
    }
}

ScreenOrientation *ScreenOrientation::instance()
{
    static ScreenOrientation *_s_sensors = 0;
    if (!_s_sensors)
        _s_sensors = new ScreenOrientation;
    return _s_sensors;
}

ScreenOrientation::ScreenOrientation()
{
}

ScreenOrientation::~ScreenOrientation()
{
}

void ScreenOrientation::runProcess(QString arg)
{
     printf("%s In\n", __func__);
     if(!mProcess) {
     	mProcess = new QProcess(this);
     	//connect (mProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));
     }
     QStringList arguments;
     arguments << arg;
     mProcess->setProcessChannelMode(QProcess::MergedChannels);
     


    //char devicename[PROPERTY_VALUE_MAX];

    //if (property_get("ro.product.device", devicename, "") > 0) {
    /*************************************************
     *  For now make it available for all devices ???
     ************************************************/
        #if 0
        if (!strcmp(devicename, "maguro"))
        #endif
        {
        //android::KlaatuSensor *k   = new android::KlaatuSensor();
        k   = new android::KlaatuSensor();
        k->sensor_event_handler = sensor_handler;
        //sleep(3);
        printf("%s calling initSensor\n", __func__);
        k->initSensor(Sensor::TYPE_ACCELEROMETER);
        }
    //}
    /****************************************************************************/
}

void ScreenOrientation::closeProcess()
{
	printf("%s In\n", __func__);
	if (k) {
		printf("%s calling exitSensor\n", __func__);
		k->exitSensor(Sensor::TYPE_ACCELEROMETER);
	}
        if(mProcess && mProcess->state())
                mProcess->close();
}
