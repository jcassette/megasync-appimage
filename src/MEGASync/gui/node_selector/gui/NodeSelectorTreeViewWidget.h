#ifndef NODESELECTORTREEVIEWWIDGET_H
#define NODESELECTORTREEVIEWWIDGET_H

#include "ButtonIconManager.h"
#include "QTMegaListener.h"
#include <megaapi.h>
#include "NodeSelectorModelItem.h"
#include <ViewLoadingScene.h>

#include <QWidget>
#include <QDebug>
#include <QItemSelectionModel>
#include <QPersistentModelIndex>
#include <memory>


class NodeSelectorProxyModel;
class NodeSelectorModel;
class NodeSelectorModelItem;
class SelectType;
typedef std::shared_ptr<SelectType> SelectTypeSPtr;

namespace Ui {
class NodeSelectorTreeViewWidget;
}

class NodeSelectorTreeViewWidget : public QWidget,  public mega::MegaListener
{
    Q_OBJECT

    struct Navigation{
      QList<mega::MegaHandle> forwardHandles = QList<mega::MegaHandle>();
      QList<mega::MegaHandle> backwardHandles = QList<mega::MegaHandle>();

      void removeFromForward(const mega::MegaHandle& handle);
      void remove(const mega::MegaHandle& handle);

      void appendToBackward(const mega::MegaHandle& handle);
      void appendToForward(const mega::MegaHandle& handle);
      void clear();
    };

public:
    static const int LOADING_VIEW_THRESSHOLD;
    static const int LABEL_ELIDE_MARGIN;
    static const char* FULL_NAME_PROPERTY;

    explicit NodeSelectorTreeViewWidget(SelectTypeSPtr mode, QWidget *parent = nullptr);
    ~NodeSelectorTreeViewWidget();
    mega::MegaHandle getSelectedNodeHandle();
    void init();
    QList<mega::MegaHandle> getMultiSelectionNodeHandle();
    void setSelectedNodeHandle(const mega::MegaHandle &selectedHandle, bool goToInit = false);
    void setFutureSelectedNodeHandle(const mega::MegaHandle &selectedHandle);
    void setDefaultUploadOption(bool value);
    bool getDefaultUploadOption();
    void showDefaultUploadOption(bool show);
    void setSearchText(const QString& text);
    void setTitleText(const QString& nodeName);
    void clearSearchText();
    void clearSelection();
    void abort();
    NodeSelectorModelItem* rootItem();
    NodeSelectorProxyModel* getProxyModel();

public slots:
    void onNodesUpdate(mega::MegaApi *, mega::MegaNodeList *nodes) override;
    void onRowsInserted();
    void onRowsRemoved();
    void onProxyModelSorted();

signals:
    void okBtnClicked();
    void cancelBtnClicked();
    void onSearch(const QString& text);

protected:
    void showEvent(QShowEvent* ) override;
    void resizeEvent(QResizeEvent* ) override;
    void mousePressEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void setTitle(const QString& title);
    void selectionChanged(const QModelIndexList &selected);
    QModelIndex getParentIncomingShareByIndex(QModelIndex idx);
    SelectTypeSPtr getSelectType(){return mSelectType;}
    virtual void modelLoaded();
    virtual bool showEmptyView(){return true;}
    virtual bool newNodeCanBeAdded(mega::MegaNode*){return true;}
    virtual QModelIndex getAddedNodeParent(mega::MegaHandle parentHandle);

    Ui::NodeSelectorTreeViewWidget *ui;
    std::unique_ptr<NodeSelectorProxyModel> mProxyModel;
    std::unique_ptr<NodeSelectorModel> mModel;
    Navigation mNavigationInfo;

protected slots:
    virtual bool containsIndexToAddOrUpdate(mega::MegaNode* node, const mega::MegaHandle& parentHandle);

private slots:
    void onbNewFolderClicked();
    void oncbAlwaysUploadToLocationChanged(bool value);
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onDeleteClicked();
    void onRenameClicked();
    void onGenMEGALinkClicked();
    virtual void onItemDoubleClick(const QModelIndex &index);
    void onGoForwardClicked();
    void onGoBackClicked();
    void onRemoveIndexFromGoBack(const QModelIndex &index);
    void onSectionResized();
    void onExpandReady();
    void setLoadingSceneVisible(bool visible);
    void onUiBlocked(bool state);
    void processCachedNodesUpdated();
    void removeItemByHandle(mega::MegaHandle handle);

private:

    mega::MegaApi* mMegaApi;
    bool mManuallyResizedColumn;
    std::unique_ptr<mega::QTMegaListener> mDelegateListener;

