/*
  Audio control functions
 */

#ifndef _AUDIOCONTROL_H
#define _AUDIOCONTROL_H

#include <QObject>
#include <QMutex>
#include <QUrl>
#include "callmodel.h"

class AudioFile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)

public:
    AudioFile(QObject *parent=0);
    
    QUrl source() const { return mUrl; }
    void setSource(const QUrl& url);

    Q_INVOKABLE bool play(int looping);
    Q_INVOKABLE bool ring();
    Q_INVOKABLE void stop();

signals:
    void sourceChanged();

private:
    QUrl mUrl;
};

class AudioControl : public QObject
{
    Q_OBJECT
    Q_ENUMS(Tone)
    Q_ENUMS(State)
    Q_PROPERTY(bool radioPresent READ radioPresent CONSTANT)
    Q_PROPERTY(bool simPresent READ simPresent NOTIFY simPresentChanged)
    Q_PROPERTY(QString shortONS READ shortONS NOTIFY operatorChanged)
    Q_PROPERTY(QString longONS READ longONS NOTIFY operatorChanged)
    Q_PROPERTY(int rssi READ rssi NOTIFY rssiChanged)
    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString callID READ callID NOTIFY callIDChanged)
    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    Q_PROPERTY(int callState READ callState NOTIFY callStateChanged)

    Q_PROPERTY(QObject *callModel READ callModel NOTIFY callModelChanged)

public:
    // The Tone constants should be kept in sync with ToneGenerator.h
    enum Tone { DTMF_0 = 0, DTMF_1, DTMF_2, DTMF_3, DTMF_4, DTMF_5, DTMF_6, DTMF_7, DTMF_8, DTMF_9, DTMF_S, DTMF_P };
    // The state constants are used to simplify QML logic.  They do NOT represent a complete set;
    // just a subset that can be used for single phone call status....
    enum State { 
	STATE_IDLE = 0,  // No current calls in progress
	STATE_INCOMING,  // Someone is trying to communicate with us.  Ring the phone
	STATE_ACTIVE     // We're talking with someone
    };

    static AudioControl *instance();
    ~AudioControl();

    bool    radioPresent() const;
    bool    simPresent() const;
    void    setSimPresent(bool);

    QString shortONS() const;
    QString longONS() const;
    void    setOperator(const QString&, const QString&);

    int     rssi() const;
    void    setRSSI(int);

    QObject *callModel() const;
    void     setCalls(const QList<Call>&);

    // These are artificial constructs to that assume we only have a single call at a time
    int      state() const;
    QString  callID() const;
    int      index() const;
    int      callState() const;

    Q_INVOKABLE void startTone(int tone, int duration);  // duration in ms (-1=forever)
    Q_INVOKABLE void stopTone();

    Q_INVOKABLE void dial(const QString&);
    Q_INVOKABLE void answer();           // Answer an incoming call
    Q_INVOKABLE void hangup(int index);  // Hang up an active call
    Q_INVOKABLE void reject();           // Reject a call in the 'RINGING' state

    // These are not connected to a property because we have no way of generating
    // an accurate NOTIFY message.  They are dependent on whether or not we're in a
    // call.
    Q_INVOKABLE int  adjustVolumeForCurrentState(int delta);

signals:
    void simPresentChanged();
    void operatorChanged();
    void rssiChanged();
    void callModelChanged();
    void stateChanged();
    void callIDChanged();
    void indexChanged();
    void callStateChanged();

private:
    AudioControl();

    bool    mRadioPresent;
    bool    mSIMPresent;
    QString mShortONS;
    QString mLongONS;
    int     mRssi;
    int     mState;
    QString mCallID;
    int     mIndex;
    int     mCallState;

    CallModel *mCallModel;

    static QMutex sAudioMutex;

    friend class CallModel;
};

#endif // _AUDIOCONTROL_H
