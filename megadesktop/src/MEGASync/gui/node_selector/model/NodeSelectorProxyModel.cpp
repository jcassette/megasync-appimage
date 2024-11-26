﻿#include "NodeSelectorProxyModel.h"
#include "megaapi.h"
#include "NodeSelectorModel.h"
#include "MegaApplication.h"
#include "QThread"
#include <QDebug>

NodeSelectorProxyModel::NodeSelectorProxyModel(QObject* parent) :
    QSortFilterProxyModel(parent),
    mSortColumn(NodeSelectorModel::NODE),
    mOrder(Qt::AscendingOrder),
    mExpandMapped(true),
    mForceInvalidate(false)
{
    mCollator.setCaseSensitivity(Qt::CaseInsensitive);
    mCollator.setNumericMode(true);
    mCollator.setIgnorePunctuation(false);

    connect(&mFilterWatcher, &QFutureWatcher<void>::finished,
            this, &NodeSelectorProxyModel::onModelSortedFiltered);
}

NodeSelectorProxyModel::~NodeSelectorProxyModel()
{

}

void NodeSelectorProxyModel::sort(int column, Qt::SortOrder order)
{
    mOrder = order;
    mSortColumn = column;

    //If it is already blocked, it is ignored.
    emit getMegaModel()->blockUi(true);
    emit layoutAboutToBeChanged();
    if(mFilterWatcher.isFinished())
    {
        QFuture<void> filtered = QtConcurrent::run([this, column, order](){
            auto itemModel = dynamic_cast<NodeSelectorModel*>(sourceModel());
            if(itemModel)
            {
                blockSignals(true);
                sourceModel()->blockSignals(true);
                invalidateFilter();
                QSortFilterProxyModel::sort(column, order);
                for (auto it = mItemsToMap.crbegin(); it != mItemsToMap.crend(); ++it)
                {
                    auto proxyIndex = mapFromSource((*it));
                    hasChildren(proxyIndex);
                }
                if(mForceInvalidate)
                {
                    invalidate();
                }
                blockSignals(false);
                sourceModel()->blockSignals(false);
            }
        });
        mFilterWatcher.setFuture(filtered);
    }
}

mega::MegaHandle NodeSelectorProxyModel::getHandle(const QModelIndex &index)
{
    auto node = getNode(index);
    return node ? node->getHandle() : mega::INVALID_HANDLE;
}

QModelIndex NodeSelectorProxyModel::getIndexFromSource(const QModelIndex& index)
{
    return mapToSource(index);
}

QModelIndex NodeSelectorProxyModel::getIndexFromHandle(const mega::MegaHandle& handle)
{
    if(handle == mega::INVALID_HANDLE)
    {
        return QModelIndex();
    }
    auto megaApi = MegaSyncApp->getMegaApi();
    auto node = std::shared_ptr<mega::MegaNode>(megaApi->getNodeByHandle(handle));
    QModelIndex ret = getIndexFromNode(node);

    return ret;
}

QVector<QModelIndex> NodeSelectorProxyModel::getRelatedModelIndexes(const std::shared_ptr<mega::MegaNode> node)
{
    QVector<QModelIndex> ret;

    if(!node)
    {
        return ret;
    }
    auto parentNodeList = std::shared_ptr<mega::MegaNodeList>(mega::MegaNodeList::createInstance());
    parentNodeList->addNode(node.get());
    mega::MegaApi* megaApi = MegaSyncApp->getMegaApi();

    std::shared_ptr<mega::MegaNode> this_node = node;
    while(this_node)
    {
        this_node.reset(megaApi->getParentNode(this_node.get()));
        if(this_node)
        {
            parentNodeList->addNode(this_node.get());
        }
    }
    ret.append(forEach(parentNodeList));

    return ret;
}

std::shared_ptr<mega::MegaNode> NodeSelectorProxyModel::getNode(const QModelIndex &index)
{
    if(!index.isValid())
    {
        return nullptr;
    }
    return qvariant_cast<std::shared_ptr<mega::MegaNode>>(index.data(toInt(NodeSelectorModelRoles::NODE_ROLE)));
}

void NodeSelectorProxyModel::removeNode(const QModelIndex& item)
{
    if(NodeSelectorModel* megaModel = getMegaModel())
    {
        megaModel->removeNodeFromModel(mapToSource(item));
    }
}

bool NodeSelectorProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    bool lIsFile = left.data(toInt(NodeSelectorModelRoles::IS_FILE_ROLE)).toBool();
    bool rIsFile = right.data(toInt(NodeSelectorModelRoles::IS_FILE_ROLE)).toBool();

    auto result(false);

    if(lIsFile && !rIsFile)
    {
        result = sortOrder() == Qt::DescendingOrder;
    }
    else if(!lIsFile && rIsFile)
    {
        result = sortOrder() != Qt::DescendingOrder;
    }
    else
    {
        if(left.column() == NodeSelectorModel::DATE && right.column() == NodeSelectorModel::DATE)
        {
            result = left.data(toInt(NodeSelectorModelRoles::DATE_ROLE)).value<int64_t>() < right.data(toInt(NodeSelectorModelRoles::DATE_ROLE)).value<int64_t>();
        }
        else
        {
            int lStatus(0);
            int rStatus(0);

            if(left.column() == NodeSelectorModel::STATUS && right.column() == NodeSelectorModel::STATUS)
            {
                lStatus = left.data(toInt(NodeSelectorModelRoles::STATUS_ROLE)).toInt();
                rStatus = right.data(toInt(NodeSelectorModelRoles::STATUS_ROLE)).toInt();
            }

            if(lStatus != rStatus)
            {
                result = lStatus < rStatus;
            }
            else if(left.column() == NodeSelectorModel::USER && right.column() == NodeSelectorModel::USER)
            {
                result = mCollator.compare(left.data(Qt::ToolTipRole).toString(),
                                           right.data(Qt::ToolTipRole).toString()) < 0;
            }
            else
            {
                result = mCollator.compare(left.data(Qt::DisplayRole).toString(),
                                           right.data(Qt::DisplayRole).toString())<0;
            }
        }

    }

    return result;
}

void NodeSelectorProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    QSortFilterProxyModel::setSourceModel(sourceModel);

    if(auto nodeSelectorModel = dynamic_cast<NodeSelectorModel*>(sourceModel))
    {
        connect(nodeSelectorModel, &NodeSelectorModel::levelsAdded, this, &NodeSelectorProxyModel::invalidateModel);
        nodeSelectorModel->firstLoad();
    }
}

QVector<QModelIndex> NodeSelectorProxyModel::forEach(std::shared_ptr<mega::MegaNodeList> parentNodeList, QModelIndex parent)
{
    QVector<QModelIndex> ret;

    for(int j = parentNodeList->size()-1; j >= 0; --j)
    {
        auto handle = parentNodeList->get(j)->getHandle();
        for(int i = 0; i < sourceModel()->rowCount(parent); ++i)
        {
            QModelIndex index = sourceModel()->index(i, 0, parent);

            if(NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(index.internalPointer()))
            {
                if(handle == item->getNode()->getHandle())
                {
                    ret.append(mapFromSource(index));

                    auto interList = std::shared_ptr<mega::MegaNodeList>(mega::MegaNodeList::createInstance());
                    for(int k = 0; k < parentNodeList->size(); ++k)
                    {
                        interList->addNode(parentNodeList->get(k));
                    }
                    ret.append(forEach(interList, index));
                    break;
                }
            }
        }
    }

    return ret;
}

QModelIndex NodeSelectorProxyModel::getIndexFromNode(const std::shared_ptr<mega::MegaNode> node)
{
    if(!node)
    {
        return QModelIndex();
    }
    mega::MegaApi* megaApi = MegaSyncApp->getMegaApi();

    std::shared_ptr<mega::MegaNode> root_p_node = node;
    auto p_node = std::unique_ptr<mega::MegaNode>(megaApi->getParentNode(root_p_node.get()));
    while(p_node)
    {
        root_p_node = std::move(p_node);
        p_node.reset(megaApi->getParentNode(root_p_node.get()));
    }

    QVector<QModelIndex> indexList = getRelatedModelIndexes(node);
    if(!indexList.isEmpty())
    {
        return indexList.last();
    }
    return QModelIndex();
}

NodeSelectorModel *NodeSelectorProxyModel::getMegaModel()
{
    return dynamic_cast<NodeSelectorModel*>(sourceModel());
}

bool NodeSelectorProxyModel::isModelProcessing() const
{
    return mFilterWatcher.isRunning();
}

bool NodeSelectorProxyModel::isNotAProtectedModel() const
{
    return dynamic_cast<NodeSelectorModel*>(sourceModel())->canBeDeleted();
}

void NodeSelectorProxyModel::invalidateModel(const QList<QPair<mega::MegaHandle,QModelIndex>>& parents, bool force)
{
    mItemsToMap.clear();
    foreach(auto parent, parents)
    {
        mItemsToMap.append(parent.second);
    }
    mForceInvalidate = force;
    sort(mSortColumn, mOrder);
}

void NodeSelectorProxyModel::onModelSortedFiltered()
{
    if(mForceInvalidate)
    {
        if(auto nodeSelectorModel = dynamic_cast<NodeSelectorModel*>(sourceModel()))
        {
            nodeSelectorModel->proxyInvalidateFinished();
        }
    }

    mForceInvalidate = false;

    emit layoutChanged();

    if(mExpandMapped)
    {
        emit expandReady();
    }
    else
    {
        emit navigateReady(mItemsToMap.isEmpty() ? QModelIndex() : mapFromSource(mItemsToMap.first()));
        if(auto nodeSelectorModel = dynamic_cast<NodeSelectorModel*>(sourceModel()))
        {
            nodeSelectorModel->clearIndexesNodeInfo();
        }
        mExpandMapped = true;
    }
    emit getMegaModel()->blockUi(false);
    mItemsToMap.clear();

    emit modelSorted();
}

NodeSelectorProxyModelSearch::NodeSelectorProxyModelSearch(QObject *parent)
    : NodeSelectorProxyModel(parent), mMode(NodeSelectorModelItemSearch::Type::NONE)
{

}

void NodeSelectorProxyModelSearch::setMode(NodeSelectorModelItemSearch::Types mode)
{
    if(mMode == mode)
    {
        return;
    }

    emit getMegaModel()->blockUi(true);
    mMode = mode;
    invalidateFilter();
    emit getMegaModel()->blockUi(false);
}

bool NodeSelectorProxyModelSearch::isNotAProtectedModel() const
{
    if(mMode & NodeSelectorModelItemSearch::Type::BACKUP)
    {
        return false;
    }
    return NodeSelectorProxyModel::isNotAProtectedModel();
}

bool NodeSelectorProxyModelSearch::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if(mMode == static_cast<int>(NodeSelectorModelItemSearch::Type::NONE))
    {
        return true;
    }

    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    if(index.isValid())
    {
        if(NodeSelectorModelItemSearch* item = static_cast<NodeSelectorModelItemSearch*>(index.internalPointer()))
        {
            return mMode & item->getType();
        }
    }

    return NodeSelectorProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}

