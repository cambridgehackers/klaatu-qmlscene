/*
 */

#ifndef _SCREEN_ORIENTATION_H
#define _SCREEN_ORIENTATION_H

#include <QObject>
#include <QProcess>

class ScreenOrientation : public QObject {
    Q_OBJECT

public:
    static ScreenOrientation *instance();
    virtual ~ScreenOrientation();
    Q_INVOKABLE void runProcess(QString arg);
    Q_INVOKABLE void closeProcess();

//signals:
    //void processSpeaks(QString line);

//public slots:
    //void processOutput();

private:
    ScreenOrientation();
};


#endif // _SCREEN_ORIENTATION_H
