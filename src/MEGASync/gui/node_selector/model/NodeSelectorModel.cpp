#include "NodeSelectorModel.h"

#include "CameraUploadFolder.h"
#include "MegaApplication.h"
#include "MegaNodeNames.h"
#include "MyChatFilesFolder.h"
#include "NodeSelectorModelSpecialised.h"
#include "RequestListenerManager.h"
#include "Utilities.h"

#include <QApplication>
#include <QToolTip>

const char* INDEX_PROPERTY = "INDEX";

NodeRequester::NodeRequester(NodeSelectorModel *model)
    : QObject(nullptr),
      mModel(model),
      mCancelToken(mega::MegaCancelToken::createInstance())
{}

NodeRequester::~NodeRequester()
{
    qDeleteAll(mRootItems);
}

void NodeRequester::lockDataMutex(bool state) const
{
    state ? mDataMutex.lock() : mDataMutex.unlock();
}

bool NodeRequester::isRequestingNodes() const
{
    return mNodesRequested.load();
}

bool NodeRequester::trySearchLock() const
{
    return mSearchMutex.tryLock();
}

void NodeRequester::lockSearchMutex(bool state) const
{
    state ? mSearchMutex.lock() : mSearchMutex.unlock();
}

void NodeRequester::requestNodeAndCreateChildren(NodeSelectorModelItem* item, const QModelIndex& parentIndex)
{
    if(item)
    {
        auto node = item->getNode();
        item->setProperty(INDEX_PROPERTY, parentIndex);
        if(!item->requestingChildren() && !item->areChildrenInitialized())
        {
            item->setRequestingChildren(true);
            mega::MegaApi* megaApi = MegaSyncApp->getMegaApi();

            mNodesRequested = true;

            std::unique_ptr<mega::MegaSearchFilter> searchFilter(mega::MegaSearchFilter::createInstance());
            searchFilter->byNodeType(mShowFiles ? mega::MegaNode::TYPE_UNKNOWN : mega::MegaNode::TYPE_FOLDER);
            searchFilter->byLocationHandle(node->getHandle());

            std::unique_ptr<mega::MegaNodeList> childNodesFiltered(megaApi->getChildren(searchFilter.get(),
                                                                                        mega::MegaApi::ORDER_NONE,
                                                                                        mCancelToken.get()));
            mNodesRequested = false;
            if(!isAborted())
            {
                lockDataMutex(true);
                item->createChildItems(std::move(childNodesFiltered));
                lockDataMutex(false);
                emit nodesReady(item);
            }
        }
    }
}

void NodeRequester::search(const QString &text, NodeSelectorModelItemSearch::Types typesAllowed)
{
    if(text.isEmpty())
    {
        return;
    }

    {
        QMutexLocker a(&mSearchMutex);
        QMutexLocker d(&mDataMutex);
        qDeleteAll(mRootItems);
        mRootItems.clear();
    }
    mSearchCanceled = false;

    std::unique_ptr<mega::MegaSearchFilter> searchFilter(mega::MegaSearchFilter::createInstance());
    searchFilter->byName(text.toUtf8().constData());

    auto nodeList = std::unique_ptr<mega::MegaNodeList>(MegaSyncApp->getMegaApi()->search(searchFilter.get(), mega::MegaApi::ORDER_NONE, mCancelToken.get()));
    QList<NodeSelectorModelItem*> items;
    mSearchedTypes = NodeSelectorModelItemSearch::Type::NONE;

    for(int i = 0; i < nodeList->size(); i++)
    {
        auto item = createSearchItem(nodeList->get(i), typesAllowed);
        if(item)
        {
            items.append(item);
        }
    }

    if(isAborted() || mSearchCanceled)
    {
        qDeleteAll(items);
    }
    else
    {
        QMutexLocker d(&mDataMutex);
        mRootItems.append(items);
        emit searchItemsCreated();
    }
}

void NodeRequester::addSearchRootItem(QList<std::shared_ptr<mega::MegaNode>> nodes, NodeSelectorModelItemSearch::Types typesAllowed)
{
    QList<NodeSelectorModelItem*> items;
    foreach(auto node, nodes)
    {
        auto item = createSearchItem(node.get(), typesAllowed);
        if(item)
        {
            items.append(item);
        }
    }

    if(isAborted())
    {
        qDeleteAll(items);
    }
    else
    {
        if(!items.isEmpty())
        {
            QMutexLocker d(&mDataMutex);
            mRootItems.append(items);
            emit rootItemsAdded();
        }
    }
}

NodeSelectorModelItem *NodeRequester::createSearchItem(mega::MegaNode *node, NodeSelectorModelItemSearch::Types typesAllowed)
{
    if(isAborted() || mSearchCanceled)
    {
        return nullptr;
    }
    if((node->isFile() && !mShowFiles) || MegaSyncApp->getMegaApi()->isInRubbish(node))
    {
        return nullptr;
    }
    else if(mSyncSetupMode)
    {
        int access = MegaSyncApp->getMegaApi()->getAccess(node);
        if(access != mega::MegaShare::ACCESS_FULL && access != mega::MegaShare::ACCESS_OWNER)
        {
            return nullptr;
        }
    }
    else if(!mShowReadOnlyFolders)
    {
        if(MegaSyncApp->getMegaApi()->getAccess(node) == mega::MegaShare::ACCESS_READ
            || !node->isNodeKeyDecrypted())
        {
            return nullptr;
        }
    }

    NodeSelectorModelItemSearch::Types type;

    if(MegaSyncApp->getMegaApi()->isInCloud(node))
    {
        type = NodeSelectorModelItemSearch::Type::CLOUD_DRIVE;
    }
    else if(MegaSyncApp->getMegaApi()->isInVault(node))
    {
        type = NodeSelectorModelItemSearch::Type::BACKUP;
    }
    else
    {
        type = NodeSelectorModelItemSearch::Type::INCOMING_SHARE;
    }

    if(typesAllowed & type)
    {
        mSearchedTypes |= type;
        auto nodeUptr = std::unique_ptr<mega::MegaNode>(node->copy());
        auto item = new NodeSelectorModelItemSearch(std::move(nodeUptr), type);
        return item;
    }

    return nullptr;
}

