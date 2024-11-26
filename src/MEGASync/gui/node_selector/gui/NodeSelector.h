#ifndef NODESELECTOR_H
#define NODESELECTOR_H

#include "ButtonIconManager.h"
#include "NewFolderDialog.h"
#include "NodeSelectorTreeViewWidget.h"

#include <QDialog>
#include <QItemSelection>
#include <QTimer>
#include <QGraphicsDropShadowEffect>

#include <memory>


class NodeSelectorProxyModel;
class NodeSelectorModel;
class NodeSelectorTreeViewWidgetCloudDrive;
class NodeSelectorTreeViewWidgetIncomingShares;
class NodeSelectorTreeViewWidgetBackups;
class NodeSelectorTreeViewWidgetSearch;

namespace mega {
class MegaApi;
}

namespace Ui {
class NodeSelector;
}

class NodeSelector : public QDialog
{
    Q_OBJECT

public:
    enum TabItem{
        CLOUD_DRIVE = 0,
        SHARES,
        BACKUPS,
        SEARCH
    };
    Q_ENUM(TabItem)

    explicit NodeSelector(QWidget *parent = 0);

    ~NodeSelector();
    void showDefaultUploadOption(bool show = true);
    void setDefaultUploadOption(bool value);
    bool getDefaultUploadOption();
    void setSelectedNodeHandle(std::shared_ptr<mega::MegaNode> node = nullptr, bool goToInit = false);
    mega::MegaHandle getSelectedNodeHandle();
    QList<mega::MegaHandle> getMultiSelectionNodeHandle();
    void closeEvent(QCloseEvent* event) override;

protected:
    void changeEvent(QEvent * event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void addBackupsView();
    int getNodeAccess(std::shared_ptr<mega::MegaNode> node);
    std::shared_ptr<mega::MegaNode> getSelectedNode();
    void showNotFoundNodeMessageBox();
    void makeConnections(SelectTypeSPtr selectType);

    NodeSelectorTreeViewWidgetCloudDrive* mCloudDriveWidget;
    NodeSelectorTreeViewWidgetIncomingShares* mIncomingSharesWidget;
    NodeSelectorTreeViewWidgetBackups* mBackupsWidget;
    NodeSelectorTreeViewWidgetSearch* mSearchWidget;
    mega::MegaApi* mMegaApi;
    Ui::NodeSelector *ui;

private slots:
    void onbShowSearchClicked();
    void onbOkClicked();
    void onbShowIncomingSharesClicked();
    void onbShowCloudDriveClicked();
    void onbShowBackupsFolderClicked();
    void onOptionSelected(int index);
    void updateNodeSelectorTabs();
    void onSearch(const QString& text);
    void on_tClearSearchResultNS_clicked();

private:
    QModelIndex getParentIncomingShareByIndex(QModelIndex idx);
    void setToggledStyle(TabItem item);
    void setAllFramesItsOnProperty();
    virtual void checkSelection() = 0;
    void shortCutConnects(int ignoreThis);
    ButtonIconManager mButtonIconManager;
    QGraphicsDropShadowEffect* mShadowTab;
    QMap<TabItem, QFrame*> mTabFramesToggleGroup;

    bool mManuallyResizedColumn;
};

#endif // NODESELECTOR_H
