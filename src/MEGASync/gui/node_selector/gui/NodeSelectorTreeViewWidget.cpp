#include "ui_NodeSelectorTreeViewWidget.h"
#include "NodeSelectorTreeViewWidget.h"
#include "NodeSelectorTreeViewWidgetSpecializations.h"
#include "NodeSelectorModel.h"
#include "NodeSelectorDelegates.h"
#include "MegaApplication.h"
#include "QMegaMessageBox.h"
#include "NodeSelectorProxyModel.h"
#include "NodeSelectorModel.h"
#include "RenameNodeDialog.h"
#include "DialogOpener.h"
#include "MegaNodeNames.h"
#include "NewFolderDialog.h"

const int NodeSelectorTreeViewWidget::LOADING_VIEW_THRESSHOLD = 500;
const int NodeSelectorTreeViewWidget::LABEL_ELIDE_MARGIN = 250;
const char* NodeSelectorTreeViewWidget::FULL_NAME_PROPERTY = "full_name";
const int CHECK_UPDATED_NODES_INTERVAL = 1000;
const int IMMEDIATE_CHECK_UPDATES_NODES_THRESHOLD = 200;

NodeSelectorTreeViewWidget::NodeSelectorTreeViewWidget(SelectTypeSPtr mode, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NodeSelectorTreeViewWidget),
    mProxyModel(nullptr),
    mModel(nullptr),
    mMegaApi(MegaSyncApp->getMegaApi()),
    mManuallyResizedColumn(false),
    mDelegateListener(new QTMegaListener(mMegaApi, this)),
    first(true),
    mUiBlocked(false),
    mNodeHandleToSelect(INVALID_HANDLE),
    mSelectType(mode),
    mNewFolderHandle(mega::INVALID_HANDLE),
    mNewFolderAdded(false)
{
    ui->setupUi(this);
    setFocusProxy(ui->tMegaFolders);
    ui->cbAlwaysUploadToLocation->hide();
    ui->bOk->setDefault(true);
    ui->bOk->setEnabled(false);
    ui->searchButtonsWidget->setVisible(false);
    ui->searchingText->setVisible(false);

    MegaSyncApp->getMegaApi()->addListener(mDelegateListener.get());

    connect(ui->bNewFolder, &QPushButton::clicked, this, &NodeSelectorTreeViewWidget::onbNewFolderClicked);
    connect(ui->bOk, &QPushButton::clicked, this, &NodeSelectorTreeViewWidget::okBtnClicked);
    connect(ui->bCancel, &QPushButton::clicked, this, &NodeSelectorTreeViewWidget::cancelBtnClicked);
    connect(ui->cbAlwaysUploadToLocation, &QCheckBox::stateChanged, this, &NodeSelectorTreeViewWidget::oncbAlwaysUploadToLocationChanged);
    connect(ui->leSearch, &SearchLineEdit::search, this, &NodeSelectorTreeViewWidget::onSearch);
    checkBackForwardButtons();

    connect(&ui->tMegaFolders->loadingView(), &ViewLoadingSceneBase::sceneVisibilityChange, this, &NodeSelectorTreeViewWidget::onUiBlocked);

    foreach(auto& button, ui->searchButtonsWidget->findChildren<QAbstractButton*>())
    {
        button->setProperty(ButtonIconManager::CHANGE_LATER, true);
        mButtonIconManager.addButton(button);
    }


    connect(&mNodesUpdateTimer, &QTimer::timeout, this, &NodeSelectorTreeViewWidget::processCachedNodesUpdated);
    mNodesUpdateTimer.start(CHECK_UPDATED_NODES_INTERVAL);
}

NodeSelectorTreeViewWidget::~NodeSelectorTreeViewWidget()
{
    delete ui;
}

void NodeSelectorTreeViewWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        if(!ui->tMegaFolders->rootIndex().isValid())
        {
            ui->lFolderName->setText(getRootText());
        }
        ui->retranslateUi(this);
    }
    QWidget::changeEvent(event);
}

void NodeSelectorTreeViewWidget::init()
{
    mProxyModel = createProxyModel();
    mModel = createModel();
    ui->emptyIcon->setIcon(getEmptyIcon());
    mSelectType->init(this);

    connect(mProxyModel.get(), &NodeSelectorProxyModel::expandReady, this, &NodeSelectorTreeViewWidget::onExpandReady);
    connect(mProxyModel.get(), &NodeSelectorProxyModel::modelSorted, this, &NodeSelectorTreeViewWidget::onProxyModelSorted);
    connect(mModel.get(), &QAbstractItemModel::rowsInserted, this, &NodeSelectorTreeViewWidget::onRowsInserted);
    connect(mModel.get(), &QAbstractItemModel::rowsRemoved, this, &NodeSelectorTreeViewWidget::onRowsRemoved);
    connect(mModel.get(), &NodeSelectorModel::blockUi, this, &NodeSelectorTreeViewWidget::setLoadingSceneVisible);

    ui->tMegaFolders->setSortingEnabled(true);
    mProxyModel->setSourceModel(mModel.get());

#ifdef __APPLE__
ui->tMegaFolders->setAnimated(false);
#endif
}

void NodeSelectorTreeViewWidget::showDefaultUploadOption(bool show)
{
    ui->cbAlwaysUploadToLocation->setVisible(show);
}

void NodeSelectorTreeViewWidget::setSearchText(const QString &text)
{
    ui->leSearch->setText(text);
}

void NodeSelectorTreeViewWidget::setTitleText(const QString &nodeName)
{
    ui->lFolderName->setProperty(FULL_NAME_PROPERTY, nodeName);

    QFontMetrics fm = ui->lFolderName->fontMetrics();

    QString elidedText = fm.elidedText(nodeName, Qt::ElideMiddle, ui->tMegaFolders->width() - LABEL_ELIDE_MARGIN);
    ui->lFolderName->setText(elidedText);

    if(elidedText != nodeName)
        ui->lFolderName->setToolTip(nodeName);
    else
        ui->lFolderName->setToolTip(QString());

}

