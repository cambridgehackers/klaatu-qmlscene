/*
 */

#include "audiocontrol.h"
#include "callmodel.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <binder/BinderService.h>
#include <media/IMediaPlayerClient.h>
#include <media/IMediaPlayerService.h>
#include <phone/PhoneClient.h>
#include <media/mediaplayer.h>
#include <media/ToneGenerator.h>
#include <cutils/properties.h>

#include <QMutex>
#include <QDebug>
#include <QDir>
#include <QUrl>

using namespace android;

#ifndef UNUSED
# define UNUSED(x) (void) x
#endif

// --------------------------------------------------------------

/*
  Tone Generator interface
 */

enum { TONE_RELATIVE_VOLUME = 80 };

static android::ToneGenerator *toneGenerator() {
    static android::ToneGenerator *_sToneGenerator;
    if (!_sToneGenerator)
	_sToneGenerator = new android::ToneGenerator(AUDIO_STREAM_MUSIC, TONE_RELATIVE_VOLUME);
    return _sToneGenerator;
}

// -------------------------------------------------------------------

class MyMediaPlayer : public RefBase {
public:
    static const sp<MyMediaPlayer>& instance() {
	static sp<MyMediaPlayer> _sMediaPlayer;
	if (_sMediaPlayer.get() == 0)
	    _sMediaPlayer = new MyMediaPlayer;
	return _sMediaPlayer;
    }
       
    bool playSong(const char *filename, int looping, audio_stream_type_t stream_type) {
	Mutex::Autolock _l(mServiceLock);

	int fd = ::open(filename, O_RDONLY);
	if (fd < 0) {
	    perror("Unable to open file!");
	    return false;
	}
	struct stat stat_buf;
	int ret = ::fstat(fd, &stat_buf);
	if (ret < 0) {
	    perror("Unable to stat file");
	    ::close(fd);
	    return false;
	}

	mMediaPlayer->reset();
	mMediaPlayer->setAudioStreamType(stream_type);
	mMediaPlayer->setDataSource(fd, 0, stat_buf.st_size);
	mMediaPlayer->setLooping(looping);
	mMediaPlayer->prepare();   // This is a bad idea...should use async method
	close(fd);
	mMediaPlayer->start();
	return true;
    }

    void stopSong() {
	Mutex::Autolock _l(mServiceLock);
	mMediaPlayer->stop();
    }

    virtual void notify(int msg, int ext1, int ext2, const Parcel *obj) {
	UNUSED(obj);
	printf("notify %d %d %d\n", msg, ext1, ext2);
    }

private:
    MyMediaPlayer() {
	mMediaPlayer = new MediaPlayer;
    }

    mutable Mutex   mServiceLock;
    sp<MediaPlayer> mMediaPlayer;
};


float getMasterVolume() 
{
    float value = 0;
    if (AudioSystem::getMasterVolume(&value) != NO_ERROR)
	fprintf(stderr, "Failed to get master volume\n");
    return value;
}

void setMasterVolume(float value) 
{
    if (AudioSystem::setMasterVolume(value) != NO_ERROR)
	fprintf(stderr, "Failed to set master volume to %f\n", value);
}

int getStreamVolume(audio_stream_type_t stream) 
{
    float value = 0;
    int output = 1;
    if (AudioSystem::getStreamVolume(stream, &value, output) != NO_ERROR)
	fprintf(stderr, "Failed to get ringtone volume\n");
    return AudioSystem::logToLinear(value);
}

void setStreamVolume(audio_stream_type_t stream, int value) 
{
    // printf("Trying to set stream %d volume to %d\n", stream, value);
    int output = 1;
    if (AudioSystem::setStreamVolume(stream, AudioSystem::linearToLog(value), output) != NO_ERROR)
	fprintf(stderr, "Failed to set stream volume to %d\n", value);
    // printf("    actually got %d\n", getStreamVolume(stream));
}

int getStreamVolumeIndex(audio_stream_type_t stream) 
{
    int value = 0;
    if (AudioSystem::getStreamVolumeIndex(stream, &value
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#else
					  , AUDIO_DEVICE_OUT_DEFAULT
#endif
                                                                  ) != NO_ERROR)
	fprintf(stderr, "Failed to get stream volume index\n");
    return value;
}

