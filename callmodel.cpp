/*
 */

#include "callmodel.h"
#include "audiocontrol.h"
#include <QDebug>
#include <QMutexLocker>

// --------------------------------------------------------------

CallModel::CallModel(QObject *parent)
    : QAbstractListModel(parent)
{
    QHash<int, QByteArray> roles;
    roles[IndexRole] = "index";
    roles[NameRole] = "name";
    roles[NumberRole] = "number";
    roles[StateRole] = "state";
    setRoleNames(roles);
}

void CallModel::update(const QList<Call>& update)
{
    // qDebug() << "Updating the call model to size" << update.size();

    beginResetModel();
    mCallList = update;
    endResetModel();
/*
    int row;
    for (row = 0 ; row < update.size() ; row++) {
	const Call& call = update.at(row);
	if (row >= mCallList.size()) {
	    beginInsertRows(QModelIndex(), row, row);
	    mCallList.append(call);
	    endInsertRows();
	} else {
	    if (mCallList.at(row).update(call))
		emit dataChanged(createIndex(row,row), createIndex(row,row));
	}
    }
    if (mCallList.size() > update.size()) {
    // Remove extra rows here...
    }
*/
}

/*
  These two functions need to use the AudioControl mutux
 */

int CallModel::rowCount(const QModelIndex&) const
{
    QMutexLocker locker(&AudioControl::sAudioMutex);
    return mCallList.count();
}

QVariant CallModel::data(const QModelIndex& index, int role) const
{
    QMutexLocker locker(&AudioControl::sAudioMutex);
    if (index.row() < 0 || index.row() > mCallList.count())
	return QVariant();
    const Call& call = mCallList[index.row()];
    if (role == IndexRole)
	return call.index();
    else if (role == NameRole)
	return call.name();
    else if (role == NumberRole)
	return call.number();
    else if (role == StateRole)
	return call.state();
    return QVariant();
}