void NodeSelectorTreeViewWidget::clearSearchText()
{
    ui->leSearch->onClearClicked();
}

void NodeSelectorTreeViewWidget::clearSelection()
{
    ui->tMegaFolders->clearSelection();
}

void NodeSelectorTreeViewWidget::abort()
{
    mModel->abort();
}

NodeSelectorModelItem* NodeSelectorTreeViewWidget::rootItem()
{
    auto rootIndex = ui->tMegaFolders->rootIndex();
    if(!rootIndex.isValid())
    {
        //Top parent
        rootIndex = mModel->index(0,0,QModelIndex());
    }

    return mModel->getItemByIndex(rootIndex);
}

NodeSelectorProxyModel* NodeSelectorTreeViewWidget::getProxyModel()
{
    return mProxyModel.get();
}

void NodeSelectorTreeViewWidget::setDefaultUploadOption(bool value)
{
    ui->cbAlwaysUploadToLocation->setChecked(value);
}

bool NodeSelectorTreeViewWidget::getDefaultUploadOption()
{
    return ui->cbAlwaysUploadToLocation->isChecked();
}
void NodeSelectorTreeViewWidget::setTitle(const QString &title)
{
    ui->lFolderName->setText(title);
}

void NodeSelectorTreeViewWidget::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::BackButton && ui->bBack->isEnabled())
    {
       onGoBackClicked();
    }
    else if(event->button() == Qt::ForwardButton && ui->bForward->isEnabled())
    {
       onGoForwardClicked();
    }
}

void NodeSelectorTreeViewWidget::showEvent(QShowEvent* )
{
    if(!mManuallyResizedColumn)
    {
        ui->tMegaFolders->setColumnWidth(NodeSelectorModel::COLUMN::NODE, qRound(ui->stackedWidget->width() * 0.50));
    }
}

void NodeSelectorTreeViewWidget::resizeEvent(QResizeEvent *)
{
    if(!mManuallyResizedColumn)
    {
        ui->tMegaFolders->setColumnWidth(NodeSelectorModel::COLUMN::NODE, qRound(ui->stackedWidget->width() * 0.50));
    }

    setTitleText(ui->lFolderName->property(FULL_NAME_PROPERTY).toString());
}

void NodeSelectorTreeViewWidget::onSectionResized()
{
    if(!mManuallyResizedColumn
            && ui->tMegaFolders->header()->rect().contains(ui->tMegaFolders->mapFromGlobal(QCursor::pos())))
    {
        mManuallyResizedColumn = true;
    }
}

void NodeSelectorTreeViewWidget::onRowsInserted()
{
    if(first)
    {
        first = false;
        setSelectedNodeHandle(mNodeHandleToSelect);
    }

    checkBackForwardButtons();
    modelLoaded();
}

void NodeSelectorTreeViewWidget::onRowsRemoved()
{
    checkBackForwardButtons();
    modelLoaded();
}

void NodeSelectorTreeViewWidget::onProxyModelSorted()
{
    if (mNewFolderAdded)
    {
        mNewFolderAdded = false;
        onItemDoubleClick(mProxyModel->getIndexFromHandle(mNewFolderHandle));
        mNewFolderHandle = mega::INVALID_HANDLE;
    }
}

void NodeSelectorTreeViewWidget::onExpandReady()
{
    if(ui->tMegaFolders->model() == nullptr)
    {
        ui->tMegaFolders->setContextMenuPolicy(Qt::DefaultContextMenu);
        ui->tMegaFolders->setExpandsOnDoubleClick(false);
        ui->tMegaFolders->setHeader(new NodeSelectorTreeViewHeaderView(Qt::Horizontal));
        ui->tMegaFolders->setItemDelegate(new NodeRowDelegate(ui->tMegaFolders));
        ui->tMegaFolders->setItemDelegateForColumn(NodeSelectorModel::STATUS, new IconDelegate(ui->tMegaFolders));
        ui->tMegaFolders->setItemDelegateForColumn(NodeSelectorModel::USER, new IconDelegate(ui->tMegaFolders));
        ui->tMegaFolders->setItemDelegateForColumn(NodeSelectorModel::DATE, new DateColumnDelegate(ui->tMegaFolders));
        ui->tMegaFolders->setTextElideMode(Qt::ElideMiddle);

        ui->tMegaFolders->sortByColumn(NodeSelectorModel::NODE, Qt::AscendingOrder);
        ui->tMegaFolders->setModel(mProxyModel.get());

        ui->tMegaFolders->header()->show();
        ui->tMegaFolders->header()->setFixedHeight(NodeSelectorModel::ROW_HEIGHT);
        ui->tMegaFolders->header()->moveSection(NodeSelectorModel::STATUS, NodeSelectorModel::NODE);
        ui->tMegaFolders->setColumnWidth(NodeSelectorModel::COLUMN::STATUS, NodeSelectorModel::ROW_HEIGHT * 2);
        ui->tMegaFolders->header()->setProperty("HeaderIconCenter", true);
        showEvent(nullptr);

        //those connects needs to be done after the model is set, do not move them

        connect(ui->tMegaFolders->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NodeSelectorTreeViewWidget::onSelectionChanged);
        connect(ui->tMegaFolders, &NodeSelectorTreeView::removeNodeClicked, this, &NodeSelectorTreeViewWidget::onDeleteClicked);
        connect(ui->tMegaFolders, &NodeSelectorTreeView::renameNodeClicked, this, &NodeSelectorTreeViewWidget::onRenameClicked);
        connect(ui->tMegaFolders, &NodeSelectorTreeView::getMegaLinkClicked, this, &NodeSelectorTreeViewWidget::onGenMEGALinkClicked);
        connect(ui->tMegaFolders, &QTreeView::doubleClicked, this, &NodeSelectorTreeViewWidget::onItemDoubleClick);
        connect(ui->tMegaFolders, &NodeSelectorTreeView::nodeSelected, this, &NodeSelectorTreeViewWidget::okBtnClicked);
        connect(ui->bForward, &QPushButton::clicked, this, &NodeSelectorTreeViewWidget::onGoForwardClicked);
        connect(ui->bBack, &QPushButton::clicked, this, &NodeSelectorTreeViewWidget::onGoBackClicked);
        connect(ui->tMegaFolders->header(), &QHeaderView::sectionResized, this, &NodeSelectorTreeViewWidget::onSectionResized);

        setRootIndex(QModelIndex());
        checkNewFolderButtonVisibility();
    }

    auto indexesAndSelected = mModel->needsToBeExpandedAndSelected();
    if(!indexesAndSelected.indexesToBeExpanded.isEmpty())
    {
        for (auto it = indexesAndSelected.indexesToBeExpanded.begin(); it != indexesAndSelected.indexesToBeExpanded.end(); ++it)
        {
            QModelIndex proxyIndex;
            auto handle((*it).first);
            if(handle != mega::INVALID_HANDLE)
            {
                proxyIndex = mProxyModel->getIndexFromHandle(handle);
            }
            else
            {
                proxyIndex = mProxyModel->mapFromSource((*it).second);
            }

            ui->tMegaFolders->setExpanded(proxyIndex, true);

            if((*it) == indexesAndSelected.indexesToBeExpanded.last())
            {
                if(indexesAndSelected.needsToBeSelected)
                {
                    selectIndex(proxyIndex, true);
                }

                if(indexesAndSelected.needsToBeEntered)
                {
                    onItemDoubleClick(proxyIndex);
                }
            }
        }
    }
}