void setStreamVolumeIndex(audio_stream_type_t stream, int volume)
{
    // printf("Setting stream %d to volume index %d\n", stream, volume);
    if (AudioSystem::setStreamVolumeIndex(stream, volume
#if defined(SHORT_PLATFORM_VERSION) && (SHORT_PLATFORM_VERSION == 40)
#else
					  ,AUDIO_DEVICE_OUT_DEFAULT
#endif
                                                                  ) != NO_ERROR)
	fprintf(stderr, "Failed to set stream volume index\n");
}



// -------------------------------------------------------------------

static const char * ril_error_to_string(int err)
{
    switch (err) {
    case RIL_E_SUCCESS: return "Okay";
    case RIL_E_RADIO_NOT_AVAILABLE: return "Radio not available";
    case RIL_E_GENERIC_FAILURE: return "Generic failure";
    case RIL_E_PASSWORD_INCORRECT: return "Password incorrect";
    case RIL_E_SIM_PIN2: return "Operation requires SIM PIN2 to be entered [4]";
    case RIL_E_SIM_PUK2: return "Operation requires SIM PIN2 to be entered [5]";
    case RIL_E_REQUEST_NOT_SUPPORTED: return "Request not supported";
    case RIL_E_CANCELLED: return "Cancelled";
    case RIL_E_OP_NOT_ALLOWED_DURING_VOICE_CALL: return "Operation not allowed during voice call";
    case RIL_E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW: return "Device needs to register in network";
    case RIL_E_SMS_SEND_FAIL_RETRY: return "Failed to send SMS; need retry";
    case RIL_E_SIM_ABSENT: return "SIM or RUIM card absent";
    case RIL_E_SUBSCRIPTION_NOT_AVAILABLE: return "Fail to find CDMA subscription";
    case RIL_E_MODE_NOT_SUPPORTED: return "HW does not support preferred network type";
    case RIL_E_FDN_CHECK_FAILURE: return "Command failed because recipient not on FDN list";
    case RIL_E_ILLEGAL_SIM_OR_ME: return "Network selection failed to due illeal SIM or ME";
    default:
	return "Unknown";
    }
}

static bool checkresult(const char *str, int result)
{
    if (result) {
	printf("%s failed: '%s' (%d)\n", str, ril_error_to_string(result), result);
	return false;
    }
    return true;
}

// ------------------------------------------------------------------------------------------

/*
 The RadioClient handles all communications with RILD.
 */

class RadioClient : public android::PhoneClient {
protected:
    virtual void ResponseGetSIMStatus(int token, int result, bool simCardPresent) {
	UNUSED(token);
	if (checkresult("GetsIMStatus", result)) {
	    // printf("SIM card = %d\n", simCardPresent);
	    AudioControl::instance()->setSimPresent(result != 0);
	}
    }
    
    virtual void ResponseGetCurrentCalls(int token, int result, const List<CallState>& calls) {
	UNUSED(token);
	if (checkresult("GetCurrentCalls", result)) {
	    // printf("%d current call(s)\n", calls.size());
	    QList<Call> callList;
	    for (List<CallState>::const_iterator it = calls.begin() ; it != calls.end() ; ++it) {
		callList << Call(static_cast<Call::CallState>(it->state), 
				 it->index, 
			       QString::fromUtf16(it->name.string(), it->name.size()),
			       QString::fromUtf16(it->number.string(), it->number.size()));
			       
		// printf(" state=%d index=%d number=%s name=%s\n", it->state, it->index, 
		// String8(it->number).string(), String8(it->name).string());
	    }
	    AudioControl::instance()->setCalls(callList);
	}
    }

    virtual void ResponseDial(int token, int result) {
	UNUSED(token);
	if (checkresult("Dial", result))
	    printf("Dial succeeded\n");
    }

    virtual void ResponseHangup(int token, int result) {
	UNUSED(token);
	if (checkresult("Hangup", result))
	    printf("Hangup succeeded\n");
    }

    virtual void ResponseOperator(int token, int result, const String16& longONS,
				  const String16& shortONS, const String16& code) {
	UNUSED(token);
	if (checkresult("Operator", result)) {
/*	    printf("Operator longONS='%s' shortONS='%s' code='%s'\n",
	    String8(longONS).string(), String8(shortONS).string(), String8(code).string()); */
	    QString s1 = QString::fromUtf16(longONS.string(), longONS.size());
	    QString s2 = QString::fromUtf16(shortONS.string(), shortONS.size());
	    AudioControl::instance()->setOperator(s1, s2);
	}
    }

