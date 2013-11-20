/*
 */

#ifndef _POWER_H
#define _POWER_H

#include <QObject>
#include <QProcess>

class Power : public QObject {
    Q_OBJECT

public:
    static Power *instance();
    virtual ~Power();
    Q_INVOKABLE void powerOff();

private:
    Power();
};


#endif 
