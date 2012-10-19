/*
Basic interaction with Android property system
 */

#include "settings.h"
#include <cutils/properties.h>

Settings *Settings::instance()
{
    static Settings *_s_settings = 0;
    if (!_s_settings)
        _s_settings = new Settings;
    return _s_settings;
}

Settings::Settings()
{
}

Settings::~Settings()
{
}

void Settings::set(const QString& name, const QString& value)
{
    property_set(name.toUtf8(), value.toUtf8());
    emit valueChanged(name, value);
}

QString Settings::get(const QString& name, const QString& defvalue)
{
    QString result;
    char data[PROPERTY_VALUE_MAX];
    int len = property_get(name.toUtf8(), data, defvalue.toUtf8());
    return QString::fromUtf8(data, len);
}