    virtual void ResponseAnswer(int token, int result) {
	UNUSED(token);
	if (checkresult("Answer", result))
	    printf("Answer succeeded\n");
    }

    virtual void ResponseVoiceRegistrationState(int token, int result, const List<String16>& strlist) {
	UNUSED(token);
	if (checkresult("VoiceRegistrationState", result)) {
/*	    printf("VoiceRegistrationState size=%d\n", strlist.size());
	    for (List<String16>::const_iterator it = strlist.begin() ; it != strlist.end() ; ++it)
		printf(" '%s'\n", String8(*it).string());
*/
	}
    }

    // All unsolicited functions run in the non-main-GUI thread.

    virtual void UnsolicitedRadioStateChanged(int state) {
	// printf("Unsolicited radio state changed to %d\n", state);
    }

    virtual void UnsolicitedCallStateChanged() {
	// printf("Unsolicited call state changed\n");
	GetCurrentCalls(1000);
    }

    virtual void UnsolicitedVoiceNetworkStateChanged() {
	// printf("Unsolicited voice network state changed\n");
	// printf("...checking voice registration state and operator\n");
	Operator(1001);
	VoiceRegistrationState(1001);
    }

    virtual void UnsolicitedNITZTimeReceived(const String16& time) {
	// printf("Unsolicited time received: %s\n", String8(time).string());
    }

    virtual void UnsolicitedSignalStrength(int rssi) {
	// printf("Unsolicited signal strength = %d\n", rssi);
	AudioControl::instance()->setRSSI(rssi);
    }
};

// ------------------------------------------------------------------------------------------

AudioFile::AudioFile(QObject *object)
    : QObject(object)
{
}

void AudioFile::setSource(const QUrl& url)
{
    if ((url.isEmpty() == mUrl.isEmpty()) && url == mUrl)
        return;

    mUrl = url;
    emit sourceChanged();
}

bool AudioFile::play(int looping)
{
    QString filename = mUrl.toLocalFile();
    return MyMediaPlayer::instance()->playSong(filename.toLocal8Bit(), looping, AUDIO_STREAM_MUSIC);
}

bool AudioFile::ring()
{
    QString filename = mUrl.toLocalFile();
    return MyMediaPlayer::instance()->playSong(filename.toLocal8Bit(), 1, AUDIO_STREAM_RING);
}

void AudioFile::stop()
{
    MyMediaPlayer::instance()->stopSong();
}


// ------------------------------------------------------------------------------------------

QMutex AudioControl::sAudioMutex;

android::sp<RadioClient> sRadioClient;

AudioControl *AudioControl::instance()
{
    static AudioControl *_s_audio_control = 0;
    if (!_s_audio_control)
        _s_audio_control = new AudioControl;
    return _s_audio_control;
}

AudioControl::AudioControl()
    : mSIMPresent(false)
    , mRssi(0)
    , mState(STATE_IDLE)
{
    mCallModel = new CallModel(this);

    // For now, we'll assume that only Maguro devices have a radio
    char devicename[PROPERTY_VALUE_MAX];

    if (property_get("ro.product.device", devicename, "") > 0) {
	if (!strcmp(devicename, "maguro") || strstr(devicename, "msm8960") ) {
	    sRadioClient = new RadioClient();
	    sRadioClient->Register(UM_ALL);
	    sRadioClient->GetSIMStatus(1);
	    sRadioClient->Operator(2);
	    sRadioClient->GetCurrentCalls(3);
	}
    }
}

AudioControl::~AudioControl()
{
}

/* 
   Properties on the AudioControl object reflect properties internal to rild/phoned
   The setters are only called from the PhoneClient code (and hence not in the main
   GUI thread).  The getters are called (usually) from the GUI thread.
 */

bool AudioControl::radioPresent() const
{
    return (sRadioClient.get() != NULL);
}

bool AudioControl::simPresent() const
{
    QMutexLocker locker(&sAudioMutex);
    return mSIMPresent;
}

void AudioControl::setSimPresent(bool isPresent)
{
    QMutexLocker locker(&sAudioMutex);
    if (isPresent != mSIMPresent) {
	mSIMPresent = isPresent;
	emit simPresentChanged();
    }
}

QString AudioControl::shortONS() const
{
    QMutexLocker locker(&sAudioMutex);
    return mShortONS;
}

QString AudioControl::longONS() const
{
    QMutexLocker locker(&sAudioMutex);
    return mLongONS;
}

