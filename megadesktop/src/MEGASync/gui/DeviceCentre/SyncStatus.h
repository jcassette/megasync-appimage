#ifndef SYNCSTATUS_H
#define SYNCSTATUS_H

#include <QObject>

namespace SyncStatus
{
Q_NAMESPACE

enum Value
{
    UP_TO_DATE,
    UPDATING,
    PAUSED,
    STOPPED
};
Q_ENUM_NS(Value)
}

namespace QmlSyncType
{
Q_NAMESPACE

enum Type
{
    UNDEFINED,
    SYNC,
    BACKUP
};
Q_ENUM_NS(Type)
}

#endif // SYNCSTATUS_H