void NodeSelectorTreeViewWidget::onGoBackClicked()
{
    auto rootIndex(ui->tMegaFolders->rootIndex());
    auto rootIndexHandle(getHandleByIndex(rootIndex));
    if(rootIndexHandle != mega::INVALID_HANDLE)
    {
        mNavigationInfo.appendToForward(rootIndexHandle);
    }
    QModelIndex indexToGo = getIndexFromHandle(mNavigationInfo.backwardHandles.last());
    mNavigationInfo.backwardHandles.removeLast();

    setRootIndex(indexToGo);
    checkBackForwardButtons();
    checkNewFolderButtonVisibility();

    if(rootIndex.isValid())
    {
        selectIndex(rootIndex, true);
    }
}

void NodeSelectorTreeViewWidget::onRemoveIndexFromGoBack(const QModelIndex& currentRootIndex)
{
    auto parentIndex(currentRootIndex.parent());
    if(parentIndex.isValid())
    {
        auto indexHandle(getHandleByIndex(parentIndex));

        if(mNavigationInfo.backwardHandles.contains(indexHandle))
        {
            mNavigationInfo.backwardHandles.removeLast();
            setRootIndex(parentIndex);
        }
        else
        {
            setRootIndex(QModelIndex());
            mNavigationInfo.backwardHandles.clear();
        }
    }
}

void NodeSelectorTreeViewWidget::onGoForwardClicked()
{
    mNavigationInfo.appendToBackward(getHandleByIndex(ui->tMegaFolders->rootIndex()));
    QModelIndex indexToGo = getIndexFromHandle(mNavigationInfo.forwardHandles.last());
    mNavigationInfo.forwardHandles.removeLast();
    setRootIndex(indexToGo);
    checkBackForwardButtons();
    checkNewFolderButtonVisibility();
}

MegaHandle NodeSelectorTreeViewWidget::getHandleByIndex(const QModelIndex& idx)
{
    return mProxyModel ? mProxyModel->getHandle(idx) : mega::INVALID_HANDLE;
}

QModelIndex NodeSelectorTreeViewWidget::getIndexFromHandle(const mega::MegaHandle &handle)
{
    return mProxyModel ? mProxyModel->getIndexFromHandle(handle) : QModelIndex();
}

void NodeSelectorTreeViewWidget::checkNewFolderButtonVisibility()
{
    mSelectType->newFolderButtonVisibility(this);
}

void NodeSelectorTreeViewWidget::onbNewFolderClicked()
{
    auto parentNode = mProxyModel->getNode(ui->tMegaFolders->rootIndex());
    if (!parentNode)
    {
        parentNode = MegaSyncApp->getRootNode();
        if (!parentNode)
            return;
    }

    QPointer<NewFolderDialog> dialog(new NewFolderDialog(parentNode, ui->tMegaFolders));
    dialog->init();
    DialogOpener::showDialog(dialog,  [this, dialog]()
    {
        auto newNode = dialog->getNewNode();
        //IF the dialog return a node, there are two scenarios:
        //1) The dialog has been accepted, a new folder has been created
        //2) The dialog has been rejected because the folder already exists. If so, select the existing folder
        if(newNode)
        {
            mNewFolderHandle = newNode->getHandle();
            mNewFolderAdded = true;
#ifdef Q_OS_LINUX
            //It seems that the NodeSelector is not activated when the NewFolderDialog is closed,
            //so the ui->tMegaFolders is not correctly focused
            qApp->setActiveWindow(parentWidget()->parentWidget());
#endif

            //Set the focus to the view to allow the user to press enter (or go back, in a future feature)
            ui->tMegaFolders->setFocus();
        }
    });
}

void NodeSelectorTreeViewWidget::oncbAlwaysUploadToLocationChanged(bool value)
{
    foreach(auto& child, parent()->children())
    {
        if(auto tvw = qobject_cast<NodeSelectorTreeViewWidget*>(child))
        {
            if(tvw != sender())
            {
                tvw->setDefaultUploadOption(value);
            }
        }
    }
}

