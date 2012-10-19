/*
   Battery and power functions
 */


#include "battery.h"
#include <hardware_legacy/uevent.h>

#include <linux/netlink.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// --------------------------------------------------------------------------------

struct uevent {
    const char *action;
    const char *path;
    const char *subsystem;
    const char *firmware;
    const char *partition_name;
    int partition_num;
    int major;
    int minor;
};


static void parse_event(const char *msg, struct uevent *uevent)
{
    uevent->action = "";
    uevent->path = "";
    uevent->subsystem = "";
    uevent->firmware = "";
    uevent->major = -1;
    uevent->minor = -1;
    uevent->partition_name = NULL;
    uevent->partition_num = -1;

        /* currently ignoring SEQNUM */
    while(*msg) {
        if(!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            uevent->action = msg;
        } else if(!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            uevent->path = msg;
        } else if(!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            uevent->subsystem = msg;
        } else if(!strncmp(msg, "FIRMWARE=", 9)) {
            msg += 9;
            uevent->firmware = msg;
        } else if(!strncmp(msg, "MAJOR=", 6)) {
            msg += 6;
            uevent->major = atoi(msg);
        } else if(!strncmp(msg, "MINOR=", 6)) {
            msg += 6;
            uevent->minor = atoi(msg);
        } else if(!strncmp(msg, "PARTN=", 6)) {
            msg += 6;
            uevent->partition_num = atoi(msg);
        } else if(!strncmp(msg, "PARTNAME=", 9)) {
            msg += 9;
            uevent->partition_name = msg;
        }
            /* advance to after the next \0 */
        while(*msg++)
            ;
    }
//    printf("event { '%s', '%s', '%s', '%s', %d, %d }\n",
//	   uevent->action, uevent->path, uevent->subsystem,
//	   uevent->firmware, uevent->major, uevent->minor);
}

const int UEVENT_MSG_LEN = 1024;

void UEventWatcher::run()
{
    if (uevent_init()) {
	char msg[UEVENT_MSG_LEN+2];
	int n;
	while ((n = uevent_next_event(msg, UEVENT_MSG_LEN)) > 0) {
	    if (n >= UEVENT_MSG_LEN)
		continue;
	    msg[n] = 0;
	    msg[n+1] = 0;
	    struct uevent uevent;
	    parse_event(msg, &uevent);
	    if (strcmp(uevent.subsystem,"power_supply") == 0) {
		emit activity();
	    }
	}
    }
}


// --------------------------------------------------------------------------------

Battery *Battery::instance()
{
    static Battery *_s_battery = 0;
    if (!_s_battery)
        _s_battery = new Battery;
    return _s_battery;
}

static QString checkFile(const QDir& dir, const QString& name)
{
    QFileInfo fi(dir.filePath(name));
    if (fi.exists())
	return fi.filePath();
    return QString();
}

Battery::Battery()
    : mStatus(STATUS_UNKNOWN)
    , mHealth(HEALTH_UNKNOWN)
    , mPresent(false)
    , mACOnline(false)
    , mUSBOnline(false)
    , mCapacity(0)
    , mVoltage(0)
    , mTemperature(0)
    , mVoltageDivisor(1)
{
    QDirIterator it(QStringLiteral("/sys/class/power_supply"));
    while (it.hasNext()) {
	QString path = it.next();
	QString name = it.fileName();

	if (name != QStringLiteral(".") && name != QStringLiteral("..")) {
	    QDir d(path);
	    QFile tfile(d.filePath(QStringLiteral("type")));

	    if (tfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QByteArray data = tfile.readLine().trimmed();
		if (data == "Mains") {
		    mPathACOnline = checkFile(d, QStringLiteral("online"));
		}
		else if (data == "USB") {
		    mPathUSBOnline = checkFile(d, QStringLiteral("online"));
		}
		else if (data == "Battery") {
		    mPathBatteryStatus     = checkFile(d, QStringLiteral("status"));
		    mPathBatteryHealth     = checkFile(d, QStringLiteral("health"));
		    mPathBatteryPresent    = checkFile(d, QStringLiteral("present"));
		    mPathBatteryCapacity   = checkFile(d, QStringLiteral("capacity"));
		    mPathBatteryTechnology = checkFile(d, QStringLiteral("technology"));

		    mPathBatteryVoltage = checkFile(d, QStringLiteral("voltage_now"));
		    if (!mPathBatteryVoltage.isEmpty())
			mVoltageDivisor = 1000;
		    else
			mPathBatteryVoltage = checkFile(d, QStringLiteral("batt_volt"));
		    
		    mPathBatteryTemperature = checkFile(d, QStringLiteral("temp"));
		    if (mPathBatteryTemperature.isEmpty())
			mPathBatteryTemperature = checkFile(d, QStringLiteral("batt_temp"));

		}
	    }
	}
    }
    update();

    // Monitor UEvent
    UEventWatcher *watcher = new UEventWatcher();
    connect(watcher, SIGNAL(activity()), SLOT(update()));
    watcher->start();
}

