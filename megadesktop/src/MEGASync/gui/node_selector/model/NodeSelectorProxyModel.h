#ifndef NODESELECTORPROXYMODEL_H
#define NODESELECTORPROXYMODEL_H

#include "megaapi.h"
#include "NodeSelectorModelItem.h"

#include <QSortFilterProxyModel>
#include <QCollator>
#include <QFutureWatcher>
#include <QEventLoop>

#include <memory>


namespace mega{
class MegaNode;
}
class NodeSelectorModel;

class NodeSelectorProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit NodeSelectorProxyModel(QObject* parent = nullptr);
    ~NodeSelectorProxyModel();

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    mega::MegaHandle getHandle(const QModelIndex &index);
    std::shared_ptr<mega::MegaNode> getNode(const QModelIndex& index);
    QModelIndex getIndexFromSource(const QModelIndex& index);
    QModelIndex getIndexFromHandle(const mega::MegaHandle& handle);
    QModelIndex getIndexFromNode(const std::shared_ptr<mega::MegaNode> node);
    QVector<QModelIndex> getRelatedModelIndexes(const std::shared_ptr<mega::MegaNode> node);
    void removeNode(const QModelIndex &item);
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;
    void setExpandMapped(bool value){mExpandMapped = value;}
    NodeSelectorModel* getMegaModel();
    bool isModelProcessing() const;

    virtual bool isNotAProtectedModel() const;

signals:
    void expandReady();
    void navigateReady(const QModelIndex& index);
    void modelAboutToBeChanged();
    void modelSorted();

private:
    QVector<QModelIndex> forEach(std::shared_ptr<mega::MegaNodeList> parentNodeList, QModelIndex parent = QModelIndex());
    QCollator mCollator;
    int mSortColumn;
    Qt::SortOrder mOrder;
    QFutureWatcher<void> mFilterWatcher;
    QModelIndexList mItemsToMap;
    bool mExpandMapped;
    bool mForceInvalidate;

private slots:
    void invalidateModel(const QList<QPair<mega::MegaHandle, QModelIndex> > &parents, bool force = false);
    void onModelSortedFiltered();
};

class NodeSelectorProxyModelSearch : public NodeSelectorProxyModel
{

public:
    explicit NodeSelectorProxyModelSearch(QObject* parent = nullptr);
    void setMode(NodeSelectorModelItemSearch::Types mode);
    bool isNotAProtectedModel() const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    NodeSelectorModelItemSearch::Types mMode;
};

#endif // NODESELECTORPROXYMODEL_H
