#ifndef INFODIALOG_H
#define INFODIALOG_H

#include <QDialog>
#include <QTimer>
#include <QProcess>
#include <QDateTime>
#include <QPainter>
#include <QAbstractItemDelegate>

#include "SettingsDialog.h"
#include "MenuItemAction.h"
#include "Preferences.h"
#include "SyncInfo.h"
#include <QGraphicsOpacityEffect>
#include "TransferScanCancelUi.h"
#include "HighDpiResize.h"
#include "Utilities.h"
#include "FilterAlertWidget.h"
#include "QtPositioningBugFixer.h"
#include "TransferQuota.h"
#include "StatusInfo.h"
#include "SyncsMenu.h"
#include "MegaDelegateHoverManager.h"

#include <memory>
#ifdef _WIN32
#include <chrono>
#endif

namespace Ui {
class InfoDialog;
}

class MegaApplication;
class TransferManager;

class InfoDialog : public QDialog
{
    Q_OBJECT

    enum {
        STATE_STARTING,
        STATE_PAUSED,
        STATE_WAITING,
        STATE_INDEXING,
        STATE_UPDATED,
        STATE_SYNCING,
        STATE_TRANSFERRING,
    };

public:

    enum {
        STATE_NONE = -1,
        STATE_LOGOUT = 0,
        STATE_LOGGEDIN = 1,
        STATE_LOCKED_EMAIL = mega::MegaApi::ACCOUNT_BLOCKED_VERIFICATION_EMAIL
    };

    explicit InfoDialog(MegaApplication *app, QWidget *parent = 0, InfoDialog* olddialog = nullptr);
    ~InfoDialog();

    PSA_info* getPSAdata();
    void setUsage();
    void setAvatar();
    void setIndexing(bool indexing);
    void setWaiting(bool waiting);
    void setSyncing(bool syncing);
    void setTransferring(bool transferring);
    void setOverQuotaMode(bool state);
    void setAccountType(int accType);
    void setDisabledSyncTags(QSet<int> tags);
    void addBackup();
    void clearUserAttributes();
    void setPSAannouncement(int id, QString title, QString text, QString urlImage, QString textButton, QString linkButton);
    bool updateOverStorageState(int state);

    void reset();

    void enterBlockingState();
    void leaveBlockingState(bool fromCancellation);
    void disableCancelling();
    void setUiInCancellingStage();
    void updateUiOnFolderTransferUpdate(const FolderTransferUpdateEvent& event);

    void on_bStorageDetails_clicked();
    HighDpiResize<QDialog> highDpiResize;
#ifdef _WIN32
    std::chrono::steady_clock::time_point lastWindowHideTime;
#endif

    int getLoggedInMode() const;
    void showNotifications();

    void move(int x, int y);

    void setTransferManager(TransferManager *transferManager);

private:
    InfoDialog() = delete;
    void animateStates(bool opt);
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void moveEvent(QMoveEvent *) override;

public slots:

    void pauseResumeClicked();
    void generalAreaClicked();
    void dlAreaClicked();
    void upAreaClicked();

    void pauseResumeHovered(QMouseEvent *event);
    void generalAreaHovered(QMouseEvent *event);
    void dlAreaHovered(QMouseEvent *event);
    void upAreaHovered(QMouseEvent *event);

    void addSync(mega::MegaHandle handle = mega::INVALID_HANDLE);
    void onAddSync(mega::MegaSync::SyncType type = mega::MegaSync::TYPE_TWOWAY);
    void onAddBackup();
    void updateDialogState();

    void enableTransferOverquotaAlert();
    void enableTransferAlmostOverquotaAlert();
    void setBandwidthOverquotaState(QuotaState state);
    void updateUsageAndAccountType();

   void onUnseenAlertsChanged(const UnseenUserMessagesMap& alerts);

private slots:
    void on_bSettings_clicked();
    void on_bUpgrade_clicked();
    void on_bUpgradeOverDiskQuota_clicked();
    void openFolder(QString path);
    void onOverlayClicked();
    void on_bTransferManager_clicked();
    void on_bAddSync_clicked();
    void on_bAddBackup_clicked();
    void on_bUpload_clicked();
    void resetLoggedInMode();

    void on_tTransfers_clicked();
    void on_tNotifications_clicked();
    void onActualFilterClicked();
    void applyFilterOption(MessageType opt);
    void on_bNotificationsSettings_clicked();