void NodeRequester::createCloudDriveRootItem()
{
    auto root = std::unique_ptr<mega::MegaNode>(MegaSyncApp->getMegaApi()->getRootNode());

    if(!isAborted())
    {
        auto item = new NodeSelectorModelItemCloudDrive(std::move(root), mShowFiles);
        mRootItems.append(item);
        emit megaCloudDriveRootItemCreated();
    }
}

bool NodeRequester::isIncomingShareCompatible(mega::MegaNode *node)
{
    if(mSyncSetupMode)
    {
        if(MegaSyncApp->getMegaApi()->getAccess(node) != mega::MegaShare::ACCESS_FULL)
        {
            return false;;
        }
    }
    else if(!mShowReadOnlyFolders)
    {
        if(MegaSyncApp->getMegaApi()->getAccess(node) == mega::MegaShare::ACCESS_READ
            || !node->isNodeKeyDecrypted())
        {
            return false;
        }
    }

    return true;
}

void NodeRequester::createIncomingSharesRootItems(std::shared_ptr<mega::MegaNodeList> nodeList)
{
    mega::MegaApi* megaApi = MegaSyncApp->getMegaApi();

    QList<NodeSelectorModelItem*> items;
    for(int i = 0; i < nodeList->size(); i++)
    {
        if(isAborted())
        {
            break;
        }

        if(!isIncomingShareCompatible(nodeList->get(i)))
        {
            continue;
        }

        auto node = std::unique_ptr<mega::MegaNode>(nodeList->get(i)->copy());
        auto user = std::unique_ptr<mega::MegaUser>(megaApi->getUserFromInShare(node.get()));
        NodeSelectorModelItem* item = new NodeSelectorModelItemIncomingShare(std::move(node), mShowFiles);

        items.append(item);

        auto incomingSharesModel = dynamic_cast<NodeSelectorModelIncomingShares*>(mModel);
        if(incomingSharesModel)
        {
            item->setProperty(INDEX_PROPERTY, incomingSharesModel->index(i,0));
            connect(item, &NodeSelectorModelItem::infoUpdated, incomingSharesModel, &NodeSelectorModelIncomingShares::onItemInfoUpdated);
            item->setOwner(std::move(user));
        }
    }

    if(isAborted())
    {
        qDeleteAll(items);
    }
    else
    {
        mRootItems.append(items);
        emit megaIncomingSharesRootItemsCreated();
    }
}

void NodeRequester::addIncomingSharesRootItem(std::shared_ptr<mega::MegaNode> node)
{
    if(isAborted())
    {
        return;
    }

    if(!isIncomingShareCompatible(node.get()))
    {
        return;
    }

    mega::MegaApi* megaApi = MegaSyncApp->getMegaApi();
    auto user = std::unique_ptr<mega::MegaUser>(megaApi->getUserFromInShare(node.get()));
    NodeSelectorModelItem* item = new NodeSelectorModelItemIncomingShare(std::unique_ptr<mega::MegaNode>(node->copy()), mShowFiles);

    auto incomingSharesModel = dynamic_cast<NodeSelectorModelIncomingShares*>(mModel);
    if(incomingSharesModel)
    {
        item->setProperty(INDEX_PROPERTY, incomingSharesModel->index(incomingSharesModel->rowCount(),0));
        connect(item, &NodeSelectorModelItem::infoUpdated, incomingSharesModel, &NodeSelectorModelIncomingShares::onItemInfoUpdated);
        item->setOwner(std::move(user));
    }

    if(isAborted())
    {
        item->deleteLater();
    }
    else
    {
        mRootItems.append(item);
        emit rootItemsAdded();
    }
}

void NodeRequester::createBackupRootItems(mega::MegaHandle backupsHandle)
{
    if (backupsHandle != mega::INVALID_HANDLE)
    {
        std::unique_ptr<mega::MegaNode> backupsNode(MegaSyncApp->getMegaApi()->getNodeByHandle(backupsHandle));
        if(backupsNode)
        {
            if(!isAborted())
            {
                NodeSelectorModelItem* item = new NodeSelectorModelItemBackup(std::move(backupsNode), mShowFiles);
                //Here we are setting my backups node as vault node in the item, it is not the same vault node that we get
                //doing megaapi->getVaultNode(), we have to hide it here thats why are doing this trick.
                //The real vault is the parent of my backups folder
                //NodeSelectorModelItem* item = new NodeSelectorModelItem(std::move(backupsNode), mShowFiles);
                //item->setAsVaultNode();
                mRootItems.append(item);
            }
        }
    }

    if(!isAborted())
    {
        emit megaBackupRootItemsCreated();
    }
}

void NodeRequester::onAddNodesRequested(QList<std::shared_ptr<mega::MegaNode>> newNodes, const QModelIndex& parentIndex, NodeSelectorModelItem *parentItem)
{
    auto lastChild = parentItem->getNumChildren();
    lockDataMutex(true);
    auto childrenItem = parentItem->addNodes(newNodes);
    lockDataMutex(false);
    foreach(auto& childItem, childrenItem)
    {
        childItem->setProperty(INDEX_PROPERTY, mModel->index(lastChild, 0, parentIndex));
        lastChild++;
    }

    if(!isAborted())
    {
        emit nodesAdded(childrenItem);
    }
    else
    {
        foreach(auto& childItem, childrenItem)
        {
            removeItem(childItem);
        }
    }
}

void NodeRequester::removeItem(NodeSelectorModelItem* item)
{
    QMutexLocker lock(&mDataMutex);
    item->deleteLater();
}

