#ifndef TRANSFERMANAGER_H
#define TRANSFERMANAGER_H

#include "TransferScanCancelUi.h"
#include "megaapi.h"
#include "Preferences.h"
#include "MenuItemAction.h"
#include "Utilities.h"
#include "TransferItem.h"
#include "TransfersModel.h"
#include "TransferQuota.h"
#include "TransfersWidget.h"
#include "StatusInfo.h"
#include "ButtonIconManager.h"

#include <QGraphicsEffect>
#include <QTimer>
#include <QDialog>
#include <QMenu>

namespace Ui {
class TransferManager;
}

namespace Ui {
class TransferManagerDragBackDrop;
}

class TransferManager : public QDialog
{
    Q_OBJECT

public:
    explicit TransferManager(TransfersWidget::TM_TAB tab, mega::MegaApi *megaApi);
    ~TransferManager();

    void pauseModel(bool state);
    void enterBlockingState();
    void leaveBlockingState(bool fromCancellation);
    void disableCancelling();
    void setUiInCancellingStage();
    void onFolderTransferUpdate(const FolderTransferUpdateEvent& event);

    void setTransferState(const StatusInfo::TRANSFERS_STATES &transferState);

    void toggleTab(TransfersWidget::TM_TAB newTab);
    void toggleTab(int newTab);

public slots:
    void onTransferQuotaStateChanged(QuotaState transferQuotaState);
    void onStorageStateChanged(int storageState);

signals:
    void viewedCompletedTransfers();
    void completedTransfersTabActive(bool);
    void userActivity();
    void showCompleted(bool showCompleted);
    void cancelScanning();
    void retryAllTransfers();
    void aboutToClose();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    static const int SPEED_REFRESH_PERIOD_MS = 700;
    static const int STATS_REFRESH_PERIOD_MS = 1000;

    Ui::TransferManager* mUi;
    mega::MegaApi* mMegaApi;

    QTimer mScanningTimer;
    int mScanningAnimationIndex;

    QTimer mTransferQuotaTimer;

    std::shared_ptr<Preferences> mPreferences;
    QPoint mDragPosition;
    QMap<TransfersWidget::TM_TAB, QFrame*> mTabFramesToggleGroup;
    QMap<TransfersWidget::TM_TAB, QLabel*> mNumberLabelsGroup;
    QMap<TransfersWidget::TM_TAB, QWidget*> mTabNoItem;
    QMap<TransfersWidget::TM_TAB, QPair<int, Qt::SortOrder>> mTabSortCriterion;

    TransfersModel* mModel;
    TransfersCount mTransfersCount;

    bool mSearchFieldReturnPressed;

    QGraphicsDropShadowEffect* mShadowTab;
    QSet<Utilities::FileType> mFileTypesFilter;
    QTimer* mSpeedRefreshTimer;
    QTimer* mStatsRefreshTimer;

    Ui::TransferManagerDragBackDrop* mUiDragBackDrop;
    QWidget* mDragBackDrop;
    TransferScanCancelUi* mTransferScanCancelUi = nullptr;

    int mStorageQuotaState;
    QuotaState mTransferQuotaState;
    bool hasOverQuotaErrors();

    bool mFoundStalledIssues;
    ButtonIconManager mButtonIconManager;

    void refreshStateStats();
    void refreshTypeStats();
    void refreshFileTypesStats();
    void applyTextSearch(const QString& text);
    void enableUserActions(bool enabled);
    void checkActionAndMediaVisibility();
    void onFileTypeButtonClicked(TransfersWidget::TM_TAB tab, Utilities::FileType fileType);
    void checkPauseButtonVisibilityIfPossible();
    void showTransferQuotaBanner(bool state);

    void showAllResults();
    void showDownloadResults();
    void showUploadResults();

    void updateCurrentSearchText();
    void updateCurrentCategoryTitle();
    void updateCurrentOverQuotaLink();

    void filterByTab(TransfersWidget::TM_TAB tab);

    void setStorageTextState(const QVariant& stateValue, const QString& text);
    void updateStorageOQText();

private slots:
    void on_tCompleted_clicked();
    void on_tDownloads_clicked();
    void on_tUploads_clicked();
    void on_tAllTransfers_clicked();
    void on_tFailed_clicked();
    void on_tActionButton_clicked();
    void on_bSearch_clicked();
    void on_leSearchField_editingFinished();
    void on_tSearchIcon_clicked();
    void on_bSearchString_clicked();
    void on_tSearchCancel_clicked();
    void on_tClearSearchResult_clicked();
    void on_bPause_toggled();
    void pauseResumeTransfers(bool isPaused);

    void onStalledIssuesStateChanged();
    void checkContentInfo();
    void on_bOpenLinks_clicked();
    void on_tCogWheel_clicked();
    void on_bDownload_clicked();
    void on_bUpload_clicked();
    void on_leSearchField_returnPressed();

    void on_bArchives_clicked();
    void on_bDocuments_clicked();
    void on_bImages_clicked();
    void on_bAudio_clicked();
    void on_bVideos_clicked();
    void on_bOther_clicked();

    void onUpdatePauseState(bool isPaused);
    void onPauseStateChangedByTransferResume();
    void onPauseResumeVisibleRows(bool isPaused);
    void showQuotaStorageDialogs(bool isPaused);

    void onTransfersDataUpdated();
    void refreshSearchStats();

    void onVerticalScrollBarVisibilityChanged(bool state);

    void refreshSpeed();
    void refreshView();
    void disableTransferManager(bool state);

    void updateTransferWidget(QWidget* widgetToShow);
    void onScanningAnimationUpdate();

    void onTransferQuotaExceededUpdate();

    void onSortCriterionChanged(int sortBy, Qt::SortOrder order);
};

#endif // TRANSFERMANAGER_H