bool NodeSelectorTreeViewWidget::isAllowedToEnterInIndex(const QModelIndex &idx)
{
    return mSelectType->isAllowedToNavigateInside(idx);
}

void NodeSelectorTreeViewWidget::onItemDoubleClick(const QModelIndex &index)
{
    if(!isAllowedToEnterInIndex(index))
        return;

    mNavigationInfo.appendToBackward(getHandleByIndex(ui->tMegaFolders->rootIndex()));
    mNavigationInfo.removeFromForward(mProxyModel->getHandle(index));

    setRootIndex(index);
    checkBackForwardButtons();
    checkNewFolderButtonVisibility();

    selectIndex(index, true);
}

std::unique_ptr<NodeSelectorProxyModel> NodeSelectorTreeViewWidget::createProxyModel()
{
    return std::unique_ptr<NodeSelectorProxyModel>(new NodeSelectorProxyModel);
}

void NodeSelectorTreeViewWidget::setLoadingSceneVisible(bool blockUi)
{
    ui->tMegaFolders->loadingView().toggleLoadingScene(blockUi);

    if(!blockUi)
    {
        modelLoaded();
    }
}

void NodeSelectorTreeViewWidget::modelLoaded()
{
    if(mModel)
    {
        if(mModel->rowCount() == 0 && showEmptyView())
        {
            ui->stackedWidget->setCurrentWidget(ui->emptyPage);
            return;
        }
    }
    ui->stackedWidget->setCurrentWidget(ui->treeViewPage);
}

QModelIndex NodeSelectorTreeViewWidget::getAddedNodeParent(mega::MegaHandle parentHandle)
{
    return mModel->findItemByNodeHandle(parentHandle, QModelIndex());
}

void NodeSelectorTreeViewWidget::onUiBlocked(bool state)
{
    if(mUiBlocked != state)
    {
        mUiBlocked = state;

        ui->bNewFolder->setDisabled(state);
        ui->bCancel->setDisabled(state);
        ui->searchButtonsWidget->setDisabled(state);

        if(!state)
        {
            if(auto selectionModel = ui->tMegaFolders->selectionModel())
            {
                checkOkButton(selectionModel->selectedRows());
            }
            checkBackForwardButtons();
        }
        else
        {
            ui->bBack->setEnabled(false);
            ui->bForward->setEnabled(false);
            ui->bOk->setDisabled(true);
        }

        processCachedNodesUpdated();
    }
}

void NodeSelectorTreeViewWidget::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)
    Q_UNUSED(selected)

    if(!mUiBlocked)
    {
        if(auto selectionModel = ui->tMegaFolders->selectionModel())
        {
            checkOkButton(selectionModel->selectedRows());
        }
    }
}

void NodeSelectorTreeViewWidget::checkOkButton(const QModelIndexList &selected)
{
    ui->bOk->setEnabled(mSelectType->okButtonEnabled(this, selected));
}

void NodeSelectorTreeViewWidget::onRenameClicked()
{
    auto node = std::unique_ptr<MegaNode>(mMegaApi->getNodeByHandle(getSelectedNodeHandle()));
    int access = mMegaApi->getAccess(node.get());
    //This is for an extra protection as we don´t show the rename action if one of this conditions are not met
    if (!node || access < MegaShare::ACCESS_FULL  || !node->isNodeKeyDecrypted())
    {
        return;
    }

    QPointer<RenameRemoteNodeDialog> dialog(new RenameRemoteNodeDialog(std::move(node), this));
    dialog->init();
    DialogOpener::showDialog(dialog);
}

void NodeSelectorTreeViewWidget::onDeleteClicked()
{
    QMegaMessageBox::MessageBoxInfo msgInfo;
    msgInfo.parent = ui->tMegaFolders;
    msgInfo.title = MegaSyncApp->getMEGAString();
    msgInfo.informativeText = tr(
        "Any shared files or folders will no longer be accessible to the people you shared "
        "them with. You can still access these items in the Rubbish bin, restore, and share "
        "them.");
    msgInfo.buttons = QMessageBox::Yes | QMessageBox::No;
    msgInfo.defaultButton = QMessageBox::Yes;
    msgInfo.buttonsText.insert(QMessageBox::Yes, tr("Move"));
    msgInfo.buttonsText.insert(QMessageBox::No, tr("Don’t move"));

    auto selectedRows = ui->tMegaFolders->selectionModel()->selectedRows();

    if(selectedRows.size() == 1)
    {
        QPersistentModelIndex p_index(selectedRows.first());

        if(auto savedNode = mProxyModel->getNode(p_index))
        {
            auto node = mModel->getNodeToRemove(savedNode->getHandle());
            if(node)
            {
                msgInfo.text =
                    tr("Move “%1” to Rubbish bin?").arg(QString::fromUtf8(node->getName()));
            }
        }
    }
    else
    {
        msgInfo.text = tr("Move %n item to Rubbish bin?", "", selectedRows.size());
    }

    msgInfo.finishFunc = [this, selectedRows](QPointer<QMessageBox> msg) {
        if (msg->result() == QMessageBox::Yes)
        {
            QList<mega::MegaHandle> nodeHandles;

            foreach(auto& index, selectedRows)
            {
                nodeHandles.append(getHandleByIndex(index));
            }

            mModel->removeNodes(nodeHandles, false);
        }
    };

    QMegaMessageBox::question(msgInfo);
}

