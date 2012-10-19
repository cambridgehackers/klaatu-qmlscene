/*
  Battery and power functions
*/

#ifndef _BATTERY_H
#define _BATTERY_H

#include <QObject>
#include <QString>
#include <QThread>

class UEventWatcher : public QThread 
{
    Q_OBJECT
public:
    void run();
signals:
    void activity();
};

class Battery : public QObject
{
    Q_OBJECT
    Q_ENUMS(BatteryStatus)
    Q_ENUMS(BatteryHealth)
    Q_PROPERTY(BatteryStatus status READ status NOTIFY dataChanged)
    Q_PROPERTY(BatteryHealth health READ health NOTIFY dataChanged)
    Q_PROPERTY(bool present READ present NOTIFY dataChanged)
    Q_PROPERTY(bool ac_online READ ac_online NOTIFY dataChanged)
    Q_PROPERTY(bool usb_online READ usb_online NOTIFY dataChanged)
    Q_PROPERTY(int capacity READ capacity NOTIFY dataChanged)
    Q_PROPERTY(int voltage READ voltage NOTIFY dataChanged)
    Q_PROPERTY(int temperature READ temperature NOTIFY dataChanged)
    Q_PROPERTY(QString technology READ technology NOTIFY dataChanged)

public:
    enum BatteryStatus { CHARGING, DISCHARGING, FULL, NOT_CHARGING, STATUS_UNKNOWN };
    enum BatteryHealth { COLD, DEAD, GOOD, OVERHEAT, OVERVOLTAGE, FAILURE, HEALTH_UNKNOWN };

    static Battery *instance();
    ~Battery();

    BatteryStatus status() const { return mStatus; }
    BatteryHealth health() const { return mHealth; }
    bool          present() const { return mPresent; }
    bool          ac_online() const { return mACOnline; }
    bool          usb_online() const { return mUSBOnline; }
    int           capacity() const { return mCapacity; }
    int           voltage() const { return mVoltage; }
    int           temperature() const { return mTemperature; }
    QString       technology() const { return mTechnology; }

signals:
    void dataChanged();

private slots:
    void update();

private:
    Battery();
    
    BatteryStatus mStatus;
    BatteryHealth mHealth;
    bool          mPresent;
    bool          mACOnline;
    bool          mUSBOnline;
    int           mCapacity;
    int           mVoltage;
    int           mTemperature;
    QString       mTechnology;

    QString       mPathACOnline, mPathUSBOnline, mPathBatteryStatus, mPathBatteryHealth, mPathBatteryPresent;
    QString       mPathBatteryCapacity, mPathBatteryVoltage, mPathBatteryTemperature, mPathBatteryTechnology;
    int           mVoltageDivisor;
};

#endif  // _BATTERY_H