void NodeRequester::removeRootItem(NodeSelectorModelItem* item)
{
    QMutexLocker lock(&mDataMutex);
    item->deleteLater();
    mRootItems.removeOne(item);
}

void NodeRequester::removeRootItem(std::shared_ptr<mega::MegaNode> node)
{
    if(isAborted())
    {
        return;
    }

    auto rootFound = std::find_if(mRootItems.begin(), mRootItems.end(), [node](NodeSelectorModelItem* item){
        return item->getNode()->getHandle() == node->getHandle();
    });

    if(rootFound != mRootItems.end())
    {
        mRootItems.removeOne(*rootFound);
        emit rootItemsDeleted();
    }
}

int NodeRequester::rootIndexSize() const
{
    QMutexLocker lock(&mDataMutex);
    return mRootItems.size();
}

int NodeRequester::rootIndexOf(NodeSelectorModelItem* item)
{
    QMutexLocker lock(&mDataMutex);
    return mRootItems.indexOf(item);
}

NodeSelectorModelItem *NodeRequester::getRootItem(int index) const
{
    QMutexLocker lock(&mDataMutex);
    return mRootItems.at(index);
}

void NodeRequester::restartSearch()
{
    if(mCancelToken)
    {
        mCancelToken->cancel();
        mSearchCanceled = true;
        mCancelToken.reset(mega::MegaCancelToken::createInstance());
    }
}

void NodeRequester::cancelCurrentRequest()
{
    if(mCancelToken)
    {
        mSearchCanceled = true;
        mCancelToken->cancel();
    }
}

bool NodeRequester::isAborted()
{
    return mAborted || (mCancelToken && mCancelToken->isCancelled());
}

bool NodeRequester::showFiles() const
{
    return mShowFiles.load();
}

const NodeSelectorModelItemSearch::Types &NodeRequester::searchedTypes() const
{
    return mSearchedTypes;
}

void NodeRequester::setShowFiles(bool show)
{
    mShowFiles = show;
}

void NodeRequester::setShowReadOnlyFolders(bool show)
{
   mShowReadOnlyFolders = show;
}

void NodeRequester::setSyncSetupMode(bool value)
{
    mSyncSetupMode = value;
}

void NodeRequester::abort()
{
    cancelCurrentRequest();
    mAborted = true;
}

/* ------------------- MODEL ------------------------- */

const int NodeSelectorModel::ROW_HEIGHT = 25;

NodeSelectorModel::NodeSelectorModel(QObject *parent) :
    QAbstractItemModel(parent),
    mRequiredRights(mega::MegaShare::ACCESS_READ),
    mDisplayFiles(false),
    mSyncSetupMode(false),
    mIsBeingModified(true)
{
    mCameraFolderAttribute = UserAttributes::CameraUploadFolder::requestCameraUploadFolder();
    mMyChatFilesFolderAttribute = UserAttributes::MyChatFilesFolder::requestMyChatFilesFolder();

    mNodeRequesterThread = new QThread();
    mNodeRequesterWorker = new NodeRequester(this);
    mNodeRequesterWorker->moveToThread(mNodeRequesterThread);
    mNodeRequesterThread->start();

    connect(this, &NodeSelectorModel::requestChildNodes, mNodeRequesterWorker, &NodeRequester::requestNodeAndCreateChildren, Qt::QueuedConnection);
    connect(this, &NodeSelectorModel::requestAddNodes, mNodeRequesterWorker, &NodeRequester::onAddNodesRequested, Qt::QueuedConnection);
    connect(this, &NodeSelectorModel::removeItem, mNodeRequesterWorker, &NodeRequester::removeItem);
    connect(this, &NodeSelectorModel::removeRootItem, this, [this](NodeSelectorModelItem* item)
            {
                mNodeRequesterWorker->removeRootItem(item);
            });

    connect(mNodeRequesterThread, &QThread::finished, mNodeRequesterThread, &QObject::deleteLater, Qt::DirectConnection);
    connect(mNodeRequesterThread, &QThread::finished, mNodeRequesterWorker, &QObject::deleteLater, Qt::DirectConnection);

    connect(mNodeRequesterWorker, &NodeRequester::nodesReady, this, &NodeSelectorModel::onChildNodesReady, Qt::QueuedConnection);
    connect(mNodeRequesterWorker, &NodeRequester::nodesAdded, this, &NodeSelectorModel::onNodesAdded, Qt::QueuedConnection);

    connect(mNodeRequesterWorker, &NodeRequester::rootItemsAdded, this, &NodeSelectorModel::onRootItemAdded, Qt::QueuedConnection);
    connect(mNodeRequesterWorker, &NodeRequester::rootItemsDeleted, this, &NodeSelectorModel::onRootItemDeleted, Qt::QueuedConnection);

    qRegisterMetaType<std::shared_ptr<mega::MegaNodeList>>("std::shared_ptr<mega::MegaNodeList>");
    qRegisterMetaType<std::shared_ptr<mega::MegaNode>>("std::shared_ptr<mega::MegaNode>");
    qRegisterMetaType<mega::MegaHandle>("mega::MegaHandle");

    protectModelWhenPerformingActions();

    mListener = RequestListenerManager::instance().registerAndGetFinishListener(this, false);
}

NodeSelectorModel::~NodeSelectorModel()
{
    mNodeRequesterThread->quit();
    mNodeRequesterThread->wait();
}

void NodeSelectorModel::protectModelWhenPerformingActions()
{
    auto protectModel = [this](){
        setIsModelBeingModified(true);
    };

    connect(this, &NodeSelectorModel::rowsAboutToBeInserted, protectModel);
    connect(this, &NodeSelectorModel::rowsAboutToBeRemoved, protectModel);
    connect(this, &NodeSelectorModel::rowsAboutToBeMoved, protectModel);
    connect(this, &NodeSelectorModel::modelAboutToBeReset, protectModel);

    auto unprotectModel = [this](){
        setIsModelBeingModified(false);
    };
    connect(this, &NodeSelectorModel::rowsInserted, unprotectModel);
    connect(this, &NodeSelectorModel::rowsRemoved, unprotectModel);
    connect(this, &NodeSelectorModel::modelReset, unprotectModel);
    connect(this, &NodeSelectorModel::rowsMoved, unprotectModel);
}