bool NodeSelectorTreeViewWidget::containsIndexToAddOrUpdate(mega::MegaNode* node, const mega::MegaHandle& parentHandle)
{
    if(parentHandle != mega::INVALID_HANDLE)
    {
        auto parentIndex = mModel->findItemByNodeHandle(parentHandle, QModelIndex());
        if(parentIndex.isValid())
        {
            auto parentItem = mModel->getItemByIndex(parentIndex);
            if(parentItem->areChildrenInitialized())
            {
                if(node)
                {
                    auto index = mModel->findItemByNodeHandle(node->getHandle(), parentIndex);
                    if(index.isValid())
                    {
                        return true;
                    }
                }
                else
                {
                    return true;
                }
            }
        }
    }
    else if(node)
    {
        auto index = mModel->findItemByNodeHandle(node->getHandle(), QModelIndex());
        if(index.isValid())
        {
            return true;
        }
    }

    return false;
}

void NodeSelectorTreeViewWidget::onNodesUpdate(mega::MegaApi*, mega::MegaNodeList *nodes)
{
    if(!nodes)
    {
        return;
    }

    QMap<mega::MegaHandle, mega::MegaHandle> updatedVersions;
    QList<UpdateNodesInfo> updatedNodes;

    for (int i = 0; i < nodes->size(); i++)
    {
        MegaNode* node = nodes->get(i);

        if(mModel->rootNodeUpdated(node))
        {
            continue;
        }

        if(node->getParentHandle() != mega::INVALID_HANDLE)
        {
            if (node->getChanges() & (MegaNode::CHANGE_TYPE_PARENT |
                                      MegaNode::CHANGE_TYPE_NEW))
            {
                if(MegaSyncApp->getMegaApi()->isInRubbish(node))
                {
                    if(containsIndexToAddOrUpdate(node, mega::INVALID_HANDLE))
                    {
                        mRemovedNodesByHandle.append(node->getHandle());
                    }
                }
                else
                {
                    std::unique_ptr<mega::MegaNode> parentNode(MegaSyncApp->getMegaApi()->getNodeByHandle(node->getParentHandle()));
                    if(parentNode)
                    {
                        if(parentNode->isFile() && containsIndexToAddOrUpdate(node,  mega::INVALID_HANDLE))
                        {
                            updatedVersions.insert(parentNode->getHandle(), node->getHandle());
                        }
                        //Check if the node exists or if we need to add it
                        else if(parentNode->isFolder() && (containsIndexToAddOrUpdate(node, mega::INVALID_HANDLE) ||
                                                              containsIndexToAddOrUpdate(nullptr, node->getParentHandle())))
                        {
                            UpdateNodesInfo info;
                            info.parentHandle = parentNode->getHandle();
                            info.previousHandle = node->getHandle();
                            info.node = std::shared_ptr<mega::MegaNode>(node->copy());
                            updatedNodes.append(info);
                        }
                    }
                }
            }
            else if(node->getChanges() & MegaNode::CHANGE_TYPE_NAME)
            {
                if(containsIndexToAddOrUpdate(node, node->getParentHandle()))
                {
                    UpdateNodesInfo info;
                    info.previousHandle = node->getHandle();
                    info.node = std::shared_ptr<mega::MegaNode>(node->copy());
                    mRenamedNodesByHandle.append(info);
                }
            }
            //Moved or new version added
            else if(node->getChanges() & MegaNode::CHANGE_TYPE_REMOVED)
            {
                if(containsIndexToAddOrUpdate(node, node->getParentHandle()))
                {
                    if(!mMovedNodesByHandle.contains(node->getHandle()))
                    {
                        mMovedNodesByHandle.append(node->getHandle());
                    }
                }
            }
        }
    }

    foreach(auto updateNode, updatedNodes)
    {
        auto previousHandle = updatedVersions.value(updateNode.previousHandle, mega::INVALID_HANDLE);
        //Updated node
        if(previousHandle != mega::INVALID_HANDLE)
        {
            updateNode.previousHandle = updatedVersions.value(updateNode.previousHandle);
            mUpdatedNodesByPreviousHandle.append(updateNode);
        }
        //New node
        else
        {
            if(newNodeCanBeAdded(updateNode.node.get()) &&
                (!updateNode.node->isFile() || mModel->showFiles()))
            {
                mAddedNodesByParentHandle.insert(updateNode.parentHandle, updateNode.node);
            }
            if(updateNode.node->getChanges() & MegaNode::CHANGE_TYPE_PARENT)
            {
                mRemovedNodesByHandle.append(updateNode.previousHandle);
            }
        }
    }

    if(areThereNodesToUpdate())
    {
        if(shouldUpdateImmediately())
        {
            if(mNodesUpdateTimer.interval() != 0)
            {
                mNodesUpdateTimer.setInterval(0);
            }
        }
        else if(mNodesUpdateTimer.interval() != CHECK_UPDATED_NODES_INTERVAL)
        {
            mNodesUpdateTimer.setInterval(CHECK_UPDATED_NODES_INTERVAL);
        }
    }
}

bool NodeSelectorTreeViewWidget::shouldUpdateImmediately()
{
    int totalSize = mUpdatedNodesByPreviousHandle.size();
    if(totalSize > IMMEDIATE_CHECK_UPDATES_NODES_THRESHOLD)
    {
        return true;
    }
    totalSize += mRemovedNodesByHandle.size();
    if(totalSize > IMMEDIATE_CHECK_UPDATES_NODES_THRESHOLD)
    {
        return true;
    }
    totalSize += mRenamedNodesByHandle.size();
    if(totalSize > IMMEDIATE_CHECK_UPDATES_NODES_THRESHOLD)
    {
        return true;
    }
    totalSize += mAddedNodesByParentHandle.size();
    if(totalSize > IMMEDIATE_CHECK_UPDATES_NODES_THRESHOLD)
    {
        return true;
    }
    totalSize += mMovedNodesByHandle.size();
    if(totalSize > IMMEDIATE_CHECK_UPDATES_NODES_THRESHOLD)
    {
        return true;
    }
    return false;
}