Battery::~Battery()
{
    // Could kill the event hub, but this should only run if everyone is dying
}

static QByteArray getByteArray(const QString& path)
{
    if (path.isEmpty())
	return QByteArray();

    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) 
	return f.readLine().trimmed();
    return QByteArray();
}

static bool getBooleanValue(const QString& path)
{
    QByteArray data = getByteArray(path);
    return (data.size() && data.data()[0] != '0');
}

static int getIntegerValue(const QString& path)
{
    QByteArray data = getByteArray(path);
    if (data.size())
	return data.toInt();
    return 0;
}

static Battery::BatteryStatus getStatusValue(const QString& path)
{
    QByteArray data = getByteArray(path);
    if (data.size()) {
	switch (data.data()[0]) {
	case 'C': return Battery::CHARGING;
	case 'D': return Battery::DISCHARGING;
	case 'F': return Battery::FULL;
	case 'N': return Battery::NOT_CHARGING;
	default:
	    return Battery::STATUS_UNKNOWN;
	}
    }
    return Battery::STATUS_UNKNOWN;
}

static Battery::BatteryHealth getHealthValue(const QString& path)
{
    QByteArray data = getByteArray(path);
    if (data.size()) {
	switch (data.data()[0]) {
	case 'C': return Battery::COLD;
	case 'D': return Battery::DEAD;
	case 'G': return Battery::GOOD;
	case 'O': 
	    if (data == "Overheat")
		return Battery::OVERHEAT;
	    else if (data == "Over voltage")
		return Battery::OVERVOLTAGE;
	    break;
	case 'U':
	    if (data == "Unspecified failure")
		return Battery::FAILURE;
	    break;
	}
    }
    return Battery::HEALTH_UNKNOWN;
}


void Battery::update()
{
    BatteryStatus status      = getStatusValue(mPathBatteryStatus);
    BatteryHealth health      = getHealthValue(mPathBatteryHealth);
    bool          ac_online   = getBooleanValue(mPathACOnline);
    bool          usb_online  = getBooleanValue(mPathUSBOnline);
    bool          present     = getBooleanValue(mPathBatteryPresent);
    int           capacity    = getIntegerValue(mPathBatteryCapacity);
    int           voltage     = getIntegerValue(mPathBatteryVoltage) / mVoltageDivisor;
    int           temperature = getIntegerValue(mPathBatteryTemperature);
    QString       technology  = getByteArray(mPathBatteryTechnology);

    bool changed = (status != mStatus || health != mHealth ||
		    ac_online != mACOnline || usb_online != mUSBOnline ||
		    present != mPresent || capacity != mCapacity ||
		    voltage != mVoltage || temperature != mTemperature ||
		    technology != mTechnology );

    if (changed) {
	mStatus      = status;
	mHealth      = health;
	mACOnline    = ac_online;
	mUSBOnline   = usb_online;
	mPresent     = present;
	mCapacity    = capacity;
	mVoltage     = voltage;
	mTemperature = temperature;
	mTechnology  = technology;
	emit dataChanged();
    }
}
