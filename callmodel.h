/*
 */

#ifndef _CALL_MODEL_H
#define _CALL_MODEL_H

#include <QAbstractListModel>

class Call
{
public:
    enum CallState { CALL_ACTIVE = 0, CALL_HOLDING, CALL_DIALING, CALL_ALERTING, CALL_INCOMING, CALL_WAITING };

    Call(CallState state, int index, const QString& name, const QString& number)
	: mState(state), mIndex(index), mName(name), mNumber(number) {}

    CallState state() const { return mState; }
    int       index() const { return mIndex; }
    QString   name() const { return mName; }
    QString   number() const { return mNumber; }

private:
    CallState mState;
    int       mIndex;
    QString   mName, mNumber;
};

class CallModel : public QAbstractListModel 
{
    Q_OBJECT
public:
    enum CallRoles { IndexRole = Qt::UserRole+1, NameRole, NumberRole, StateRole };

    CallModel(QObject *parent = 0);

    void     update(const QList<Call>& update);
    int      rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;

private:
    QList<Call> mCallList;
};

#endif // _CALL_MODEL_H

