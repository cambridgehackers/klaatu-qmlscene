/*
A front-end for the sensortest-ndk application
 */

#include "sensors.h"
#include <cutils/properties.h>


static QProcess *mProcess = NULL;

Sensors *Sensors::instance()
{
    static Sensors *_s_sensors = 0;
    if (!_s_sensors)
        _s_sensors = new Sensors;
    return _s_sensors;
}

Sensors::Sensors()
{
}

Sensors::~Sensors()
{
	if(mProcess)
        {       
		if(mProcess->state())
                	mProcess->close();

                mProcess->disconnect();
                mProcess->deleteLater();
                mProcess = NULL;
        }

}

void Sensors::runProcess(QString arg)
{
	if(!mProcess) {
        	mProcess = new QProcess(this);
		connect (mProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processOutput()));
	}
        QStringList arguments;
        arguments << arg;
        mProcess->setProcessChannelMode(QProcess::MergedChannels);
        mProcess->start("sensortest-ndk", arguments );
        emit processSpeaks(QString("Starting...\n"));
}

void Sensors::closeProcess()
{
        if(mProcess && mProcess->state())
                mProcess->close();
	
}

void Sensors::processOutput()
{
    emit processSpeaks(QString::fromAscii(mProcess->readAllStandardOutput()));
}
