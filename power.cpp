/*
A front-end for the sensortest-ndk application
 */

#include "power.h"
#include <cutils/properties.h>


Power *Power::instance()
{
    static Power *_s_Power = 0;
    if (!_s_Power)
        _s_Power = new Power;
    return _s_Power;
}

Power::Power()
{
}

Power::~Power()
{
}


void Power::powerOff()
{
	system("poweroff -d 1 -f");
}


