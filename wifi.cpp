/*
  Wifi state machine and interface.
  This object should be placed in its own thread

  This is strongly based on the WifiStateMachine.java code
  in Android.
 */

#include <stdio.h>
#include "wifi.h"

#include <wifi/WifiClient.h>

#include <QDebug>
#include <QMutex>

// using namespace android;

/*
  This common mutex is used to guard against reading data that
  originates from the Wifi server
 */

static QMutex sWifiMutex;

/*
  Adjust the signalLevel to between 0 and 4 (see notes in WifiManager.java)
 */

const int kNumBars = 4;      // Inclusive max; valid values are 0..kNumBars
const int kMinRSSI = -100;   // Below this level we return 0
const int kMaxRSSI = -55;    // Above this level we return kNumBars

static int rssiToLevel(int rssi) 
{
    if (rssi < kMinRSSI)
	return 0;
    if (rssi > kMaxRSSI)
	return kNumBars;
    return (kNumBars - 1 ) * (rssi - kMinRSSI) / (kMaxRSSI - kMinRSSI) + 1;
}

// ------------------------------------------------------------
// Communicate with the wifi server

class MyWifiClient : public android::WifiClient
{
public:
    static MyWifiClient *instance();
    virtual ~MyWifiClient() {}
       
    void State(android::WifiState state) {
	Wifi::instance()->setState(state);
    }

    void ScanResults(const android::Vector<android::ScannedStation>& scandata) {
	Wifi::instance()->setScanResults(scandata);
    }

    void ConfiguredStations(const android::Vector<android::ConfiguredStation>& configdata) {
	Wifi::instance()->setConfiguredStations(configdata);
    }

    void Information(const android::WifiInformation& info) {
	Wifi::instance()->setInformation(info);
    }

private:
    MyWifiClient() {}
};

MyWifiClient *MyWifiClient::instance() {
    static android::sp<MyWifiClient> _s_wifiClient = 0;
    if (!_s_wifiClient.get())
	_s_wifiClient = new MyWifiClient;
    return _s_wifiClient.get();
}

// ------------------------------------------------------------

ScannedStationModel::ScannedStationModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[BssidRole]     = "bssid";
    roles[FrequencyRole] = "frequency";
    roles[RssiRole]      = "rssi";
    roles[FlagsRole]     = "flags";
    roles[SsidRole]      = "ssid";
    setRoleNames(roles);
}

void ScannedStationModel::update(const android::Vector<android::ScannedStation>& update)
{
    QMutexLocker locker(&sWifiMutex);
    beginResetModel();
    mStations = update;
    endResetModel();
}

int ScannedStationModel::rowCount(const QModelIndex&) const
{
    QMutexLocker locker(&sWifiMutex);
    return mStations.size();
}

QVariant ScannedStationModel::data(const QModelIndex& index, int role) const
{
    QMutexLocker locker(&sWifiMutex);
    if (index.row() < 0 || index.row() > (int) mStations.size())
	return QVariant();
    const android::ScannedStation& station(mStations[index.row()]);
    if (role == BssidRole)
	return QString::fromLocal8Bit(station.bssid.string());
    else if (role == FrequencyRole)
	return station.frequency;
    else if (role == RssiRole)
	return station.rssi;
    else if (role == FlagsRole)
	return QString::fromLocal8Bit(station.flags.string());
    else if (role == SsidRole)
	return QString::fromLocal8Bit(station.ssid.string());
    return QVariant();
}

// ------------------------------------------------------------

ConfiguredStationModel::ConfiguredStationModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[NetworkIdRole] = "networkId";
    roles[SsidRole]      = "ssid";
    setRoleNames(roles);
}

void ConfiguredStationModel::update(const android::Vector<android::ConfiguredStation>& update)
{
    QMutexLocker locker(&sWifiMutex);
    beginResetModel();
    // TODO: Sort the list based on index
    mStations = update;
    endResetModel();
}

int ConfiguredStationModel::rowCount(const QModelIndex&) const
{
    QMutexLocker locker(&sWifiMutex);
    return mStations.size();
}

