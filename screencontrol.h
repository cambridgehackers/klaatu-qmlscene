/*
  Screen control
 */

#ifndef _SCREEN_CONTROL_H
#define _SCREEN_CONTROL_H

#include <QObject>

class InputHandler;
class QTimer;

class ScreenControl : public QObject
{
    Q_OBJECT
    Q_ENUMS(SystemState)
    Q_PROPERTY(int dimTimeout READ dimTimeout WRITE setDimTimeout NOTIFY dimTimeoutChanged)
    Q_PROPERTY(int sleepTimeout READ sleepTimeout WRITE setSleepTimeout NOTIFY sleepTimeoutChanged)
    Q_PROPERTY(bool screenLockOn READ screenLockOn WRITE setScreenLockOn NOTIFY screenLockOnChanged)
    Q_PROPERTY(SystemState state READ state NOTIFY stateChanged)

public:
    enum SystemState { NORMAL, DIM, SLEEP };

    static ScreenControl *instance();
    ~ScreenControl();

    int          dimTimeout() const { return mDimTimeout; }
    void         setDimTimeout(int);
    
    int          sleepTimeout() const { return mSleepTimeout; }
    void         setSleepTimeout(int);
    
    bool         screenLockOn() const { return mScreenLockOn; }
    void         setScreenLockOn(bool);
    
    SystemState  state() const { return mState; }

    Q_INVOKABLE void userActivity(int ms = 1000);
    Q_INVOKABLE void goToSleep();
    void         powerKey(int value);

signals:
    void         dimTimeoutChanged();
    void         sleepTimeoutChanged();
    void         screenLockOnChanged();
    void         stateChanged();

private:
    ScreenControl();
    void         setState(SystemState);
		   
private slots:
    void         timeout();

private:
    int          mDimTimeout;
    int          mSleepTimeout;
    bool         mScreenLockOn;
    SystemState  mState;
    QTimer      *mTimer;
};

#endif // _SCREEN_CONTROL_H
