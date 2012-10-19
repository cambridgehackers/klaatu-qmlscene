/*
  Wifi
 */

#ifndef _KLAATU_WIFI_H
#define _KLAATU_WIFI_H

#include <QObject>
#include <QAbstractListModel>

#include <wifi/IWifiClient.h>
#include <utils/Vector.h>

class ScannedStationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ScannedStationRoles { BssidRole = Qt::UserRole+1, FrequencyRole, RssiRole,
			    FlagsRole, SsidRole };
    ScannedStationModel(QObject *parent=0);
    
    void     update(const android::Vector<android::ScannedStation>&);
    int      rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;

private:
    android::Vector<android::ScannedStation> mStations;
};

class ConfiguredStationModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ConfiguredStationRoles { NetworkIdRole = Qt::UserRole+1, SsidRole };
    ConfiguredStationModel(QObject *parent=0);
    
    void     update(const android::Vector<android::ConfiguredStation>& update);
    int      rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;

private:
    android::Vector<android::ConfiguredStation> mStations;
};

class Station;

class CombinedModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum CombinedRoles { NetworkIdRole = Qt::UserRole+1, SsidRole,
			 RssiRole, SignalLevelRole, NoteRole, StatusRole, 
			 StationCountRole, FlagsRole, FrequencyRole, 
			 KeyMgmtRole, PreSharedKeyRole };
    CombinedModel(QObject *parent=0);
    
    void     update(const android::Vector<android::ScannedStation>& update);
    void     update(const android::Vector<android::ConfiguredStation>& update);
    int      rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;

private:
    Station * findBySsid(const QString& ssid);

    QList<Station*> mStations;
};



/*
  Master object (singleton)
 */

class Wifi : public QObject
{
    Q_OBJECT
    Q_ENUMS(DriverState)
    Q_ENUMS(DisplayState)
    Q_ENUMS(KeyMgmt)

    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(DriverState driverState READ driverState NOTIFY driverStateChanged )
    Q_PROPERTY(DisplayState displayState READ displayState NOTIFY displayStateChanged)  // Matches SupplicantState
    Q_PROPERTY(int networkId READ networkId NOTIFY networkIdChanged);
    Q_PROPERTY(QString bssid READ bssid NOTIFY bssidChanged);
    Q_PROPERTY(QString ssid READ ssid NOTIFY ssidChanged);
    Q_PROPERTY(QString ipaddr READ ipaddr NOTIFY ipaddrChanged);
    Q_PROPERTY(QString macaddr READ macaddr NOTIFY macaddrChanged);
    Q_PROPERTY(int rssi READ rssi NOTIFY rssiChanged);
    Q_PROPERTY(int signalLevel READ signalLevel NOTIFY signalLevelChanged);
    Q_PROPERTY(int linkSpeed READ linkSpeed NOTIFY linkSpeedChanged);
    Q_PROPERTY(QObject *stationModel READ stationModel NOTIFY stationModelChanged)
    Q_PROPERTY(QObject *configuredModel READ configuredModel NOTIFY configuredModelChanged)
    Q_PROPERTY(QObject *combinedModel READ combinedModel NOTIFY combinedModelChanged)

public:
    enum DriverState { DISABLED, DISABLING, ENABLED, ENABLING, UNKNOWN };
    enum DisplayState { // A combination of supplicant state and DHCP information
	DISCONNECTED = 0,  // Copied from "defs.h" in wpa_supplicant.c
	INTERFACE_DISABLED,   // 1: The network interface is disabled
	INACTIVE,             // 2: No enabled networks in the configuration
	SCANNING,             // 3: Scanning for a network
	AUTHENTICATING,       // 4: Driver authentication with the BSS
	ASSOCIATING,          // 5: Driver associating with BSS (ap_scan=1)
	ASSOCIATED,           // 6: Association successfully completed
	FOUR_WAY_HANDSHAKE,   // 7: WPA 4-way key handshake has started
	GROUP_HANDSHAKE,      // 8: WPA 4-way key completed; group rekeying started
	COMPLETED,            // 9: All authentication is complete.  Now DHCP!
	DORMANT,              // 10: Android-specific state when client issues DISCONNECT
	UNINITIALIZED,        // 11: Android-specific state where wpa_supplicant not running
	ACQUIRING_DHCP,       // 12: Custom
	FULLY_CONFIGURED      // 13: Custom
    };
    enum KeyMgmt { KEYMGMT_NONE, KEYMGMT_WPA2, KEYMGMT_WPA, KEYMGMT_WEP };

    static Wifi *instance();
    ~Wifi();

    // These functions are used to control the Wifi state machine
    Q_INVOKABLE void setEnabled(bool enabled);
    Q_INVOKABLE void enableRssiPolling(bool enable);
    Q_INVOKABLE void startScan(bool active=false);
    Q_INVOKABLE void addOrUpdateNetwork(const QString& ssid, const QString& password=QString());
    Q_INVOKABLE void removeNetwork(int network_id);
    Q_INVOKABLE void selectNetwork(int network_id);
    Q_INVOKABLE void reconnect();
    Q_INVOKABLE void disconnect();
    Q_INVOKABLE void reassociate();

    // Standard accessor functions
    bool         active() const;
    DriverState  driverState() const;
    DisplayState displayState() const;
    int          networkId() const;
    QString      bssid() const;
    QString      ssid() const;
    QString      ipaddr() const;
    QString      macaddr() const;
    int          rssi() const;
    int          signalLevel() const;
    int          linkSpeed() const;

    QObject *stationModel() const { return mStationModel; }
    QObject *configuredModel() const { return mConfiguredModel; }
    QObject *combinedModel() const { return mCombinedModel; }

    // Used internally by the wifi client
    void setState(android::WifiState);
    void setScanResults(const android::Vector<android::ScannedStation>&);
    void setConfiguredStations(const android::Vector<android::ConfiguredStation>&);
    void setRssiLocked(int rssi);
    void setLinkSpeedLocked(int link_speed);
    void setInformation(const android::WifiInformation& info);

signals:
    void activeChanged();
    void driverStateChanged();   
    void displayStateChanged();
    void networkIdChanged();
    void bssidChanged();
    void ssidChanged();
    void ipaddrChanged();
    void macaddrChanged();
    void signalLevelChanged();
    void rssiChanged();
    void linkSpeedChanged();
    void stationModelChanged();
    void configuredModelChanged();
    void combinedModelChanged();

private:
    Wifi();
		   
private:
    // Check this one variable if you need networking
    bool                       mActive;

    // Basic enable/disable
    DriverState                mDriverState;  // Interface enabled / disabled

    // Information from WPA Supplicant
    DisplayState               mDisplayState;  // Complete "displayable" state
    int                        mNetworkId;
    QString                    mBssid;
    QString                    mSsid;
    QString                    mIPAddress;
    QString                    mMacAddress;
    int                        mRssi;
    int                        mSignalLevel;
    int                        mLinkSpeed;

    // List of scanned Wifi base stations
    ScannedStationModel       *mStationModel;
    
    // Configured WPA supplicant elements
    ConfiguredStationModel    *mConfiguredModel;

    // Display-suitable combination model
    CombinedModel             *mCombinedModel;
};

#endif // _KLAATU_WIFI_H