QVariant ConfiguredStationModel::data(const QModelIndex& index, int role) const
{
    QMutexLocker locker(&sWifiMutex);
    if (index.row() < 0 || index.row() > (int) mStations.size())
	return QVariant();
    const android::ConfiguredStation& station(mStations[index.row()]);
    if (role == NetworkIdRole)
	return station.network_id;
    else if (role == SsidRole)
	return QString::fromLocal8Bit(station.ssid.string());
    return QVariant();
}

// ------------------------------------------------------------

class Station
{
public:
    // Keep these values aligned with ConfiguredStation from IWifiClient.h
    enum Status { DISABLED, ENABLED, CURRENT, UNKNOWN };

    Station(const QString& inSsid) 
	: network_id(-1)
	, rssi(-9999)
	, frequency(0)
	, station_count(0)
	, ssid(inSsid)
	, status(UNKNOWN)
	, key_mgmt(Wifi::KEYMGMT_NONE) {
    }

    static Wifi::KeyMgmt flagsToKeyMgmt(const QString& flags) {
	if (flags.contains("WPA2"))
	    return Wifi::KEYMGMT_WPA2;
	if (flags.contains("WPA"))
	    return Wifi::KEYMGMT_WPA;
	if (flags.contains("WEP"))
	    return Wifi::KEYMGMT_WEP;
	return Wifi::KEYMGMT_NONE;
    }

    QString createNote() const { 
	if (network_id != -1 && station_count == 0) {
	    if (status == CURRENT) return QStringLiteral("Current, but out of range");
	    if (status == ENABLED) return QStringLiteral("Enabled, but out of range");
	    return QStringLiteral("Not in range");
	}
	if (status == CURRENT)  return QStringLiteral("Current");
	if (status == ENABLED)  return QStringLiteral("Enabled");
	if (status == DISABLED) return QStringLiteral("Disabled");
	switch (flagsToKeyMgmt(flags)) {
	case Wifi::KEYMGMT_WPA2: return QStringLiteral("WPA2");
	case Wifi::KEYMGMT_WPA:  return QStringLiteral("WPA");
	case Wifi::KEYMGMT_WEP:  return QStringLiteral("WEP");
	case Wifi::KEYMGMT_NONE: return QStringLiteral("");
	}
	return QStringLiteral("");
    }

    static bool lessThan(const Station *self, const Station *other) {
	if (self->status == CURRENT) 
	    return true;
	if (other->status == CURRENT)
	    return false;
	return self->rssi > other->rssi;
    }

    int     network_id, rssi, frequency, station_count;
    QString ssid, flags, pre_shared_key;
    Status  status;
    Wifi::KeyMgmt key_mgmt;
};

CombinedModel::CombinedModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[NetworkIdRole]    = "networkId";
    roles[SsidRole]         = "ssid";
    roles[RssiRole]         = "rssi";
    roles[SignalLevelRole]  = "signalLevel";
    roles[NoteRole]         = "note";
    roles[StatusRole]       = "status";
    roles[StationCountRole] = "stationCount";
    roles[FlagsRole]        = "flags";
    roles[FrequencyRole]    = "frequency";
    roles[KeyMgmtRole]      = "keyMgmt";
    roles[PreSharedKeyRole] = "preSharedKey";
    setRoleNames(roles);
}

Station * CombinedModel::findBySsid(const QString& ssid)
{
    QListIterator<Station *> iter(mStations);
    while (iter.hasNext()) {
	Station *s = iter.next();
	if (s->ssid == ssid) 
	    return s;
    }

    beginInsertRows(QModelIndex(), mStations.count(), mStations.count());
    // qDebug() << " ...insert row" << mStations.count();
    Station *s = new Station(ssid);
    mStations.append(s);
    endInsertRows();
    return s;
}

