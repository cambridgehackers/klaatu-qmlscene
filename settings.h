/*
Basic interaction with Android property system
 */

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <QObject>

class Settings : public QObject {
    Q_OBJECT

public:
    static Settings *instance();
    virtual ~Settings();

    Q_INVOKABLE void set(const QString& name, const QString& value);
    Q_INVOKABLE QString get(const QString& name, const QString& defvalue=QString());

signals:
    void valueChanged(const QString& name, const QString& value);

private:
    Settings();
};


#endif // _SETTINGS_H