int NodeSelectorModel::columnCount(const QModelIndex &) const
{
    return last;
}

QVariant NodeSelectorModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid())
    {
        NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(index.internalPointer());
        if (item)
        {
            switch(role)
            {
            case Qt::DecorationRole:
            {
                return getIcon(index, item);
            }
            case Qt::DisplayRole:
            {
                return getText(index, item);
            }
            case Qt::SizeHintRole:
            {
                return QSize(0, ROW_HEIGHT);
            }
            case  Qt::TextAlignmentRole:
            {
                if(index.column() == STATUS || index.column() == USER)
                {
                    return QVariant::fromValue<Qt::Alignment>(Qt::AlignHCenter | Qt::AlignCenter);
                }
                break;
            }
            case Qt::ToolTipRole:
            {
                if(index.column() == USER)
                {
                    return item->getOwnerName();
                }
                else if(mSyncSetupMode)
                {
                    if((item->getStatus() == NodeSelectorModelItem::Status::SYNC)
                            || (item->getStatus() == NodeSelectorModelItem::Status::SYNC_CHILD))
                    {
                        return tr("Folder already synced");
                    }
                    else if(item->getStatus() == NodeSelectorModelItem::Status::SYNC_PARENT)
                    {
                        return tr("Folder contents already synced");
                    }
                    QToolTip::hideText();
                }
                break;
            }
            case toInt(NodeSelectorModelRoles::DATE_ROLE):
            {
                return QVariant::fromValue(item->getNode()->getCreationTime());
            }
            case toInt(NodeSelectorModelRoles::IS_FILE_ROLE):
            {
                return QVariant::fromValue(item->getNode()->isFile());
            }
            case toInt(NodeSelectorModelRoles::IS_SYNCABLE_FOLDER_ROLE):
            {
                return QVariant::fromValue(item->isSyncable() && item->getNode()->isFolder());
            }
            case toInt(NodeSelectorModelRoles::STATUS_ROLE):
            {
                return QVariant::fromValue(item->getStatus());
            }
            case toInt(NodeSelectorModelRoles::HANDLE_ROLE):
            {
                return QVariant::fromValue(item->getNode() ?
                                               item->getNode()->getHandle()
                                             : mega::INVALID_HANDLE);
            }
            case toInt(NodeSelectorModelRoles::MODEL_ITEM_ROLE):
            {
                return QVariant::fromValue(item);
            }
            case toInt(NodeSelectorModelRoles::NODE_ROLE):
            {
                return QVariant::fromValue(item->getNode());
            }
            case toInt(NodeRowDelegateRoles::INDENT_ROLE):
            {
                return item->isCloudDrive() || item->isVault() ? -10 : 0;
            }
            case toInt(NodeRowDelegateRoles::SMALL_ICON_ROLE):
            {
                return item->isCloudDrive() || item->isVault() ? true : false;
            }
            case toInt(NodeRowDelegateRoles::INIT_ROLE):
            {
                return item->areChildrenInitialized();
            }
            default:
            {
                break;
            }
            }
        }
    }
    return QVariant();
}

Qt::ItemFlags NodeSelectorModel::flags(const QModelIndex &index) const
{
    auto flags = QAbstractItemModel::flags(index);

    if (index.isValid())
    {
        NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(index.internalPointer());
        if (item)
        {
            if((mSyncSetupMode && !item->isSyncable()) || (item->getNode() && !item->getNode()->isNodeKeyDecrypted()))
            {
                flags &= ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            }
        }
    }

    return flags;
}

bool NodeSelectorModel::showFiles() const
{
    return mNodeRequesterWorker->showFiles();
}

QModelIndex NodeSelectorModel::index(int row, int column, const QModelIndex &parent) const
{
    QModelIndex index;

    if (hasIndex(row, column, parent))
    {
        if (parent.isValid())
        {
            mNodeRequesterWorker->lockDataMutex(true);
            NodeSelectorModelItem* item(static_cast<NodeSelectorModelItem*>(parent.internalPointer()));
            if(item)
            {
                auto data = item->getChild(row).data();
                if(data)
                {
                    index =  createIndex(row, column, data);
                }
            }
            mNodeRequesterWorker->lockDataMutex(false);
        }
        else if(mNodeRequesterWorker->rootIndexSize() > row)
        {
            auto rootItem = mNodeRequesterWorker->getRootItem(row);
            index = createIndex(row, column, rootItem);
        }
    }

    return index;
}

QModelIndex NodeSelectorModel::parent(const QModelIndex &index) const
{
    QModelIndex parentIndex;

    if(index.isValid())
    {
        NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(index.internalPointer());
        if(item)
        {
            NodeSelectorModelItem* parent = item->getParent();
            if (parent)
            {
                auto indexOfParent = mNodeRequesterWorker->rootIndexOf(parent);
                if(indexOfParent >= 0)
                {
                    parentIndex = createIndex(indexOfParent, 0, parent);
                }
                else
                {
                    parentIndex = createIndex(parent->row(), 0, parent);
                }
            }
        }
    }

    return parentIndex;
}

int NodeSelectorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        mNodeRequesterWorker->lockDataMutex(true);
        NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(parent.internalPointer());
        auto rows = item ? item->getNumChildren() : 0;
        mNodeRequesterWorker->lockDataMutex(false);
        return rows;
    }
    return mNodeRequesterWorker->rootIndexSize();
}