void CombinedModel::update(const android::Vector<android::ScannedStation>& update)
{
    // qDebug() << "[" << Q_FUNC_INFO;
    QMutexLocker locker(&sWifiMutex);

    // Remove all current RSSI values
    QListIterator<Station *> it(mStations);
    while (it.hasNext()) {
	Station *s = it.next();
	s->rssi = -9999;
	s->station_count = 0;
    }

    for (size_t i = 0 ; i < update.size() ; i++) {
	const android::ScannedStation& scanned(update[i]);
	Station *s = findBySsid(QString::fromLocal8Bit(scanned.ssid.string()));
	QSet<int> changed;
	changed << CombinedModel::RssiRole << CombinedModel::SignalLevelRole << CombinedModel::StationCountRole;

	if (scanned.rssi > s->rssi) 
	    s->rssi = scanned.rssi;  // Might be more than one station

	if (s->frequency != scanned.frequency) {
	    s->frequency = scanned.frequency;
	    changed << CombinedModel::FrequencyRole;
	}

	if (s->flags != scanned.flags) {
	    s->flags = scanned.flags;
	    s->key_mgmt = Station::flagsToKeyMgmt(QString::fromLocal8Bit(scanned.flags.string()));
	    changed << CombinedModel::FlagsRole << CombinedModel::KeyMgmtRole;
	}
	
	s->station_count += 1;
	int row = mStations.indexOf(s);
	// qDebug() << " ...data changed row" << row;
	emit dataChanged(createIndex(row, 0), createIndex(row, 0), changed);
    }

    // Remove stations that didn't show up on the scan
    int row = 0;
    while (row < mStations.size()) {
	Station *s = mStations[row];
	if (s->rssi == -9999 && s->network_id == -1) {
	    beginRemoveRows(QModelIndex(), row, row);
	    delete mStations.takeAt(row);
	    // qDebug() << " ...remove row" << row;
	    endRemoveRows();
	}
	else {
	    row += 1;
	}
    }

    emit layoutAboutToBeChanged();
    QList<Station *> mOldList(mStations);
    qStableSort(mStations.begin(), mStations.end(), Station::lessThan);
    for (int row = 0 ; row < mStations.size() ; row++) {
	int old_row = mOldList.indexOf(mStations[row]);
	if (row != old_row) {
	    changePersistentIndex(createIndex(old_row, 0), createIndex(row, 0));
	    // qDebug() << " ...move row" << old_row << "to" << row;
	}
    }
    emit layoutChanged();

    // qDebug() << " > After scan update, combined list is";
    // for (int row = 0 ; row < mStations.size() ; row++) {
    // 	Station *s = mStations[row];
    // 	qDebug("   id=%d ssid='%s' rssi=%d status=%d", s->network_id, 
    // 	       s->ssid.toLocal8Bit().constData(), s->rssi, s->status);
    // }
}

void CombinedModel::update(const android::Vector<android::ConfiguredStation>& update)
{
    QMutexLocker locker(&sWifiMutex);

    // Mark stations with a non-negative network id
    QListIterator<Station *> it(mStations);
    while (it.hasNext()) {
	Station *s = it.next();
	if (s->network_id != -1)
	    s->network_id = -2;
    }

    for (size_t i = 0 ; i < update.size() ; i++) {
	const android::ConfiguredStation& config(update[i]);
	Station *s = findBySsid(QString::fromLocal8Bit(config.ssid.string()));
	s->network_id = config.network_id;
	s->status     = static_cast<Station::Status>(config.status);
	s->key_mgmt   = Station::flagsToKeyMgmt(QString::fromLocal8Bit(config.key_mgmt.string()));
	s->pre_shared_key = config.pre_shared_key;
	int row = mStations.indexOf(s);
	emit dataChanged(createIndex(row, 0), createIndex(row, 0));
    }

    // Remove stations that didn't show up in the list and don't have scan results
    int row = 0;
    while (row < mStations.size()) {
	Station *s = mStations[row];
	if (s->network_id == -2) {
	    if (s->rssi != -9999) {
		s->network_id = -1;
		s->status = Station::UNKNOWN;
		row += 1;
	    }
	    else {
		beginRemoveRows(QModelIndex(), row, row);
		delete mStations.takeAt(row);
		endRemoveRows();
	    }
	}
	else {
	    row += 1;
	}
    }

    emit layoutAboutToBeChanged();
    QList<Station *> mOldList(mStations);
    qStableSort(mStations.begin(), mStations.end(), Station::lessThan);
    for (int row = 0 ; row < mStations.size() ; row++) {
	int old_row = mOldList.indexOf(mStations[row]);
	if (row != old_row)
	    changePersistentIndex(createIndex(old_row, 0), createIndex(row, 0));
    }
    emit layoutChanged();

    // qDebug() << " > After update, combined list is";
    // for (int row = 0 ; row < mStations.size() ; row++) {
    // 	Station *s = mStations[row];
    // 	qDebug("   id=%d ssid='%s' rssi=%d status=%d", s->network_id, 
    // 	       s->ssid.toLocal8Bit().constData(), s->rssi, s->status);
    // }
}