    void on_bDiscard_clicked();
    void on_bBuyQuota_clicked();

    void onAnimationFinished();

    void sTabsChanged(int tab);

    void on_bDismissSyncSettings_clicked();
    void on_bOpenSyncSettings_clicked();
    void on_bDismissBackupsSettings_clicked();
    void on_bOpenBackupsSettings_clicked();
    void on_bDismissAllSyncsSettings_clicked();
    void on_bOpenAllSyncsSettings_clicked();

    void updateTransfersCount();

    void onResetTransfersSummaryWidget();
    void onTransfersStateChanged();

    void onStalledIssuesChanged();

signals:

    void openTransferManager(int tab);
    void dismissStorageOverquota(bool oq);
    // signal emitted when showing or dismissing the overquota message.
    // parameter messageShown is true when alert is enabled, false when dismissed
    void transferOverquotaMsgVisibilityChange(bool messageShown);
    // signal emitted when showing or dismissing the almost overquota message.
    // parameter messageShown is true when alert is enabled, false when dismissed
    void almostTransferOverquotaMsgVisibilityChange(bool messageShown);
    void userActivity();
    void cancelScanning();

private:
    Ui::InfoDialog *ui;
    QPushButton *overlay;

    FilterAlertWidget* filterMenu;

    MenuItemAction *cloudItem;
    MenuItemAction *sharesItem;
    MenuItemAction *rubbishItem;

    int activeDownloadState, activeUploadState;
    bool pendingUploadsTimerRunning = false;
    bool pendingDownloadsTimerRunning = false;
    bool circlesShowAllActiveTransfersProgress;
    void showSyncsMenu(QPushButton* b, mega::MegaSync::SyncType type);
    SyncsMenu* initSyncsMenu(mega::MegaSync::SyncType type, bool isEnabled);
    void setUnseenNotifications(long long value);

    bool mIndexing; //scanning
    bool mWaiting;
    bool mSyncing; //if any sync is in syncing state
    bool mTransferring; // if there are ongoing regular transfers
    StatusInfo::TRANSFERS_STATES mState;
    bool overQuotaState;
    bool transferOverquotaAlertEnabled;
    bool transferAlmostOverquotaAlertEnabled;
    int storageState;
    QuotaState transferQuotaState;
    int actualAccountType;
    int loggedInMode = STATE_NONE;
    bool notificationsReady = false;
    bool isShown = false;

    QPointer<TransferManager> mTransferManager;

#ifdef Q_OS_LINUX
    bool doNotActAsPopup;
#endif

    QPropertyAnimation *animation;
    QGraphicsOpacityEffect *opacityEffect;

    bool mShownSomeIssuesOccurred = false;
    QPropertyAnimation *minHeightAnimationSomeIssues;
    QPropertyAnimation *maxHeightAnimationSomeIssues;
    QParallelAnimationGroup animationGroupSomeIssues;
    void hideSomeIssues();
    void showSomeIssues();
    QHash<QPushButton*, SyncsMenu*> mSyncsMenus;
    MegaDelegateHoverManager mNotificationsViewHoverManager;

    void updateUpgradeButtonText();

protected:
    void updateBlockedState();
    void updateState();
    bool checkFailedState();
    void changeEvent(QEvent * event) override;
    bool eventFilter(QObject *obj, QEvent *e) override;

protected:
    QDateTime lastPopupUpdate;
    QTimer downloadsFinishedTimer;
    QTimer uploadsFinishedTimer;
    QTimer transfersFinishedTimer;
    QTimer mResetTransferSummaryWidget;
    MegaApplication *app;
    std::shared_ptr<Preferences> mPreferences;
    SyncInfo *mSyncInfo;
    mega::MegaApi *megaApi;
    mega::MegaTransfer *activeDownload;
    mega::MegaTransfer *activeUpload;

 private:
    static double computeRatio(long long completed, long long remaining);
    void enableUserActions(bool newState);
    void changeStatusState(StatusInfo::TRANSFERS_STATES newState,
                           bool animate = true);
    void fixMultiscreenResizeBug(int& posX, int& posY);
    void repositionInfoDialog();
    void initNotificationArea();

    TransferScanCancelUi* mTransferScanCancelUi = nullptr;
    QtPositioningBugFixer qtBugFixer;
};

#endif // INFODIALOG_H