    virtual bool isAllowedToEnterInIndex(const QModelIndex &idx);
    QModelIndex getSelectedIndex();
    void checkBackForwardButtons();
    void setRootIndex(const QModelIndex& proxy_idx);
    virtual QIcon getEmptyIcon();
    virtual void onRootIndexChanged(const QModelIndex& source_idx){Q_UNUSED(source_idx)};
    mega::MegaHandle getHandleByIndex(const QModelIndex& idx);
    QModelIndex getIndexFromHandle(const mega::MegaHandle &handle);
    void checkNewFolderButtonVisibility();
    virtual QString getRootText() = 0;
    virtual std::unique_ptr<NodeSelectorProxyModel> createProxyModel();
    virtual std::unique_ptr<NodeSelectorModel> createModel() = 0;
    virtual bool newFolderBtnVisibleInRoot(){return true;}
    virtual bool newFolderBtnCanBeVisisble(){return true;}
    void checkOkButton(const QModelIndexList& selected);
    bool shouldUpdateImmediately();
    bool areThereNodesToUpdate();
    void selectIndex(const QModelIndex& index, bool setCurrent);

    ButtonIconManager mButtonIconManager;
    bool first;
    bool mUiBlocked;
    mega::MegaHandle mNodeHandleToSelect;
    SelectTypeSPtr mSelectType;
    struct UpdateNodesInfo
    {
      mega::MegaHandle previousHandle = mega::INVALID_HANDLE;
      mega::MegaHandle parentHandle = mega::INVALID_HANDLE;
      std::shared_ptr<mega::MegaNode> node;
    };
    void updateNode(const UpdateNodesInfo& info, bool scrollTo = false);
    QList<UpdateNodesInfo> mRenamedNodesByHandle;
    QList<UpdateNodesInfo> mUpdatedNodesByPreviousHandle;
    QMultiMap<mega::MegaHandle, std::shared_ptr<mega::MegaNode>> mAddedNodesByParentHandle;
    QList<mega::MegaHandle> mRemovedNodesByHandle;
    QList<mega::MegaHandle> mMovedNodesByHandle;
    QTimer mNodesUpdateTimer;
    mega::MegaHandle mNewFolderHandle;
    bool mNewFolderAdded;

    friend class DownloadType;
    friend class SyncType;
    friend class UploadType;
    friend class StreamType;
    friend class CloudDriveType;
};

class SelectType
{
public:
    explicit SelectType() = default;
    virtual ~SelectType() = default;
    virtual bool isAllowedToNavigateInside(const QModelIndex& index);
    virtual void init(NodeSelectorTreeViewWidget* wdg) = 0;
    virtual bool okButtonEnabled(NodeSelectorTreeViewWidget* wdg, const QModelIndexList &selected) = 0;
    virtual void newFolderButtonVisibility(NodeSelectorTreeViewWidget* wdg){Q_UNUSED(wdg)};
    virtual NodeSelectorModelItemSearch::Types allowedTypes() = 0;

protected:
    bool cloudDriveIsCurrentRootIndex(NodeSelectorTreeViewWidget* wdg);
};

class DownloadType : public SelectType
{
public:
    explicit DownloadType() = default;
    void init(NodeSelectorTreeViewWidget* wdg) override;
    bool okButtonEnabled(NodeSelectorTreeViewWidget* wdg, const QModelIndexList &selected) override;
    NodeSelectorModelItemSearch::Types allowedTypes() override;
};

class SyncType : public SelectType
{
public:
    explicit SyncType() = default;
    bool isAllowedToNavigateInside(const QModelIndex& index) override;
    void init(NodeSelectorTreeViewWidget* wdg) override;
    void newFolderButtonVisibility(NodeSelectorTreeViewWidget* wdg) override;
    bool okButtonEnabled(NodeSelectorTreeViewWidget* wdg, const QModelIndexList &selected) override;
    NodeSelectorModelItemSearch::Types allowedTypes() override;
};

class StreamType : public SelectType
{
public:
    explicit StreamType() = default;
    void init(NodeSelectorTreeViewWidget* wdg) override;
    bool okButtonEnabled(NodeSelectorTreeViewWidget* wdg, const QModelIndexList &selected) override;
    NodeSelectorModelItemSearch::Types allowedTypes() override;
};

class UploadType : public SelectType
{
public:
    explicit UploadType() = default;
    void init(NodeSelectorTreeViewWidget* wdg) override;
    void newFolderButtonVisibility(NodeSelectorTreeViewWidget* wdg) override;
    bool okButtonEnabled(NodeSelectorTreeViewWidget* wdg, const QModelIndexList &selected) override;
    NodeSelectorModelItemSearch::Types allowedTypes() override;
};

class MoveBackupType : public UploadType
{
public:
    explicit MoveBackupType() = default;
    NodeSelectorModelItemSearch::Types allowedTypes() override;
};

#endif // NODESELECTORTREEVIEWWIDGET_H