void AudioControl::setOperator(const QString&l, const QString&s)
{
    QMutexLocker locker(&sAudioMutex);
    if (l != mLongONS || s != mShortONS) {
	mLongONS = l;
	mShortONS = s;
	emit operatorChanged();
    }
}

int AudioControl::rssi() const
{
    QMutexLocker locker(&sAudioMutex);
    return mRssi;
}

void AudioControl::setRSSI(int value)
{
    QMutexLocker locker(&sAudioMutex);
    if (value != mRssi) {
	mRssi = value;
	emit rssiChanged();
    }
}

QObject *AudioControl::callModel() const
{
    QMutexLocker locker(&sAudioMutex);
    return mCallModel;
}

void AudioControl::setCalls(const QList<Call>& callList)
{
    QMutexLocker locker(&sAudioMutex);
    mCallModel->update(callList);
    emit callModelChanged();

    // Calculate state here...
    State s = STATE_IDLE;
    QString callID;
    int index = -1;
    int callstate = -1;

    foreach (const Call& call, callList) {
	switch (call.state()) {
	case Call::CALL_ACTIVE:
	case Call::CALL_HOLDING:
	case Call::CALL_DIALING:  // MO only
	case Call::CALL_ALERTING: // MO only
	    s = STATE_ACTIVE;
	    break;
	case Call::CALL_INCOMING: // MT only
	case Call::CALL_WAITING:  // MT Call only - is this set up time?
	    s = STATE_INCOMING;
	    break;
	}
	callID = call.number();
	index = call.index();
	callstate = call.state();
    }

    printf("#### AudioControl::setCalls old_state=%d new_state=%d\n", mState, s);
    if (s != mState) {
	mState = s;
	emit stateChanged();
    }

    if (callID != mCallID) {
	mCallID = callID;
	emit callIDChanged();
    }

    if (index != mIndex) {
	mIndex = index;
	emit indexChanged();
    }

    if (callstate != mCallState) {
	mCallState = callstate;
	emit callStateChanged();
    }
}

int AudioControl::state() const
{
    QMutexLocker locker(&sAudioMutex);
    return mState;
}

QString AudioControl::callID() const
{
    QMutexLocker locker(&sAudioMutex);
    return mCallID;
}

int AudioControl::index() const
{
    QMutexLocker locker(&sAudioMutex);
    return mIndex;
}

int AudioControl::callState() const
{
    QMutexLocker locker(&sAudioMutex);
    return mCallState;
}

// --------------------------------------------------------------

void AudioControl::startTone(int tone, int duration)
{
    toneGenerator()->startTone((android::ToneGenerator::tone_type) tone, duration);
}

void AudioControl::stopTone()
{
    toneGenerator()->stopTone();
}

/*
  These direct control functions need a great deal of cleanup
  to work correctly when you have 
 */

void AudioControl::dial(const QString& number)
{
    if (sRadioClient.get()) {
	String16 s(number.utf16());
	sRadioClient->Dial(100, s);
    }
}

void AudioControl::answer()
{
    if (sRadioClient.get()) 
	sRadioClient->Answer(101);
}

void AudioControl::hangup(int index)
{
    if (sRadioClient.get()) 
	sRadioClient->Hangup(102, index);
}

void AudioControl::reject()
{
    // We should check to make sure that the incoming call is in Ringing
    if (sRadioClient.get()) 
	sRadioClient->Reject(103);
}

/*
  Pass in a value from +15 to -15 (depending on channel)
 */

int AudioControl::adjustVolumeForCurrentState(int delta)
{
    QMutexLocker locker(&sAudioMutex);

    int volume = getStreamVolumeIndex(mState == STATE_ACTIVE ? 
				      AUDIO_STREAM_VOICE_CALL : AUDIO_STREAM_RING);
    volume += delta;

    switch (mState) {
    case STATE_IDLE:
    case STATE_INCOMING:
	setStreamVolumeIndex(AUDIO_STREAM_RING, volume);
	setStreamVolumeIndex(AUDIO_STREAM_NOTIFICATION, volume);
	break;
    case STATE_ACTIVE:
	setStreamVolumeIndex(AUDIO_STREAM_VOICE_CALL, volume);
	setStreamVolumeIndex(AUDIO_STREAM_DTMF, volume);
	break;
    }

    volume = getStreamVolumeIndex(mState == STATE_ACTIVE ? 
				  AUDIO_STREAM_VOICE_CALL : AUDIO_STREAM_RING);
    return volume;
}