bool NodeSelectorTreeViewWidget::areThereNodesToUpdate()
{
    return !mUpdatedNodesByPreviousHandle.isEmpty() ||
        !mRemovedNodesByHandle.isEmpty() ||
        !mRenamedNodesByHandle.isEmpty() ||
        !mAddedNodesByParentHandle.isEmpty() ||
        !mMovedNodesByHandle.isEmpty();
}

void NodeSelectorTreeViewWidget::selectIndex(const QModelIndex& index, bool setCurrent)
{
    if(setCurrent)
    {
        ui->tMegaFolders->selectionModel()->setCurrentIndex(
            index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
    ui->tMegaFolders->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    ui->tMegaFolders->scrollTo(index, QAbstractItemView::ScrollHint::PositionAtCenter);
}

void NodeSelectorTreeViewWidget::removeItemByHandle(mega::MegaHandle handle)
{
    auto index = mModel->findItemByNodeHandle(handle, QModelIndex());
    if(index.isValid())
    {
        auto proxyIndex(mProxyModel->mapFromSource(index));
        if(proxyIndex.isValid())
        {
            auto goBack(proxyIndex == ui->tMegaFolders->rootIndex());
            if(goBack)
            {
                onRemoveIndexFromGoBack(proxyIndex);
            }
            else if(mNavigationInfo.forwardHandles.contains(handle))
            {
                mNavigationInfo.forwardHandles.removeLast();
            }

            checkBackForwardButtons();
            checkNewFolderButtonVisibility();

            mProxyModel->removeNode(proxyIndex);
            mNavigationInfo.remove(handle);
        }
    }
}

void NodeSelectorTreeViewWidget::processCachedNodesUpdated()
{
    //We check if the model is being modified (insert rows, remove rows...etc) before each action in order to avoid
    //calling twice to begininsertrows (as some of these actions are performed in different threads...)
    if(!mProxyModel->isModelProcessing() && !mModel->isRequestingNodes() && areThereNodesToUpdate())
    {
        if(!mModel->isBeingModified())
        {
            foreach(auto info, mRenamedNodesByHandle)
            {
                updateNode(info, true);
            }
            mRenamedNodesByHandle.clear();
        }

        if(!mModel->isBeingModified())
        {
            foreach(auto info, mUpdatedNodesByPreviousHandle)
            {
                updateNode(info, false);
                //If they have been updated, we don´t need to remove them
                mMovedNodesByHandle.removeOne(info.node->getHandle());
            }
            mUpdatedNodesByPreviousHandle.clear();
        }

        if(!mModel->isBeingModified())
        {
            foreach(auto handle, mMovedNodesByHandle)
            {
                removeItemByHandle(handle);
            }
            mMovedNodesByHandle.clear();
        }

        if(!mModel->isBeingModified())
        {
            foreach(auto handle, mRemovedNodesByHandle)
            {
                removeItemByHandle(handle);
            }
            mRemovedNodesByHandle.clear();
        }

        if(!mModel->isBeingModified())
        {
            if(!mAddedNodesByParentHandle.isEmpty())
            {
                mProxyModel->setExpandMapped(true);
            }
            foreach(auto& parentHandle, mAddedNodesByParentHandle.uniqueKeys())
            {
                auto parentIndex = getAddedNodeParent(parentHandle);
                mModel->addNodes(mAddedNodesByParentHandle.values(parentHandle), parentIndex);
            }

            mAddedNodesByParentHandle.clear();
        }
    }
}

void NodeSelectorTreeViewWidget::updateNode(const UpdateNodesInfo &info, bool scrollTo)
{
    auto index = mModel->findItemByNodeHandle(info.previousHandle, QModelIndex());
    auto proxyIndex = mProxyModel->mapFromSource(index);

    auto isSelected(false);

    if(scrollTo)
    {
        if(ui->tMegaFolders->selectionModel())
        {
            if(proxyIndex.isValid())
            {
                isSelected = ui->tMegaFolders->selectionModel()->isSelected(proxyIndex);
            }
        }
    }

    mModel->updateItemNode(index, info.node);

    if(info.node)
    {
        if(proxyIndex.isValid() &&
           ui->tMegaFolders->rootIndex() == proxyIndex)
        {
            setTitleText(MegaNodeNames::getNodeName(info.node.get()));
        }
    }

    if(isSelected)
    {
        //The proxy index may has changed,, update it
        proxyIndex = mProxyModel->mapFromSource(index);
        ui->tMegaFolders->scrollTo(proxyIndex, QAbstractItemView::ScrollHint::PositionAtCenter);
    }
}

void NodeSelectorTreeViewWidget::setSelectedNodeHandle(const MegaHandle& selectedHandle, bool goToInit)
{
    if(selectedHandle == INVALID_HANDLE)
    {
        return;
    }

    auto node = std::shared_ptr<MegaNode>(mMegaApi->getNodeByHandle(selectedHandle));
    if (!node)
        return;

    if(goToInit)
    {
        mNavigationInfo.clear();
        setRootIndex(QModelIndex());
        checkBackForwardButtons();
    }

    mModel->loadTreeFromNode(node);
}

void NodeSelectorTreeViewWidget::setFutureSelectedNodeHandle(const mega::MegaHandle &selectedHandle)
{
    mNodeHandleToSelect = selectedHandle;
}

MegaHandle NodeSelectorTreeViewWidget::getSelectedNodeHandle()
{
    return ui->tMegaFolders->getSelectedNodeHandle();
}

QList<MegaHandle> NodeSelectorTreeViewWidget::getMultiSelectionNodeHandle()
{
    return ui->tMegaFolders->getMultiSelectionNodeHandle();
}

QModelIndex NodeSelectorTreeViewWidget::getSelectedIndex()
{
    QModelIndex ret;
    if(ui->tMegaFolders->selectionModel()->selectedRows().size() > 0)
        ret = ui->tMegaFolders->selectionModel()->selectedRows().at(0);
    return ret;
}

void NodeSelectorTreeViewWidget::checkBackForwardButtons()
{
    ui->bBack->setEnabled(!mNavigationInfo.backwardHandles.isEmpty());
    ui->bForward->setEnabled(!mNavigationInfo.forwardHandles.isEmpty());
}

void NodeSelectorTreeViewWidget::setRootIndex(const QModelIndex &proxy_idx)
{    
    //In case the idx is coming from a potentially hidden column, we always take the NODE column
    //As it is the only one that have childrens
    auto node_column_idx = proxy_idx.sibling(proxy_idx.row(), NodeSelectorModel::COLUMN::NODE);

    ui->tMegaFolders->setRootIndex(node_column_idx);

    //Everytime we move among folders, we reset the selection
    ui->tMegaFolders->selectionModel()->clear();

    onRootIndexChanged(node_column_idx);

    if(!node_column_idx.isValid())
    {
        setTitleText(getRootText());

        ui->lOwnerIcon->setPixmap(QPixmap());
        ui->avatarSpacer->spacerItem()->changeSize(0, 0);
        ui->lIcon->setPixmap(QPixmap());
        ui->syncSpacer->spacerItem()->changeSize(0, 0);
        return;
    }

    //Taking the sync icon
    auto status_column_idx = proxy_idx.sibling(proxy_idx.row(), NodeSelectorModel::COLUMN::STATUS);
    QIcon syncIcon = qvariant_cast<QIcon>(status_column_idx.data(Qt::DecorationRole));


    if(!syncIcon.isNull())
    {
        QPixmap pm = syncIcon.pixmap(QSize(NodeSelectorModelItem::ICON_SIZE, NodeSelectorModelItem::ICON_SIZE), QIcon::Normal);
        ui->lIcon->setPixmap(pm);
        ui->syncSpacer->spacerItem()->changeSize(10, 0);
    }
    else
    {
        ui->lIcon->setPixmap(QPixmap());
        ui->syncSpacer->spacerItem()->changeSize(0, 0);
    }

    auto item = NodeSelectorModel::getItemByIndex(node_column_idx);
    if(!item)
    {
        return;
    }

    auto node = item->getNode();
    if(node)
    {
        setTitleText(MegaNodeNames::getNodeName(node.get()));
    }
}

QIcon NodeSelectorTreeViewWidget::getEmptyIcon()
{
    return QIcon();
}

QModelIndex NodeSelectorTreeViewWidget::getParentIncomingShareByIndex(QModelIndex idx)
{
    while(idx.isValid())
    {
        if(auto item = NodeSelectorModel::getItemByIndex(idx))
        {
            if(item->getNode()->isInShare())
            {
                return idx;
            }
            else
            {
                idx = idx.parent();
            }
        }
    }
    return QModelIndex();
}


void NodeSelectorTreeViewWidget::onGenMEGALinkClicked()
{
    auto node = std::unique_ptr<MegaNode>(mMegaApi->getNodeByHandle(getSelectedNodeHandle()));
    if (!node || node->getType() == MegaNode::TYPE_ROOT
            || mMegaApi->getAccess(node.get()) != MegaShare::ACCESS_OWNER)
    {
        return;
    }
    mMegaApi->exportNode(node.get());
}

void NodeSelectorTreeViewWidget::Navigation::removeFromForward(const mega::MegaHandle &handle)
{
    if(forwardHandles.isEmpty())
        return;

    auto megaApi = MegaSyncApp->getMegaApi();
    auto p_node = std::unique_ptr<mega::MegaNode>(megaApi->getNodeByHandle(handle));

    QMap<MegaHandle, MegaHandle> parentHandles;
    while(p_node)
    {
        MegaHandle actualHandle = p_node->getHandle();
        p_node.reset(megaApi->getParentNode(p_node.get()));
        MegaHandle parentHandle= INVALID_HANDLE;
        if(p_node)
            parentHandle = p_node->getHandle();
        parentHandles.insert(parentHandle, actualHandle);
    }

    p_node.reset(megaApi->getNodeByHandle(forwardHandles.last()));
    QMap<MegaHandle, MegaHandle> actualListParentHandles;
    while(p_node)
    {
        MegaHandle actualHandle = p_node->getHandle();
        p_node.reset(megaApi->getParentNode(p_node.get()));
        MegaHandle parentHandle= INVALID_HANDLE;
        if(p_node)
            parentHandle = p_node->getHandle();
        actualListParentHandles.insert(parentHandle, actualHandle);
    }

    for(auto it = actualListParentHandles.begin(); it != actualListParentHandles.end(); ++it)
    {
        if(parentHandles.contains(it.key()))
        {
            forwardHandles.clear();
            return;
        }
    }
}

void NodeSelectorTreeViewWidget::Navigation::remove(const mega::MegaHandle &handle)
{
    backwardHandles.removeAll(handle);
    int forwardPos = forwardHandles.indexOf(handle);
    for(int i = 0; i <= forwardPos; i++)
    {
        forwardHandles.removeFirst();
    }
}

void NodeSelectorTreeViewWidget::Navigation::appendToBackward(const mega::MegaHandle &handle)
{
    if(!backwardHandles.contains(handle))
        backwardHandles.append(handle);
}

void NodeSelectorTreeViewWidget::Navigation::appendToForward(const mega::MegaHandle &handle)
{
    if(!forwardHandles.contains(handle))
        forwardHandles.append(handle);
}

void NodeSelectorTreeViewWidget::Navigation::clear()
{
    backwardHandles.clear();
    forwardHandles.clear();
}

bool SelectType::isAllowedToNavigateInside(const QModelIndex &index)
{
    auto item = NodeSelectorModel::getItemByIndex(index);
    if(!item)
    {
        return false;
    }
    return !(item->getNode()->isFile() || item->isCloudDrive());
}

bool SelectType::cloudDriveIsCurrentRootIndex(NodeSelectorTreeViewWidget* wdg)
{
    auto result(false);
    auto rootItem = wdg->rootItem();
    if(rootItem)
    {
        result = rootItem->getNode() &&
                 (rootItem->getNode()->getHandle() == MegaSyncApp->getRootNode()->getHandle());
    }

    return result;
}

void DownloadType::init(NodeSelectorTreeViewWidget *wdg)
{
    wdg->ui->bNewFolder->hide();
    wdg->ui->tMegaFolders->setSelectionMode(QAbstractItemView::ExtendedSelection);
    wdg->mModel->showFiles(true);
    wdg->mModel->showReadOnlyFolders(true);
}

bool DownloadType::okButtonEnabled(NodeSelectorTreeViewWidget* wdg, const QModelIndexList &selected)
{
    auto result(!selected.isEmpty());
    if(selected.isEmpty())
    {
        result = !cloudDriveIsCurrentRootIndex(wdg);
    }

    return result;
}

NodeSelectorModelItemSearch::Types DownloadType::allowedTypes()
{
    return NodeSelectorModelItemSearch::Type::CLOUD_DRIVE
            | NodeSelectorModelItemSearch::Type::INCOMING_SHARE
            | NodeSelectorModelItemSearch::Type::BACKUP;
}

void SyncType::init(NodeSelectorTreeViewWidget *wdg)
{
    wdg->mModel->setSyncSetupMode(true);
    wdg->ui->bNewFolder->setVisible(wdg->newFolderBtnCanBeVisisble());
    wdg->mModel->showFiles(false);
    wdg->mModel->showReadOnlyFolders(false);
}

void SyncType::newFolderButtonVisibility(NodeSelectorTreeViewWidget *wdg)
{
    if(wdg->newFolderBtnCanBeVisisble())
    {
        auto sourceIndex = wdg->mProxyModel->getIndexFromSource(wdg->ui->tMegaFolders->rootIndex());
        wdg->ui->bNewFolder->setVisible(sourceIndex.isValid() || wdg->newFolderBtnVisibleInRoot());
    }
}

bool SyncType::okButtonEnabled(NodeSelectorTreeViewWidget*, const QModelIndexList &selected)
{
    bool enable(false);
    if(!selected.isEmpty() && selected.size() < 2)
    {
        auto& index = selected.at(0);
        bool isSyncable = index.data(toInt(NodeSelectorModelRoles::IS_SYNCABLE_FOLDER_ROLE)).toBool();
        bool isFile = index.data(toInt(NodeSelectorModelRoles::IS_FILE_ROLE)).toBool();
        if(isSyncable && !isFile)
        {
            enable = true;
        }
    }
    return enable;
}

NodeSelectorModelItemSearch::Types SyncType::allowedTypes()
{
    return NodeSelectorModelItemSearch::Type::CLOUD_DRIVE | NodeSelectorModelItemSearch::Type::INCOMING_SHARE;
}

bool SyncType::isAllowedToNavigateInside(const QModelIndex& index)
{
    if(!SelectType::isAllowedToNavigateInside(index))
    {
        return false;
    }
    auto item = NodeSelectorModel::getItemByIndex(index);
    return !(item->getStatus() == NodeSelectorModelItem::Status::SYNC || item->getStatus() == NodeSelectorModelItem::Status::SYNC_CHILD);
}

void StreamType::init(NodeSelectorTreeViewWidget *wdg)
{
    wdg->ui->bNewFolder->hide();
    wdg->mModel->showFiles(true);
    wdg->mModel->showReadOnlyFolders(true);
}

bool StreamType::okButtonEnabled(NodeSelectorTreeViewWidget*, const QModelIndexList &selected)
{
    bool enable(false);
    if(!selected.isEmpty() && selected.size() < 2)
    {
        auto& index = selected.at(0);
        enable = index.data(toInt(NodeSelectorModelRoles::IS_FILE_ROLE)).toBool();
    }
    return enable;
}

NodeSelectorModelItemSearch::Types StreamType::allowedTypes()
{
    return NodeSelectorModelItemSearch::Type::CLOUD_DRIVE
            | NodeSelectorModelItemSearch::Type::INCOMING_SHARE
            | NodeSelectorModelItemSearch::Type::BACKUP;
}

void UploadType::init(NodeSelectorTreeViewWidget *wdg)
{
    wdg->ui->bNewFolder->setVisible(wdg->newFolderBtnCanBeVisisble());
    wdg->mModel->showFiles(false);
    wdg->mModel->showReadOnlyFolders(false);
}

void UploadType::newFolderButtonVisibility(NodeSelectorTreeViewWidget *wdg)
{
    if(wdg->newFolderBtnCanBeVisisble())
    {
        auto sourceIndex = wdg->mProxyModel->getIndexFromSource(wdg->ui->tMegaFolders->rootIndex());
        wdg->ui->bNewFolder->setVisible(sourceIndex.isValid() || wdg->newFolderBtnVisibleInRoot());
    }
}

bool UploadType::okButtonEnabled(NodeSelectorTreeViewWidget* wdg, const QModelIndexList &selected)
{
    auto result(!selected.isEmpty());
    if(selected.isEmpty())
    {
        result = !cloudDriveIsCurrentRootIndex(wdg);
    }

    return result;
}

NodeSelectorModelItemSearch::Types UploadType::allowedTypes()
{
    return NodeSelectorModelItemSearch::Type::CLOUD_DRIVE
            | NodeSelectorModelItemSearch::Type::INCOMING_SHARE;
}

NodeSelectorModelItemSearch::Types MoveBackupType::allowedTypes()
{
    return NodeSelectorModelItemSearch::Type::CLOUD_DRIVE;
}