int CombinedModel::rowCount(const QModelIndex&) const
{
    // This function is called recursively when we're updating models
    //  so we can't grab the mutex.
    // QMutexLocker locker(&sWifiMutex);
    return mStations.size();
}

QVariant CombinedModel::data(const QModelIndex& index, int role) const
{
    QMutexLocker locker(&sWifiMutex);
    if (index.row() < 0 || index.row() > (int) mStations.size())
	return QVariant();
    const Station *s = mStations.at(index.row());
    if (role == NetworkIdRole) return s->network_id;
    else if (role == SsidRole) return s->ssid;
    else if (role == RssiRole) return s->rssi;
    else if (role == SignalLevelRole) return rssiToLevel(s->rssi);
    else if (role == NoteRole) return s->createNote();
    else if (role == StatusRole) return s->status;
    else if (role == StationCountRole) return s->station_count;
    else if (role == FlagsRole) return s->flags;
    else if (role == KeyMgmtRole) return s->key_mgmt;
    else if (role == PreSharedKeyRole) return s->pre_shared_key;
    return QVariant();
}

// ------------------------------------------------------------

// This is sometimes called from the non-main thread.
Wifi * Wifi::instance() 
{
    static QMutex _sWifiInstance;
    static Wifi *_s_wifi = 0;

    QMutexLocker _l(&_sWifiInstance);
    if (!_s_wifi)
        _s_wifi = new Wifi;
    return _s_wifi;
}

/* For the moment we assume only the wlan0 interface */

Wifi::Wifi()
    : mActive(false)
    , mDriverState(UNKNOWN)
    , mDisplayState(DISCONNECTED)
    , mNetworkId(-1)
    , mRssi(-9999)
    , mSignalLevel(-1)
    , mLinkSpeed(-1)
{
    mStationModel = new ScannedStationModel(this);
    mConfiguredModel = new ConfiguredStationModel(this);
    mCombinedModel = new CombinedModel(this);

    MyWifiClient *client = MyWifiClient::instance();
    client->Register(android::WIFI_CLIENT_FLAG_BROADCAST);
}

Wifi::~Wifi()
{
}


void Wifi::setEnabled(bool value)
{
    MyWifiClient::instance()->SetEnabled(value);
}

void Wifi::enableRssiPolling(bool enable)
{
    MyWifiClient::instance()->EnableRssiPolling(enable);
}

void Wifi::startScan(bool active)
{
    MyWifiClient::instance()->StartScan(active);
}

void Wifi::addOrUpdateNetwork(const QString& ssid, const QString& password)
{
    android::ConfiguredStation config;
    config.ssid = ssid.toLocal8Bit();
    if (!password.isEmpty()) {
	config.key_mgmt = "WPA-PSK";
	config.pre_shared_key = password.toLocal8Bit();
    }
    else {
	config.key_mgmt = "NONE";
    }
    MyWifiClient::instance()->AddOrUpdateNetwork(config);
}

void Wifi::removeNetwork(int network_id)
{
    MyWifiClient::instance()->RemoveNetwork(network_id);
}

void Wifi::selectNetwork(int network_id)
{
    MyWifiClient::instance()->SelectNetwork(network_id);
}

void Wifi::reconnect()
{
    MyWifiClient::instance()->Reconnect();
}

void Wifi::disconnect()
{
    MyWifiClient::instance()->Disconnect();
}

