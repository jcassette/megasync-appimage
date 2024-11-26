#ifndef SETTYPES_H
#define SETTYPES_H

#include <QObject>
#include <QString>
#include <QList>
#include "megaapi.h"

using MegaNodeSPtr = std::shared_ptr<mega::MegaNode>;

struct SetImportParams
{
    MegaNodeSPtr importParentNode;
};

struct AlbumCollection
{
    QString link = QString::fromUtf8("");
    QString name = QString::fromUtf8("");
    QList<mega::MegaHandle> elementHandleList = {};
    QList<MegaNodeSPtr> nodeList = {};

    // Default constructor
    AlbumCollection() = default;
    ~AlbumCollection()
    {
        reset();
    }

    void reset()
    {
        link = QString::fromUtf8("");
        name = QString::fromUtf8("");
        elementHandleList.clear();
        nodeList.clear();
    }

    bool isComplete() const
    {
        return (!link.isEmpty() &&
                !name.isEmpty() &&
                !elementHandleList.isEmpty() &&
                (elementHandleList.size() == nodeList.size()));
    }
};

Q_DECLARE_METATYPE(AlbumCollection)
Q_DECLARE_METATYPE(SetImportParams)

#endif // SETTYPES_H