bool NodeSelectorModel::hasChildren(const QModelIndex &parent) const
{
    /////FROM MODEL TESTER:
    // Column 0                | Column 1    |
    // QModelIndex()           |             |
    //    \- topIndex          | topIndex1   |
    //         \- childIndex   | childIndex1 |

    // Common error test #3, the second column should NOT have the same children
    // as the first column in a row.
    // Usually the second column shouldn't have children.

    if(parent.isValid() && parent.column() != NODE)
    {
        return false;
    }

    NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(parent.internalPointer());
    if(item && item->getNode())
    {
        mNodeRequesterWorker->lockDataMutex(true);
        auto numChild = item->getNumChildren() > 0;
        mNodeRequesterWorker->lockDataMutex(false);
        return numChild;
    }
    else
    {
        return QAbstractItemModel::hasChildren(parent);
    }
}

QVariant NodeSelectorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Orientation::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
             switch(section)
             {
             case STATUS:
             case USER:
             {
                 return QLatin1String();
             }
             case DATE:
             {
                 return tr("Recently used");
             }
             case NODE:
             {
                 return tr("Name");
             }
             }
        }
        else if(role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case STATUS:
            {
                return tr("Sort by status");
            }
            case USER:
            {
                return tr("Sort by owner name");
            }
            case DATE:
            {
                return tr("Sort by date");
            }
            case NODE:
            {
                return tr("Sort by name");
            }
            }
        }
        else if(role == toInt(HeaderRoles::ICON_ROLE))
        {
            if(section == USER)
            {
                return QIcon(QLatin1String("://images/node_selector/icon_small_user.png"));
            }
            else if(section == STATUS)
            {
                return QIcon(QLatin1String("://images/node_selector/icon-small-MEGA.png"));
            }
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

void NodeSelectorModel::setSyncSetupMode(bool value)
{
    mSyncSetupMode = value;
    mNodeRequesterWorker->setSyncSetupMode(value);
}

void NodeSelectorModel::addNodes(QList<std::shared_ptr<mega::MegaNode>> nodes, const QModelIndex &parent)
{
    if(!nodes.isEmpty())
    {
        if(parent.isValid())
        {
            NodeSelectorModelItem* parentItem = static_cast<NodeSelectorModelItem*>(parent.internalPointer());
            if(parentItem && parentItem->getNode()->isFolder() && parentItem->areChildrenInitialized())
            {
                clearIndexesNodeInfo();
                auto totalRows = rowCount(parent);
                beginInsertRows(parent, totalRows, totalRows + nodes.size() - 1);
                emit requestAddNodes(nodes, parent, parentItem);
            }
        }
    }
}

void NodeSelectorModel::onNodesAdded(QList<QPointer<NodeSelectorModelItem>> childrenItem)
{
    endInsertRows();

    foreach(auto child, childrenItem)
    {
        auto index = child->property(INDEX_PROPERTY).toModelIndex();
        emit dataChanged(index, index);
    }

    if (!childrenItem.empty())
    {
        emit levelsAdded({}, false);
    }
}

void NodeSelectorModel::onRootItemAdded()
{
    endInsertRows();
}

void NodeSelectorModel::onRootItemDeleted()
{
    endRemoveRows();
}

bool NodeSelectorModel::addToLoadingList(const std::shared_ptr<mega::MegaNode> node)
{
    return node != nullptr;
}

std::shared_ptr<mega::MegaNode> NodeSelectorModel::getNodeToRemove(mega::MegaHandle handle)
{
    auto node = std::shared_ptr<mega::MegaNode>(MegaSyncApp->getMegaApi()->getNodeByHandle(handle));
    int access = MegaSyncApp->getMegaApi()->getAccess(node.get());

    // This is for an extra protection as we don´t show the delete action if one of this
    // conditions are not met
    if (!node || access < mega::MegaShare::ACCESS_FULL || !node->isNodeKeyDecrypted())
    {
        return nullptr;
    }

    return node;
}

void NodeSelectorModel::removeNodes(const QList<mega::MegaHandle>& nodeHandles, bool permanently)
{
    emit blockUi(true);
    //It will be unblocked when all requestFinish calls are received (check onRequestFinish)
    QtConcurrent::run([this, nodeHandles, permanently]() {
        foreach(auto handle, nodeHandles)
        {
            std::shared_ptr<mega::MegaNode> node(
                MegaSyncApp->getMegaApi()->getNodeByHandle(handle));
            if (node)
            {
                int access = MegaSyncApp->getMegaApi()->getAccess(node.get());

                mRequestByHandle.insert(handle, mega::MegaRequest::TYPE_REMOVE);

                // Double protection in case the node properties changed while the node is deleted
                if (permanently ||
                    (access == mega::MegaShare::ACCESS_FULL && node->isNodeKeyDecrypted()))
                {
                    MegaSyncApp->getMegaApi()->remove(node.get(), mListener.get());
                }
                else
                {
                    auto rubbish = MegaSyncApp->getRubbishNode();
                    moveNode(node, rubbish);
                }
            }
        }
    });
}

bool NodeSelectorModel::areAllNodesEligibleForDeletion(const QList<mega::MegaHandle>& handles)
{
    foreach(auto&& handle, handles)
    {
        std::unique_ptr<mega::MegaNode> node(MegaSyncApp->getMegaApi()->getNodeByHandle(handle));
        if (!node || !node->isNodeKeyDecrypted() ||
            getNodeAccess(node.get()) < mega::MegaShare::ACCESS_FULL)
        {
            return false;
        }
    }

    // Return false if there are no handles (disabled rows...)
    return !handles.isEmpty();
}

void NodeSelectorModel::removeNodeFromModel(const QModelIndex& index)
{
    if(!index.isValid())
    {
        return;
    }
    auto item = static_cast<NodeSelectorModelItem*>(index.internalPointer());
    if(item)
    {
        std::shared_ptr<mega::MegaNode> node = item->getNode();
        if (node)
        {
            NodeSelectorModelItem* parent = static_cast<NodeSelectorModelItem*>(index.parent().internalPointer());
            if(parent)
            {
                int row = parent->indexOf(item);
                beginRemoveRows(index.parent(), row, row);
                mNodeRequesterWorker->lockDataMutex(true);
                auto itemToRemove = parent->findChildNode(node);
                mNodeRequesterWorker->lockDataMutex(false);
                emit removeItem(itemToRemove);
                endRemoveRows();
            }
            else
            {
                int row = index.row();
                beginRemoveRows(index.parent(), row, row);
                emit removeRootItem(item);
                endRemoveRows();
            }
        }
    }
}

int NodeSelectorModel::getNodeAccess(mega::MegaNode* node)
{
    auto parent = std::unique_ptr<mega::MegaNode>(MegaSyncApp->getMegaApi()->getParentNode(node));
    if (parent && node)
    {
        auto access(MegaSyncApp->getMegaApi()->getAccess(node));

        if (access >= mega::MegaShare::ACCESS_FULL && (!node->isNodeKeyDecrypted()))
        {
            return mega::MegaShare::ACCESS_UNKNOWN;
        }

        return access;
    }
    else
    {
        return mega::MegaShare::ACCESS_UNKNOWN;
    }
}

void NodeSelectorModel::moveNode(std::shared_ptr<mega::MegaNode> moveNode,
                                 std::shared_ptr<mega::MegaNode> targetParentFolder)
{
    MegaSyncApp->getMegaApi()->moveNode(moveNode.get(), targetParentFolder.get(), mListener.get());
}

void NodeSelectorModel::onRequestFinish(mega::MegaRequest* request, mega::MegaError* e)
{
    if (request->getType() == mega::MegaRequest::TYPE_MOVE ||
        request->getType() == mega::MegaRequest::TYPE_REMOVE)
    {
        auto requestType = mRequestByHandle.take(request->getNodeHandle());
        if (e->getErrorCode() != mega::MegaError::API_OK)
        {
            mRequestFailedByHandle.insert(request->getNodeHandle(), requestType);

            std::unique_ptr<mega::MegaNode> node(
                MegaSyncApp->getMegaApi()->getNodeByHandle(request->getNodeHandle()));
            if (node)
            {
                MovedItemsType itemType =
                    node->isFile() ? MovedItemsType::FILES : MovedItemsType::FOLDERS;
                mMovedItemsType |= itemType;
            }
        }

        auto multipleRequest(mRequestFailedByHandle.size() > 1);

        if (mRequestByHandle.isEmpty())
        {
            if (!mRequestFailedByHandle.isEmpty())
            {
                QMegaMessageBox::MessageBoxInfo msgInfo;
                msgInfo.title = MegaSyncApp->getMEGAString();

                if (requestType == mega::MegaRequest::TYPE_REMOVE)
                {
                    if (multipleRequest)
                    {
                        if (mMovedItemsType.testFlag(MovedItemsType::BOTH))
                        {
                            msgInfo.text = tr("Error removing items");
                            msgInfo.informativeText =
                                tr("The items couldn’t be removed. Try again later");
                        }
                        else if (mMovedItemsType.testFlag(MovedItemsType::FILES))
                        {
                            msgInfo.text = tr("Error removing files");
                            msgInfo.informativeText =
                                tr("The files couldn’t be removed. Try again later");
                        }
                        else if (mMovedItemsType.testFlag(MovedItemsType::FOLDERS))
                        {
                            msgInfo.text = tr("Error removing folders");
                            msgInfo.informativeText =
                                tr("The folders couldn’t be removed. Try again later");
                        }
                    }
                    else
                    {
                        std::unique_ptr<mega::MegaNode> node(
                            MegaSyncApp->getMegaApi()->getNodeByHandle(
                                mRequestFailedByHandle.firstKey()));

                        if (node->isFile())
                        {
                            msgInfo.text = tr("Error removing file");
                            msgInfo.informativeText =
                                tr("The file %1 couldn’t be removed. Try again later")
                                    .arg(MegaNodeNames::getNodeName(node.get()));
                        }
                        else
                        {
                            msgInfo.text = tr("Error removing folder");
                            msgInfo.informativeText =
                                tr("The folder %1 couldn’t be removed. Try again later")
                                    .arg(MegaNodeNames::getNodeName(node.get()));
                        }
                    }
                }

                //Reset value
                mMovedItemsType = MovedItemsType::NONE;

                emit showMessageBox(msgInfo);
            }

            mRequestFailedByHandle.clear();
            mRequestByHandle.clear();

            emit blockUi(false);
        }
    }
}

void NodeSelectorModel::showFiles(bool show)
{
    mNodeRequesterWorker->setShowFiles(show);
}

void NodeSelectorModel::showReadOnlyFolders(bool show)
{
    mNodeRequesterWorker->setShowReadOnlyFolders(show);
}

QVariant NodeSelectorModel::getIcon(const QModelIndex &index, NodeSelectorModelItem* item) const
{
    switch(index.column())
    {
    case COLUMN::NODE:
    {
        return QVariant::fromValue<QIcon>(getFolderIcon(item));
    }
    case COLUMN::DATE:
    {
        break;
    }
    case COLUMN::USER:
    {
        return QVariant::fromValue<QPixmap>(item->getOwnerIcon());
    }
    case COLUMN::STATUS:
    {
        return QVariant::fromValue<QIcon>(item->getStatusIcons());
    }
    default:
        break;
    }
    return QVariant();
}

QVariant NodeSelectorModel::getText(const QModelIndex &index, NodeSelectorModelItem *item) const
{
    switch(index.column())
    {
        case COLUMN::NODE:
        {
            if(item->isVault() || item->isCloudDrive())
            {
                return MegaNodeNames::getRootNodeName(item->getNode().get());
            }
            else
            {
                return MegaNodeNames::getNodeName(item->getNode().get());
            }
        }
        case COLUMN::DATE:
        {
            if(item->isCloudDrive() || item->isVault())
            {
                return QVariant();
            }

            QDateTime dateTime = dateTime.fromSecsSinceEpoch(item->getNode()->getCreationTime());
            return MegaSyncApp->getFormattedDateByCurrentLanguage(dateTime, QLocale::FormatType::ShortFormat);
        }
        default:
            break;
    }
    return QVariant();
}

NodeSelectorModel::IndexesActionInfo NodeSelectorModel::needsToBeExpandedAndSelected()
{
    IndexesActionInfo info = mIndexesActionInfo;
    clearIndexesNodeInfo(mIndexesActionInfo.needsToBeSelected);

    return info;
}

void NodeSelectorModel::clearIndexesNodeInfo(bool select)
{
    mIndexesActionInfo = IndexesActionInfo();
    mIndexesActionInfo.needsToBeSelected = select;
}

void NodeSelectorModel::abort()
{
    mNodeRequesterWorker->cancelCurrentRequest();
}

bool NodeSelectorModel::canBeDeleted() const
{
    return true;
}

void NodeSelectorModel::loadTreeFromNode(const std::shared_ptr<mega::MegaNode> node)
{
    //First, we set the loading view as it can take long to load the tree path to the node
    emit blockUi(true);

    clearIndexesNodeInfo();
    mIndexesActionInfo.needsToBeSelected = true;

    mNodesToLoad.clear();
    mNodesToLoad.append(node);

    auto p_node = std::shared_ptr<mega::MegaNode>(MegaSyncApp->getMegaApi()->getParentNode(node.get()));

    //The vault node is not represented in the node selector, hence if the parent of a node is the vault
    //it doesn´t have to be added to the node list to load. If it is added the loading of a specific node
    //will stops working in backups screen.
    while(addToLoadingList(p_node))
    {
        mNodesToLoad.append(p_node);
        p_node.reset(MegaSyncApp->getMegaApi()->getParentNode(p_node.get()));
    }

    if(!fetchMoreRecursively(QModelIndex()))
    {
        emit blockUi(false);
        mNodesToLoad.clear();
    }
}

bool NodeSelectorModel::fetchMoreRecursively(const QModelIndex& parentIndex)
{
    auto result(false);
    if(!mNodesToLoad.isEmpty())
    {
        auto node = mNodesToLoad.last();
        if(node)
        {
            auto indexToCheck = getIndexFromNode(node, parentIndex);
            if(canFetchMore(indexToCheck))
            {
                fetchMore(indexToCheck);
                result = true;
            }
            else
            {
                mIndexesActionInfo.indexesToBeExpanded.append(qMakePair(node->getHandle(),indexToCheck));
                result = continueWithNextItemToLoad(indexToCheck);
            }
        }
    }

    return result;
}

QModelIndex NodeSelectorModel::getIndexFromNode(const std::shared_ptr<mega::MegaNode> node, const QModelIndex &parent)
{
    if(node)
    {
        auto childrenCount = rowCount(parent);
        for(int row = 0; row < childrenCount; ++row)
        {
            auto indexToCheck = index(row,0,parent);
            NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(indexToCheck.internalPointer());
            if(item)
            {
                if(item->getNode()->getHandle() == node->getHandle())
                {
                    return indexToCheck;
                }
            }
        }
    }

    return QModelIndex();
}

void NodeSelectorModel::rootItemsLoaded()
{
    endResetModel();
}

void NodeSelectorModel::addRootItems()
{
    emit blockUi(true);
    beginResetModel();
    createRootNodes();
}

void NodeSelectorModel::loadLevelFinished()
{
   emit levelsAdded(mIndexesActionInfo.indexesToBeExpanded);
}

bool NodeSelectorModel::canFetchMore(const QModelIndex &parent) const
{
    if(!parent.isValid())
    {
        return false;
    }
    NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(parent.internalPointer());
    if(item)
    {
        return item->canFetchMore();
    }
    else
    {
        return mNodeRequesterWorker->rootIndexSize() < rootItemsCount();
    }
}

bool NodeSelectorModel::isRequestingNodes() const
{
    return mNodeRequesterWorker->isRequestingNodes();
}

void NodeSelectorModel::fetchItemChildren(const QModelIndex& parent)
{
    emit blockUi(true);

    NodeSelectorModelItem* item = static_cast<NodeSelectorModelItem*>(parent.internalPointer());
    if(!item->areChildrenInitialized() && !item->requestingChildren())
    {
        int itemNumChildren = item->getNumChildren();
        if(itemNumChildren > 0)
        {
            blockSignals(true);
            beginInsertRows(parent, 0, itemNumChildren-1);
            blockSignals(false);
            emit requestChildNodes(item, parent);
        }
        else
        {
            emit blockUi(false);
        }
    }
    else
    {
        emit blockUi(false);
    }
}

void NodeSelectorModel::onChildNodesReady(NodeSelectorModelItem* parent)
{
    auto index = parent->property(INDEX_PROPERTY).value<QModelIndex>();
    mIndexesActionInfo.indexesToBeExpanded.append(qMakePair(parent->getNode()->getHandle(), index));
    continueWithNextItemToLoad(index);
}

bool NodeSelectorModel::continueWithNextItemToLoad(const QModelIndex& parentIndex)
{
    bool result = false;

    if(!mNodesToLoad.isEmpty())
    {
        //The last one has been already processed
        mNodesToLoad.removeLast();
        if(!mNodesToLoad.isEmpty())
        {
            result = fetchMoreRecursively(parentIndex);
            if(!result && !mNodesToLoad.isEmpty())
            {
                //The last node is empty
                mNodesToLoad.removeLast();
            }
        }
    }

    if(mNodesToLoad.isEmpty())
    {
        loadLevelFinished();
    }
    return result;
}

QModelIndex NodeSelectorModel::findItemByNodeHandle(const mega::MegaHandle& handle, const QModelIndex &parent)
{
    for(int i = 0; i < rowCount(parent); ++i)
    {
        QModelIndex idx = index(i, COLUMN::NODE, parent);
        if(idx.isValid())
        {
            if(NodeSelectorModelItem* chkItem = static_cast<NodeSelectorModelItem*>(idx.internalPointer()))
            {
                if(chkItem->getNode()->getHandle() == handle)
                {
                    return idx;
                }
            }
        }
    }
    for(int i = 0; i < rowCount(parent); ++i)
    {
        QModelIndex child = parent.isValid() ? index(i, COLUMN::NODE, parent) : index(i, COLUMN::NODE);
        if(child.isValid())
        {
            auto ret = findItemByNodeHandle(handle, child);
            if(ret.isValid())
            {
                return ret;
            }
        }
    }

    return QModelIndex();
}

NodeSelectorModelItem* NodeSelectorModel::getItemByIndex(const QModelIndex &index)
{
    return qvariant_cast<NodeSelectorModelItem*>(index.data(toInt(NodeSelectorModelRoles::MODEL_ITEM_ROLE)));
}

void NodeSelectorModel::updateItemNode(const QModelIndex &indexToUpdate, std::shared_ptr<mega::MegaNode> node)
{
    auto item = getItemByIndex(indexToUpdate);
    if(item)
    {
        item->updateNode(node);
        auto lastColumnIndex = index(indexToUpdate.row(), columnCount()-1, indexToUpdate.parent());
        emit dataChanged(indexToUpdate, lastColumnIndex);
    }
}

void NodeSelectorModel::updateRow(const QModelIndex& indexToUpdate)
{
    auto firstColumnIndex = index(indexToUpdate.row(), 0, indexToUpdate.parent());
    auto lastColumnIndex = index(indexToUpdate.row(), columnCount()-1, indexToUpdate.parent());
    emit dataChanged(firstColumnIndex, lastColumnIndex);
}

QIcon NodeSelectorModel::getFolderIcon(NodeSelectorModelItem *item) const
{
    if(item)
    {
        auto node = item->getNode();

        if(node)
        {
            if (node->getType() >= mega::MegaNode::TYPE_FOLDER)
            {
                if(node->getHandle() == mCameraFolderAttribute->getCameraUploadFolderHandle()
                        || node->getHandle() == mCameraFolderAttribute->getCameraUploadFolderSecondaryHandle())
                {
                    QIcon icon;
                    icon.addFile(QLatin1String("://images/icons/folder/small-camera-sync.png"), QSize(), QIcon::Normal);
                    icon.addFile(QLatin1String("://images/icons/folder/small-folder-camera-sync-disabled.png"), QSize(), QIcon::Disabled);
                    return icon;;
                }
                else if(node->getHandle() == mMyChatFilesFolderAttribute->getMyChatFilesFolderHandle())
                {
                    QIcon icon;
                    icon.addFile(QLatin1String("://images/icons/folder/small-chat-files.png"), QSize(), QIcon::Normal);
                    icon.addFile(QLatin1String("://images/icons/folder/small-chat-files-disabled.png"), QSize(), QIcon::Disabled);
                    return icon;
                }
                else if (node->isInShare())
                {
                    QIcon icon;
                    icon.addFile(QLatin1String("://images/icons/folder/small-folder-incoming.png"), QSize(), QIcon::Normal);
                    icon.addFile(QLatin1String("://images/icons/folder/small-folder-incoming-disabled.png"), QSize(), QIcon::Disabled);
                    return icon;
                }
                else if (node->isOutShare())
                {
                    QIcon icon;
                    icon.addFile(QLatin1String("://images/icons/folder/small-folder-outgoing.png"), QSize(), QIcon::Normal);
                    icon.addFile(QLatin1String("://images/icons/folder/small-folder-outgoing-disabled.png"), QSize(), QIcon::Disabled);
                    return icon;
                }
                else if(node->getHandle() == MegaSyncApp->getRootNode()->getHandle())
                {
                    QIcon icon;
                    icon.addFile(QLatin1String("://images/ico-cloud-drive.png"));
                    return icon;
                }
                else if(item->isVault())
                {
                    QIcon icon;
                    icon.addFile(QLatin1String("://images/node_selector/Backups_small_ico.png"));
                    return icon;
                }
                else
                {
                    QString nodeDeviceId (QString::fromUtf8(node->getDeviceId()));
                    if (!nodeDeviceId.isEmpty())
                    {
                        // TODO, future: choose icon according to host OS
                        if (nodeDeviceId == QString::fromUtf8(MegaSyncApp->getMegaApi()->getDeviceId()))
                        {
#ifdef Q_OS_WINDOWS
                            const QIcon thisDeviceIcon (QLatin1String("://images/icons/pc/pc-win_24.png"));
#elif defined(Q_OS_MACOS)
                            const QIcon thisDeviceIcon (QLatin1String("://images/icons/pc/pc-mac_24.png"));
#elif defined(Q_OS_LINUX)
                            const QIcon thisDeviceIcon (QLatin1String("://images/icons/pc/pc-linux_24.png"));
#endif
                            return thisDeviceIcon;
                        }
                        return QIcon(QLatin1String("://images/icons/pc/pc_24.png"));
                    }
                    QIcon icon;
                    icon.addFile(QLatin1String("://images/icons/folder/small-folder.png"), QSize(), QIcon::Normal);
                    icon.addFile(QLatin1String("://images/icons/folder/small-folder-disabled.png"), QSize(), QIcon::Disabled);
                    return icon;
                }
            }
            else
            {
                return Utilities::getExtensionPixmapSmall(QString::fromUtf8(node->getName()));
            }
        }
    }

    return QIcon();
}