void Wifi::reassociate()
{
    MyWifiClient::instance()->Reassociate();
}

bool Wifi::active() const
{
    QMutexLocker _l(&sWifiMutex);
    return mActive; 
}

Wifi::DriverState Wifi::driverState() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mDriverState; 
}

Wifi::DisplayState Wifi::displayState() const 
{ 
    QMutexLocker _l(&sWifiMutex);
     return mDisplayState; 
}

int Wifi::networkId() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mNetworkId; 
}

QString Wifi::bssid() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mBssid; 
}

QString Wifi::ssid() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mSsid; 
}

QString Wifi::ipaddr() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mIPAddress; 
}

QString Wifi::macaddr() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mMacAddress; 
}

int Wifi::rssi() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mRssi; 
}

int Wifi::signalLevel() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mSignalLevel; 
}

int Wifi::linkSpeed() const 
{ 
    QMutexLocker _l(&sWifiMutex);
    return mLinkSpeed; 
}

void Wifi::setState(android::WifiState state)
{
    QMutexLocker _l(&sWifiMutex);
    DriverState s = static_cast<DriverState>(state);
    if (s != mDriverState) {
	mDriverState = s;
	emit driverStateChanged();
    }
}

void Wifi::setScanResults(const android::Vector<android::ScannedStation>& scandata)
{
    // fprintf(stderr, "Wifi::setScanResults (%d)\n", scandata.size());
    mStationModel->update(scandata);
    emit stationModelChanged();
    mCombinedModel->update(scandata);
    emit combinedModelChanged();
}

void Wifi::setConfiguredStations(const android::Vector<android::ConfiguredStation>& configdata)
{
    // fprintf(stderr, "Wifi::setConfiguredStations (%d)\n", configdata.size());
    mConfiguredModel->update(configdata);
    emit configuredModelChanged();
    mCombinedModel->update(configdata);
    emit combinedModelChanged();
}

static Wifi::DisplayState _infoToDisplayState(const android::WifiInformation& info)
{
    if (info.supplicant_state != Wifi::COMPLETED)
	return static_cast<Wifi::DisplayState>(info.supplicant_state);
    if (info.ipaddr.size() == 0)
	return Wifi::ACQUIRING_DHCP;
    return Wifi::FULLY_CONFIGURED;
}

void Wifi::setInformation(const android::WifiInformation& info)
{
    // fprintf(stderr, "Wifi::setInformation)\n");
    QMutexLocker _l(&sWifiMutex);

    QString ipaddr = QString::fromLocal8Bit(info.ipaddr.string());
    if (ipaddr != mIPAddress) {
	mIPAddress = ipaddr;
	emit ipaddrChanged();
    }

    QString macaddr = QString::fromLocal8Bit(info.macaddr.string());
    if (macaddr != mMacAddress) {
	mMacAddress = macaddr;
	emit macaddrChanged();
    }

    QString bssid = QString::fromLocal8Bit(info.bssid.string());
    if (bssid != mBssid) {
	mBssid = bssid;
	emit bssidChanged();
    }

    QString ssid = QString::fromLocal8Bit(info.ssid.string());
    if (ssid != mSsid) {
	mSsid = ssid;
	emit ssidChanged();
    }

    DisplayState s = _infoToDisplayState(info);
    if (mDisplayState != s) {
	mDisplayState = s;
	emit displayStateChanged();
    }

    if (mNetworkId != info.network_id) {
	mNetworkId = info.network_id;
	emit networkIdChanged();
    }

    if (info.link_speed != mLinkSpeed) {
	mLinkSpeed = info.link_speed;
	emit linkSpeedChanged();
    }

    if (mRssi != info.rssi) {
	int signal_level = rssiToLevel(info.rssi);
	if (signal_level != mSignalLevel) {
	    mSignalLevel = signal_level;
	    emit signalLevelChanged();
	}

	mRssi = info.rssi;
	emit rssiChanged();
    }

    bool active = (mDisplayState == Wifi::FULLY_CONFIGURED);
    if (active != mActive) {
	mActive = active;
	emit activeChanged();
    }
}

