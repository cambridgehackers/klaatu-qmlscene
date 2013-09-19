/*
 */

#ifndef _SENSORS_H
#define _SENSORS_H

#include <QObject>
#include <QProcess>

class Sensors : public QObject {
    Q_OBJECT

public:
    static Sensors *instance();
    virtual ~Sensors();
    Q_INVOKABLE void runProcess(QString arg);
    Q_INVOKABLE void closeProcess();

signals:
    void processSpeaks(QString line);

public slots:
    void processOutput();

private:
    Sensors();
};


#endif // _SENSORS_H
