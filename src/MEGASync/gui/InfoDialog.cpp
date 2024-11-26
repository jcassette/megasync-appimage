#include "InfoDialog.h"

#include "AccountDetailsDialog.h"
#include "AccountDetailsManager.h"
#include "assert.h"
#include "CreateRemoveBackupsManager.h"
#include "CreateRemoveSyncsManager.h"
#include "DialogOpener.h"
#include "GuiUtilities.h"
#include "MegaApplication.h"
#include "MenuItemAction.h"
#include "Platform.h"
#include "QMegaMessageBox.h"
#include "QmlDialogManager.h"
#include "StalledIssuesModel.h"
#include "StatsEventHandler.h"
#include "SyncsComponent.h"
#include "TextDecorator.h"
#include "TransferManager.h"
#include "ui_InfoDialog.h"
#include "UserMessageController.h"
#include "UserMessageDelegate.h"
#include "Utilities.h"

#include <QDesktopServices>
#include <QDesktopWidget>
#include <QEvent>
#include <QFileInfo>
#include <QHelpEvent>
#include <QRect>
#include <QScrollBar>
#include <QSignalMapper>
#include <QTimer>
#include <QToolTip>
#include <QUrl>
#include <QVBoxLayout>

#ifdef _WIN32
#include <chrono>
using namespace std::chrono;
#endif

#include <QtConcurrent/QtConcurrent>

using namespace mega;

static constexpr int DEFAULT_MIN_PERCENTAGE{1};
static constexpr int FONT_SIZE_BUSINESS_PX{20};
static constexpr int FONT_SIZE_NO_BUSINESS_PX{14};

void InfoDialog::pauseResumeClicked()
{
    app->pauseTransfers();
}

void InfoDialog::generalAreaClicked()
{
    app->transferManagerActionClicked(TransfersWidget::ALL_TRANSFERS_TAB);
}

void InfoDialog::dlAreaClicked()
{
    app->transferManagerActionClicked(TransfersWidget::DOWNLOADS_TAB);
}

void InfoDialog::upAreaClicked()
{
    app->transferManagerActionClicked(TransfersWidget::UPLOADS_TAB);
}

void InfoDialog::pauseResumeHovered(QMouseEvent *event)
{
    QToolTip::showText(event->globalPos(), tr("Pause/Resume"));
}

void InfoDialog::generalAreaHovered(QMouseEvent *event)
{
    QToolTip::showText(event->globalPos(), tr("Open Transfer Manager"));
}
void InfoDialog::dlAreaHovered(QMouseEvent *event)
{
    QToolTip::showText(event->globalPos(), tr("Open Downloads"));
}

void InfoDialog::upAreaHovered(QMouseEvent *event)
{
    QToolTip::showText(event->globalPos(), tr("Open Uploads"));
}

InfoDialog::InfoDialog(MegaApplication *app, QWidget *parent, InfoDialog* olddialog) :
    QDialog(parent),
    ui(new Ui::InfoDialog),
    mIndexing (false),
    mWaiting (false),
    mSyncing (false),
    mTransferring (false),
    mTransferManager(nullptr),
    mPreferences (Preferences::instance()),
    mSyncInfo (SyncInfo::instance()),
    qtBugFixer(this)
{
    ui->setupUi(this);

    mSyncsMenus[ui->bAddSync] = nullptr;
    mSyncsMenus[ui->bAddBackup] = nullptr;

    filterMenu = new FilterAlertWidget(this);
    connect(filterMenu, SIGNAL(filterClicked(MessageType)),
            this, SLOT(applyFilterOption(MessageType)));

    setUnseenNotifications(0);

    QSizePolicy sp_retain = ui->bNumberUnseenNotifications->sizePolicy();
    sp_retain.setRetainSizeWhenHidden(true);
    ui->bNumberUnseenNotifications->setSizePolicy(sp_retain);

    connect(ui->bTransferManager, SIGNAL(pauseResumeClicked()), this, SLOT(pauseResumeClicked()));
    connect(ui->bTransferManager, SIGNAL(generalAreaClicked()), this, SLOT(generalAreaClicked()));
    connect(ui->bTransferManager, SIGNAL(upAreaClicked()), this, SLOT(upAreaClicked()));
    connect(ui->bTransferManager, SIGNAL(dlAreaClicked()), this, SLOT(dlAreaClicked()));

    connect(ui->bTransferManager, SIGNAL(pauseResumeHovered(QMouseEvent *)), this, SLOT(pauseResumeHovered(QMouseEvent *)));
    connect(ui->bTransferManager, SIGNAL(generalAreaHovered(QMouseEvent *)), this, SLOT(generalAreaHovered(QMouseEvent *)));
    connect(ui->bTransferManager, SIGNAL(upAreaHovered(QMouseEvent *)), this, SLOT(upAreaHovered(QMouseEvent*)));
    connect(ui->bTransferManager, SIGNAL(dlAreaHovered(QMouseEvent *)), this, SLOT(dlAreaHovered(QMouseEvent *)));

    connect(ui->wSortNotifications, SIGNAL(clicked()), this, SLOT(onActualFilterClicked()));

    connect(app->getTransfersModel(), &TransfersModel::transfersCountUpdated, this, &InfoDialog::updateTransfersCount);
    connect(app->getTransfersModel(), &TransfersModel::transfersProcessChanged, this, &InfoDialog::onTransfersStateChanged);

    connect(mPreferences.get(),
            &Preferences::valueChanged,
            this,
            [this](const QString& key)
            {
                if(key == Preferences::wasPausedKey)
                {
                    ui->bTransferManager->setPaused(mPreferences->getGlobalPaused());
                }
            });

    //Set window properties
#ifdef Q_OS_LINUX
    doNotActAsPopup = Platform::getInstance()->getValue("USE_MEGASYNC_AS_REGULAR_WINDOW", false);

    if (!doNotActAsPopup && QSystemTrayIcon::isSystemTrayAvailable())
    {
        // To avoid issues with text input we implement a popup ourselves
        // instead of using Qt::Popup by listening to the WindowDeactivate
        // event.
        Qt::WindowFlags flags = Qt::FramelessWindowHint;

        if (Platform::getInstance()->isTilingWindowManager())
        {
            flags |= Qt::Dialog;
        }

        setWindowFlags(flags);
    }
    else
    {
        setWindowFlags(Qt::Window);
        doNotActAsPopup = true; //the first time systray is not available will set this flag to true to disallow popup until restarting
    }
#elif defined(_WIN32)
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup | Qt::NoDropShadowWindowHint);
#else // OS X
    setWindowFlags(Qt::FramelessWindowHint | Qt::Popup);
#endif

#ifdef _WIN32
    if (qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS") ||
        qEnvironmentVariableIsSet("QT_SCALE_FACTOR"))
    {
        //do not use WA_TranslucentBackground when using custom scale factors in windows
        setStyleSheet(styleSheet().append(QString::fromUtf8("#wInfoDialogIn{border-radius: 0px;}" ) ));
    }
    else
#endif
    {
        setAttribute(Qt::WA_TranslucentBackground);
    }

    //Initialize fields
    this->app = app;

    circlesShowAllActiveTransfersProgress = true;

    cloudItem = NULL;
    sharesItem = NULL;
    rubbishItem = NULL;
    opacityEffect = NULL;
    animation = NULL;

    actualAccountType = -1;

    notificationsReady = false;
    ui->sNotifications->setCurrentWidget(ui->pNoNotifications);

    overQuotaState = false;
    storageState = Preferences::STATE_BELOW_OVER_STORAGE;

    reset();

    hideSomeIssues();

    initNotificationArea();

    //Initialize header dialog and disable chat features
    ui->wHeader->setStyleSheet(QString::fromUtf8("#wHeader {border: none;}"));

    //Set properties of some widgets
    ui->sActiveTransfers->setCurrentWidget(ui->pUpdated);

    ui->sStorage->setCurrentWidget(ui->wCircularStorage);
    ui->sQuota->setCurrentWidget(ui->wCircularQuota);

    ui->wCircularQuota->setProgressBarGradient(QColor("#60D1FE"), QColor("#58B9F3"));

#ifdef __APPLE__
    auto current = QOperatingSystemVersion::current();
    if (current <= QOperatingSystemVersion::OSXMavericks) //Issues with mavericks and popup management
    {
        installEventFilter(this);
    }
#endif

#ifdef Q_OS_LINUX
    installEventFilter(this);
#endif

    ui->wStorageUsage->installEventFilter(this);

    ui->lOQDesc->setTextFormat(Qt::RichText);

    mState = StatusInfo::TRANSFERS_STATES::STATE_STARTING;
    ui->wStatus->setState(mState);

    megaApi = app->getMegaApi();

    actualAccountType = -1;

    connect(mSyncInfo, SIGNAL(syncDisabledListUpdated()), this, SLOT(updateDialogState()));

    connect(ui->wPSA, SIGNAL(PSAseen(int)), app, SLOT(PSAseen(int)), Qt::QueuedConnection);

    connect(ui->sTabs, SIGNAL(currentChanged(int)), this, SLOT(sTabsChanged(int)), Qt::QueuedConnection);

    on_tTransfers_clicked();

    ui->wListTransfers->setupTransfers();

    //Create the overlay widget with a transparent background
    overlay = new QPushButton(ui->pUpdated);
    overlay->setStyleSheet(QString::fromLatin1("background-color: transparent; "
                                              "border: none; "));
    overlay->resize(ui->pUpdated->size());
    overlay->setCursor(Qt::PointingHandCursor);

    overlay->resize(overlay->width()-4, overlay->height());

    overlay->show();
    connect(overlay, SIGNAL(clicked()), this, SLOT(onOverlayClicked()));
    connect(this, SIGNAL(openTransferManager(int)), app, SLOT(externalOpenTransferManager(int)));

    if (mPreferences->logged())
    {
        setAvatar();
        setUsage();
    }
    highDpiResize.init(this);

#ifdef _WIN32
    lastWindowHideTime = std::chrono::steady_clock::now() - 5s;

    PSA_info *psaData = olddialog ? olddialog->getPSAdata() : nullptr;
    if (psaData)
    {
        this->setPSAannouncement(psaData->idPSA, psaData->title, psaData->desc,
                                 psaData->urlImage, psaData->textButton, psaData->urlClick);
        delete psaData;
    }
#endif

    adjustSize();

    mTransferScanCancelUi = new TransferScanCancelUi(ui->sTabs, ui->pTransfersTab);
    connect(mTransferScanCancelUi, &TransferScanCancelUi::cancelTransfers,
            this, &InfoDialog::cancelScanning);

    mResetTransferSummaryWidget.setInterval(2000);
    mResetTransferSummaryWidget.setSingleShot(true);
    connect(&mResetTransferSummaryWidget, &QTimer::timeout, this, &InfoDialog::onResetTransfersSummaryWidget);

    connect(MegaSyncApp->getStalledIssuesModel(), &StalledIssuesModel::stalledIssuesChanged,
            this,  &InfoDialog::onStalledIssuesChanged);
    onStalledIssuesChanged();

    connect(AccountDetailsManager::instance(),
            &AccountDetailsManager::accountDetailsUpdated,
            this,
            &InfoDialog::updateUsageAndAccountType);

    updateUpgradeButtonText();
}

InfoDialog::~InfoDialog()
{
    removeEventFilter(this);
    if(ui->tvNotifications->itemDelegate())
    {
        // Remove delegate cache before deleting the parent QTreeView widget
        delete ui->tvNotifications->itemDelegate();
    }
    delete ui;
    delete animation;
    delete filterMenu;
}

PSA_info *InfoDialog::getPSAdata()
{
    if (ui->wPSA->isPSAshown())
    {
        PSA_info* info = new PSA_info(ui->wPSA->getPSAdata());
        return info;
    }

    return nullptr;
}

void InfoDialog::showEvent(QShowEvent *event)
{
    emit ui->sTabs->currentChanged(ui->sTabs->currentIndex());
    if (ui->bTransferManager->alwaysAnimateOnShow || ui->bTransferManager->neverPainted )
    {
        ui->bTransferManager->showAnimated();
    }
    isShown = true;
    mTransferScanCancelUi->update();

    app->getNotificationController()->requestNotifications();

    repositionInfoDialog();
    QDialog::showEvent(event);
}

void InfoDialog::moveEvent(QMoveEvent*)
{
    qtBugFixer.onEndMove();
}

void InfoDialog::setBandwidthOverquotaState(QuotaState state)
{
    transferQuotaState = state;
    setUsage();
}

void InfoDialog::updateUsageAndAccountType()
{
    setUsage();
    setAccountType(mPreferences->accountType());
}

void InfoDialog::enableTransferOverquotaAlert()
{
    if (!transferOverquotaAlertEnabled)
    {
        transferOverquotaAlertEnabled = true;
        emit transferOverquotaMsgVisibilityChange(transferOverquotaAlertEnabled);
    }
    updateDialogState();
}

void InfoDialog::enableTransferAlmostOverquotaAlert()
{
    if (!transferAlmostOverquotaAlertEnabled)
    {
        transferAlmostOverquotaAlertEnabled = true;
        emit almostTransferOverquotaMsgVisibilityChange(transferAlmostOverquotaAlertEnabled);
    }
    updateDialogState();
}

void InfoDialog::hideEvent(QHideEvent *event)
{
    if (filterMenu && filterMenu->isVisible())
    {
        filterMenu->hide();
    }

    QTimer::singleShot(1000, this, [this] () {
        if (!isShown)
        {
            emit ui->sTabs->currentChanged(-1);
        }
    });


    isShown = false;
    if (ui->bTransferManager->alwaysAnimateOnShow || ui->bTransferManager->neverPainted )
    {
        ui->bTransferManager->shrink(true);
    }
    QDialog::hideEvent(event);

#ifdef _WIN32
    lastWindowHideTime = std::chrono::steady_clock::now();
#endif

}

void InfoDialog::setAvatar()
{
    ui->bAvatar->setUserEmail(mPreferences->email().toUtf8().constData());
}

void InfoDialog::setUsage()
{
    auto accType = mPreferences->accountType();

    QString quotaStringFormat = QString::fromLatin1("<span style='color:%1; font-size:%2px;'>%3</span>");
    // Get font to adapt size to widget if needed
    // Getting font from lUsedStorage considering both
    // lUsedStorage and lUsedTransfer use the same font.
    ui->lUsedStorage->style()->polish(ui->lUsedStorage);
    QFont font (ui->lUsedStorage->font());

    // ---------- Process storage usage

    // Get useful data
    auto totalStorage(mPreferences->totalStorage());
    auto usedStorage(mPreferences->usedStorage());

    QString usageColorS;
    QString usedStorageString = Utilities::getSizeString(usedStorage);
    QString totalStorageString;
    QString storageUsageStringFormatted (usedStorageString);

    if (Utilities::isBusinessAccount())
    {
        ui->sStorage->setCurrentWidget(ui->wBusinessStorage);
        ui->wCircularStorage->setState(CircularUsageProgressBar::STATE_OK);
        usageColorS = QString::fromLatin1("#333333");
        font.setPixelSize(FONT_SIZE_BUSINESS_PX);
    }
    else
    {
        int parts = 0;

        if (totalStorage != 0ULL)
        {
            switch (mPreferences->getStorageState())
            {
                case MegaApi::STORAGE_STATE_PAYWALL:
                // Fallthrough
                case MegaApi::STORAGE_STATE_RED:
                {
                    ui->wCircularStorage->setState(CircularUsageProgressBar::STATE_OVER);
                    usageColorS = QString::fromLatin1("#D90007");
                    break;
                }
                case MegaApi::STORAGE_STATE_ORANGE:
                {
                    ui->wCircularStorage->setState(CircularUsageProgressBar::STATE_WARNING);
                    usageColorS = QString::fromLatin1("#F98400");
                    break;
                }
                case MegaApi::STORAGE_STATE_UNKNOWN:
                // Fallthrough
                case MegaApi::STORAGE_STATE_GREEN:
                // Fallthrough
                default:
                {
                    ui->wCircularStorage->setState(CircularUsageProgressBar::STATE_OK);
                    usageColorS = QString::fromLatin1("#666666");
                    break;
                }
            }

            parts = usedStorage ?
                        std::max(Utilities::partPer(usedStorage, totalStorage),
                                 DEFAULT_MIN_PERCENTAGE)
                                : 0;

            totalStorageString = Utilities::getSizeString(totalStorage);

            storageUsageStringFormatted = Utilities::getTranslatedSeparatorTemplate().arg(
                usedStorageString,
                totalStorageString);
        }

        ui->wCircularStorage->setValue(parts);
        ui->sStorage->setCurrentWidget(ui->wCircularStorage);
        font.setPixelSize(FONT_SIZE_NO_BUSINESS_PX);
    }


    // ---------- Process transfer usage

    // Get useful data
    auto totalTransfer(mPreferences->totalBandwidth());
    auto usedTransfer(mPreferences->usedBandwidth());

    QString usageColorT;
    QString usedTransferString (Utilities::getSizeString(usedTransfer));
    QString totalTransferString;
    QString transferUsageStringFormatted (usedTransferString);

    if (Utilities::isBusinessAccount())
    {
        ui->sQuota->setCurrentWidget(ui->wBusinessQuota);
        usageColorT = QString::fromLatin1("#333333");
        ui->wCircularStorage->setTotalValueUnknown();
    }
    else
    {
        // Set color according to state
        switch (transferQuotaState)
        {
            case QuotaState::OK:
            {
                ui->wCircularQuota->setState(CircularUsageProgressBar::STATE_OK);
                usageColorT = QString::fromLatin1("#666666");
                break;
            }
            case QuotaState::WARNING:
            {
                ui->wCircularQuota->setState(CircularUsageProgressBar::STATE_WARNING);
                usageColorT = QString::fromLatin1("#F98400");
                break;
            }
            case QuotaState::OVERQUOTA:
            // Fallthrough
            case QuotaState::FULL:
            {
                ui->wCircularQuota->setState(CircularUsageProgressBar::STATE_OVER);
                usageColorT = QString::fromLatin1("#D90007");
                break;
            }
        }

        if (accType == Preferences::ACCOUNT_TYPE_FREE)
        {
            ui->wCircularQuota->setTotalValueUnknown(transferQuotaState != QuotaState::FULL
                                                        && transferQuotaState != QuotaState::OVERQUOTA);
        }
        else
        {
            int parts = 0;

            if (totalTransfer == 0ULL)
            {
                ui->wCircularQuota->setTotalValueUnknown();
            }
            else
            {
                parts = usedTransfer ?
                            std::max(Utilities::partPer(usedTransfer, totalTransfer),
                                     DEFAULT_MIN_PERCENTAGE)
                                     : 0;

                totalTransferString = Utilities::getSizeString(totalTransfer);

                transferUsageStringFormatted = Utilities::getTranslatedSeparatorTemplate().arg(
                    usedTransferString,
                    totalTransferString);
            }

            ui->wCircularQuota->setValue(parts);
        }

        ui->sQuota->setCurrentWidget(ui->wCircularQuota);
    }

    // Now compute the font size and set usage strings
    // Find correct font size so that the string does not overflow
    auto defaultColor = QString::fromLatin1("#999999");

    auto contentsMargins = ui->lUsedStorage->contentsMargins();
    auto margin = contentsMargins.left() + contentsMargins.right() + 2 * ui->lUsedStorage->margin();
    auto storageStringMaxWidth = ui->wStorageDetails->contentsRect().width() - margin;

    contentsMargins = ui->lUsedQuota->contentsMargins();
    margin = contentsMargins.left() + contentsMargins.right() + 2 * ui->lUsedQuota->margin();
    auto transferStringMaxWidth = ui->wQuotaDetails->contentsRect().width() - margin;

    QFontMetrics fMetrics (font);
    while ((fMetrics.horizontalAdvance(storageUsageStringFormatted) >= storageStringMaxWidth
            || fMetrics.horizontalAdvance(transferUsageStringFormatted) >= transferStringMaxWidth)
           && font.pixelSize() > 1)
    {
        font.setPixelSize(font.pixelSize() - 1);
        fMetrics = QFontMetrics(font);
    }

    // Now apply format (color, font size) to Storage usage string
    auto usedStorageStringFormatted = quotaStringFormat.arg(usageColorS,
                                                            QString::number(font.pixelSize()),
                                                            usedStorageString);
    if (totalStorage == 0ULL || Utilities::isBusinessAccount())
    {
        storageUsageStringFormatted = usedStorageStringFormatted;
    }
    else
    {
        storageUsageStringFormatted = Utilities::getTranslatedSeparatorTemplate().arg(
            usedStorageStringFormatted,
            totalStorageString);
        storageUsageStringFormatted = quotaStringFormat.arg(defaultColor,
                                                            QString::number(font.pixelSize()),
                                                            storageUsageStringFormatted);
    }

    ui->lUsedStorage->setText(storageUsageStringFormatted);

    // Now apply format (color, font size) to Transfer usage string
    auto usedTransferStringFormatted = quotaStringFormat.arg(usageColorT,
                                                             QString::number(font.pixelSize()),
                                                             usedTransferString);
    if (totalTransfer == 0ULL || Utilities::isBusinessAccount())
    {
        transferUsageStringFormatted = usedTransferStringFormatted;
    }
    else
    {
        transferUsageStringFormatted = Utilities::getTranslatedSeparatorTemplate().arg(
            usedTransferStringFormatted,
            totalTransferString);
        transferUsageStringFormatted = quotaStringFormat.arg(defaultColor,
                                                            QString::number(font.pixelSize()),
                                                            transferUsageStringFormatted);
    }

    ui->lUsedQuota->setText(transferUsageStringFormatted);
}

void InfoDialog::updateTransfersCount()
{
    if(app->getTransfersModel())
    {
        auto transfersCountUpdated = app->getTransfersModel()->getLastTransfersCount();

        ui->bTransferManager->setDownloads(transfersCountUpdated.completedDownloads(), transfersCountUpdated.totalDownloads);
        ui->bTransferManager->setUploads(transfersCountUpdated.completedUploads(), transfersCountUpdated.totalUploads);

        ui->bTransferManager->setPercentUploads(transfersCountUpdated.completedUploadBytes, transfersCountUpdated.totalUploadBytes);
        ui->bTransferManager->setPercentDownloads(transfersCountUpdated.completedDownloadBytes, transfersCountUpdated.totalDownloadBytes);
    }
}

void InfoDialog::onTransfersStateChanged()
{
    if(app->getTransfersModel())
    {
        auto transfersCountUpdated = app->getTransfersModel()->getLastTransfersCount();

        if(transfersCountUpdated.pendingTransfers() == 0)
        {
            if (!overQuotaState && (ui->sActiveTransfers->currentWidget() != ui->pUpdated))
            {
                updateDialogState();
            }

            mResetTransferSummaryWidget.start();
        }
        else
        {
            mResetTransferSummaryWidget.stop();
        }

        ui->wStatus->update();
    }
}

void InfoDialog::onStalledIssuesChanged()
{
    if (!MegaSyncApp->getStalledIssuesModel()->isEmpty())
    {
        showSomeIssues();
    }
    else
    {
        hideSomeIssues();
    }

    updateState();
}

void InfoDialog::onResetTransfersSummaryWidget()
{
    ui->bTransferManager->reset();
}

void InfoDialog::setIndexing(bool indexing)
{
    mIndexing = indexing;
}

void InfoDialog::setWaiting(bool waiting)
{
    mWaiting = waiting;
}

void InfoDialog::setSyncing(bool syncing)
{
    mSyncing = syncing;
}

void InfoDialog::setTransferring(bool transferring)
{
    mTransferring = transferring;
}

void InfoDialog::setOverQuotaMode(bool state)
{
    if (overQuotaState == state)
    {
        return;
    }

    overQuotaState = state;
    ui->wStatus->setOverQuotaState(state);
}

void InfoDialog::setAccountType(int accType)
{
    if (actualAccountType == accType)
    {
        return;
    }

    actualAccountType = accType;
    if (Utilities::isBusinessAccount())
    {
         ui->bUpgrade->hide();
    }
    else
    {
         ui->bUpgrade->show();
    }
}

void InfoDialog::updateBlockedState()
{
    if (!mPreferences->logged())
    {
        return;
    }
}

void InfoDialog::updateState()
{
    if (!mPreferences->logged())
    {
        return;
    }

    if(!checkFailedState())
    {
        if (mTransferScanCancelUi != nullptr && mTransferScanCancelUi->isActive())
        {
            changeStatusState(StatusInfo::TRANSFERS_STATES::STATE_INDEXING);
        }
        else if (mPreferences->getGlobalPaused())
        {
            mState = StatusInfo::TRANSFERS_STATES::STATE_PAUSED;
            animateStates(mWaiting || mIndexing || mSyncing);
        }
        else if (mIndexing)
        {
            changeStatusState(StatusInfo::TRANSFERS_STATES::STATE_INDEXING);
        }
        else if (mSyncing)
        {
            changeStatusState(StatusInfo::TRANSFERS_STATES::STATE_SYNCING);
        }
        else if (mWaiting)
        {
            changeStatusState(StatusInfo::TRANSFERS_STATES::STATE_WAITING);
        }
        else if (mTransferring)
        {
            changeStatusState(StatusInfo::TRANSFERS_STATES::STATE_TRANSFERRING);
        }
        else
        {
            changeStatusState(StatusInfo::TRANSFERS_STATES::STATE_UPDATED, false);
        }
    }

    if(ui->wStatus->getState() != mState)
    {
        ui->wStatus->setState(mState);
        if(mTransferManager)
        {
            mTransferManager->setTransferState(mState);
        }
    }
}

bool InfoDialog::checkFailedState()
{
    auto isFailed(false);

    if((app->getTransfersModel() && app->getTransfersModel()->failedTransfers()) 
    || (app->getStalledIssuesModel() && !app->getStalledIssuesModel()->isEmpty()))
    {
        changeStatusState(StatusInfo::TRANSFERS_STATES::STATE_FAILED);
        isFailed = true;
    }

    return isFailed;
}

void InfoDialog::onAddSync(mega::MegaSync::SyncType type)
{
    switch (type)
    {
        case mega::MegaSync::TYPE_TWOWAY:
        {
            MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::MENU_ADD_SYNC_CLICKED, true);
            addSync();
            break;
        }
        case mega::MegaSync::TYPE_BACKUP:
        {
            MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::MENU_ADD_BACKUP_CLICKED, true);
            addBackup();
            break;
        }
        default:
        {
            break;
        }
    }
}

void InfoDialog::onAddBackup()
{
    onAddSync(mega::MegaSync::TYPE_BACKUP);
}

void InfoDialog::updateDialogState()
{
    updateState();
    const bool transferOverQuotaEnabled{(transferQuotaState == QuotaState::FULL || transferQuotaState == QuotaState::OVERQUOTA)
                && transferOverquotaAlertEnabled};

    if (storageState == Preferences::STATE_PAYWALL)
    {
        MegaIntegerList* tsWarnings = megaApi->getOverquotaWarningsTs();
        const char *email = megaApi->getMyEmail();

        long long numFiles{mPreferences->cloudDriveFiles() + mPreferences->vaultFiles() + mPreferences->rubbishFiles()};
        QString contactMessage = tr("We have contacted you by email to [A] on [B] but you still have %n file taking up [D] in your MEGA account, which requires you to have [E].", "", static_cast<int>(numFiles));
        QString overDiskText = QString::fromUtf8("<p style='line-height: 20px;'>") + contactMessage
                .replace(QString::fromUtf8("[A]"), QString::fromUtf8(email))
                .replace(QString::fromUtf8("[B]"), Utilities::getReadableStringFromTs(tsWarnings))
                .replace(QString::fromUtf8("[D]"), Utilities::getSizeString(mPreferences->usedStorage()))
                .replace(QString::fromUtf8("[E]"), Utilities::minProPlanNeeded(MegaSyncApp->getPricing(), mPreferences->usedStorage()))
                + QString::fromUtf8("</p>");
        ui->lOverDiskQuotaLabel->setText(overDiskText);

        int64_t remainDaysOut(0);
        int64_t remainHoursOut(0);
        Utilities::getDaysAndHoursToTimestamp(megaApi->getOverquotaDeadlineTs() * 1000, remainDaysOut, remainHoursOut);
        if (remainDaysOut > 0)
        {
            QString descriptionDays = tr("You have [A]%n day[/A] left to upgrade. After that, your data is subject to deletion.", "", static_cast<int>(remainDaysOut));
            ui->lWarningOverDiskQuota->setText(QString::fromUtf8("<p style='line-height: 20px;'>") + descriptionDays
                    .replace(QString::fromUtf8("[A]"), QString::fromUtf8("<span style='color: #FF6F00;'>"))
                    .replace(QString::fromUtf8("[/A]"), QString::fromUtf8("</span>"))
                    + QString::fromUtf8("</p>"));
        }
        else if (remainDaysOut == 0 && remainHoursOut > 0)
        {
            QString descriptionHours = tr("You have [A]%n hour[/A] left to upgrade. After that, your data is subject to deletion.", "", static_cast<int>(remainHoursOut));
            ui->lWarningOverDiskQuota->setText(QString::fromUtf8("<p style='line-height: 20px;'>") + descriptionHours
                    .replace(QString::fromUtf8("[A]"), QString::fromUtf8("<span style='color: #FF6F00;'>"))
                    .replace(QString::fromUtf8("[/A]"), QString::fromUtf8("</span>"))
                    + QString::fromUtf8("</p>"));
        }
        else
        {
            ui->lWarningOverDiskQuota->setText(tr("You must act immediately to save your data"));
        }


        delete tsWarnings;
        delete [] email;

        ui->sActiveTransfers->setCurrentWidget(ui->pOverDiskQuotaPaywall);
        overlay->setVisible(false);
        ui->wPSA->hidePSA();
    }
    else if(storageState == Preferences::STATE_OVER_STORAGE)
    {
        const bool transferIsOverQuota{transferQuotaState == QuotaState::FULL || transferQuotaState == QuotaState::OVERQUOTA};
        const bool userIsFree{mPreferences->accountType() == Preferences::Preferences::ACCOUNT_TYPE_FREE};
        if(transferIsOverQuota && userIsFree)
        {
            ui->bOQIcon->setIcon(QIcon(QString::fromLatin1("://images/storage_transfer_full_FREE.png")));
            ui->bOQIcon->setIconSize(QSize(96,96));
        }
        else if(transferIsOverQuota && !userIsFree)
        {
            ui->bOQIcon->setIcon(QIcon(QString::fromLatin1("://images/storage_transfer_full_PRO.png")));
            ui->bOQIcon->setIconSize(QSize(96,96));
        }
        else
        {
            ui->bOQIcon->setIcon(QIcon(QString::fromLatin1("://images/storage_full.png")));
            ui->bOQIcon->setIconSize(QSize(64,64));
        }
        ui->lOQTitle->setText(tr("Your MEGA account is full."));
        ui->lOQDesc->setText(tr("All file uploads are currently disabled.")
                                + QString::fromUtf8("<br>")
                                + tr("Please upgrade to PRO."));
        ui->bBuyQuota->setText(tr("Buy more space"));
        ui->sActiveTransfers->setCurrentWidget(ui->pOverquota);
        overlay->setVisible(false);
        ui->wPSA->hidePSA();
    }
    else if(transferOverQuotaEnabled)
    {
        ui->lOQTitle->setText(tr("Transfer quota exceeded"));

        if(mPreferences->accountType() == Preferences::ACCOUNT_TYPE_FREE)
        {
            ui->lOQDesc->setText(tr("Your queued transfers exceed the current quota available for your IP address."));
            ui->bBuyQuota->setText(tr("Upgrade Account"));
            ui->bDiscard->setText(tr("I will wait"));
        }
        else
        {

            ui->lOQDesc->setText(tr("You can't continue downloading as you don't have enough transfer quota left on this account. "
                                    "To continue downloading, purchase a new plan, or if you have a recurring subscription with MEGA, "
                                    "you can wait for your plan to renew."));
            ui->bBuyQuota->setText(tr("Buy new plan"));
            ui->bDiscard->setText(tr("Dismiss"));
        }
        ui->bOQIcon->setIcon(QIcon(QString::fromLatin1(":/images/transfer_empty_64.png")));
        ui->bOQIcon->setIconSize(QSize(64,64));
        ui->sActiveTransfers->setCurrentWidget(ui->pOverquota);
        overlay->setVisible(false);
        ui->wPSA->hidePSA();
    }
    else if(storageState == Preferences::STATE_ALMOST_OVER_STORAGE)
    {
        ui->bOQIcon->setIcon(QIcon(QString::fromLatin1("://images/storage_almost_full.png")));
        ui->bOQIcon->setIconSize(QSize(64,64));
        ui->lOQTitle->setText(tr("You're running out of storage space."));
        ui->lOQDesc->setText(tr("Upgrade to PRO now before your account runs full and your uploads to MEGA stop."));
        ui->bBuyQuota->setText(tr("Buy more space"));
        ui->sActiveTransfers->setCurrentWidget(ui->pOverquota);
        overlay->setVisible(false);
        ui->wPSA->hidePSA();
    }
    else if(transferQuotaState == QuotaState::WARNING &&
            transferAlmostOverquotaAlertEnabled)
    {
        ui->bOQIcon->setIcon(QIcon(QString::fromLatin1(":/images/transfer_empty_64.png")));
        ui->bOQIcon->setIconSize(QSize(64,64));
        ui->lOQTitle->setText(tr("Limited available transfer quota"));
        ui->lOQDesc->setText(tr("Downloading may be interrupted as you have used 90% of your transfer quota on this "
                                "account. To continue downloading, purchase a new plan, or if you have a recurring "
                                "subscription with MEGA, you can wait for your plan to renew. "));
        ui->bBuyQuota->setText(tr("Buy new plan"));

        ui->sActiveTransfers->setCurrentWidget(ui->pOverquota);
        overlay->setVisible(false);
        ui->wPSA->hidePSA();
    }
    else if (mSyncInfo->hasUnattendedDisabledSyncs({mega::MegaSync::TYPE_TWOWAY, mega::MegaSync::TYPE_BACKUP}))
    {
        if (mSyncInfo->hasUnattendedDisabledSyncs(mega::MegaSync::TYPE_TWOWAY)
            && mSyncInfo->hasUnattendedDisabledSyncs(mega::MegaSync::TYPE_BACKUP))
        {
            ui->sActiveTransfers->setCurrentWidget(ui->pAllSyncsDisabled);
        }
        else if (mSyncInfo->hasUnattendedDisabledSyncs(mega::MegaSync::TYPE_BACKUP))
        {
            ui->sActiveTransfers->setCurrentWidget(ui->pBackupsDisabled);
        }
        else
        {
            ui->sActiveTransfers->setCurrentWidget(ui->pSyncsDisabled);
        }
        overlay->setVisible(false);
        ui->wPSA->hidePSA();
    }
    else
    {
        if(app->getTransfersModel())
        {
            auto transfersCount = app->getTransfersModel()->getTransfersCount();

            if (transfersCount.totalDownloads || transfersCount.totalUploads
                    || ui->wPSA->isPSAready())
            {
                overlay->setVisible(false);
                ui->sActiveTransfers->setCurrentWidget(ui->pTransfers);
                ui->wPSA->showPSA();
            }
            else
            {
                ui->wPSA->hidePSA();
                ui->sActiveTransfers->setCurrentWidget(ui->pUpdated);
                if (!mWaiting && !mIndexing)
                {
                    overlay->setVisible(true);
                }
                else
                {
                    overlay->setVisible(false);
                }
            }
        }
    }
    updateBlockedState();
}

void InfoDialog::on_bSettings_clicked()
{
    emit userActivity();

    QPoint p = ui->bSettings->mapToGlobal(QPoint(ui->bSettings->width() - 2, ui->bSettings->height()));

#ifdef __APPLE__
    QPointer<InfoDialog> iod = this;
#endif

    app->showTrayMenu(&p);

#ifdef __APPLE__
    if (!iod)
    {
        return;
    }

    if (!this->rect().contains(this->mapFromGlobal(QCursor::pos())))
    {
        this->hide();
    }
#endif

    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::MENU_CLICKED, true);
}

void InfoDialog::on_bUpgrade_clicked()
{
    Utilities::upgradeClicked();
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::UPGRADE_ACCOUNT_CLICKED, true);
}

void InfoDialog::on_bUpgradeOverDiskQuota_clicked()
{
    on_bUpgrade_clicked();
}

void InfoDialog::openFolder(QString path)
{
    Utilities::openUrl(QUrl::fromLocalFile(path));
}

void InfoDialog::addSync(mega::MegaHandle handle)
{
    CreateRemoveSyncsManager::addSync(handle);
}

void InfoDialog::addBackup()
{
    auto manager = CreateRemoveBackupsManager::addBackup(false);
    if(manager->isBackupsDialogOpen())
    {
        hide();
    }
}

void InfoDialog::onOverlayClicked()
{
    app->uploadActionClicked();
}

void InfoDialog::on_bTransferManager_clicked()
{
    emit userActivity();
    app->transferManagerActionClicked();
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::OPEN_TRANSFER_MANAGER_CLICKED, true);
}

void InfoDialog::on_bAddSync_clicked()
{
    showSyncsMenu(ui->bAddSync, MegaSync::TYPE_TWOWAY);
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::ADD_SYNC_CLICKED, true);
}

void InfoDialog::on_bAddBackup_clicked()
{
    showSyncsMenu(ui->bAddBackup, MegaSync::TYPE_BACKUP);
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::ADD_BACKUP_CLICKED, true);
}

void InfoDialog::showSyncsMenu(QPushButton* b, mega::MegaSync::SyncType type)
{
    if (mPreferences->logged())
    {
        auto* menu (mSyncsMenus.value(b, nullptr));
        if (!menu)
        {
            menu = initSyncsMenu(type, ui->bUpload->isEnabled());
            mSyncsMenus.insert(b, menu);
        }
        if (menu) menu->callMenu(b->mapToGlobal(QPoint(b->width() - 100, b->height() + 3)));
    }
}

SyncsMenu* InfoDialog::initSyncsMenu(mega::MegaSync::SyncType type, bool isEnabled)
{
    SyncsMenu* menu (SyncsMenu::newSyncsMenu(type, isEnabled, this));
    connect(menu, &SyncsMenu::addSync, this, &InfoDialog::onAddSync);
    return menu;
}

void InfoDialog::on_bUpload_clicked()
{
    app->uploadActionClicked();
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::UPLOAD_CLICKED, true);
}

void InfoDialog::clearUserAttributes()
{
    ui->bAvatar->clearData();
}

bool InfoDialog::updateOverStorageState(int state)
{
    if (storageState != state)
    {
        storageState = state;
        updateDialogState();
        return true;
    }
    return false;
}

void InfoDialog::onUnseenAlertsChanged(const UnseenUserMessagesMap& alerts)
{
    setUnseenNotifications(alerts[MessageType::ALL]);
    filterMenu->setUnseenNotifications(alerts[MessageType::ALL],
                                       alerts[MessageType::ALERT_CONTACTS],
                                       alerts[MessageType::ALERT_SHARES],
                                       alerts[MessageType::ALERT_PAYMENTS]);
    ui->wSortNotifications->resetAllFilterHasBeenSelected();
}

void InfoDialog::reset()
{
    notificationsReady = false;
    ui->sNotifications->setCurrentWidget(ui->pNoNotifications);
    ui->wSortNotifications->setActualFilter(MessageType::ALL);

    ui->bTransferManager->reset();

    hideSomeIssues();

    setUnseenNotifications(0);
    if (filterMenu)
    {
        filterMenu->reset();
    }

    transferOverquotaAlertEnabled = false;
    transferAlmostOverquotaAlertEnabled = false;
    transferQuotaState = QuotaState::OK;
}

void InfoDialog::setPSAannouncement(int id, QString title, QString text, QString urlImage, QString textButton, QString linkButton)
{
    ui->wPSA->setAnnounce(id, title, text, urlImage, textButton, linkButton);
}

void InfoDialog::enterBlockingState()
{
    enableUserActions(false);
    ui->bTransferManager->setPauseEnabled(false);
    ui->wTabOptions->setVisible(false);
    mTransferScanCancelUi->show();
    updateState();
}

void InfoDialog::leaveBlockingState(bool fromCancellation)
{
    enableUserActions(true);
    ui->bTransferManager->setPauseEnabled(true);
    ui->wTabOptions->setVisible(true);
    mTransferScanCancelUi->hide(fromCancellation);
    updateState();
}

void InfoDialog::disableCancelling()
{
    mTransferScanCancelUi->disableCancelling();
}

void InfoDialog::setUiInCancellingStage()
{
    mTransferScanCancelUi->setInCancellingStage();
}

void InfoDialog::updateUiOnFolderTransferUpdate(const FolderTransferUpdateEvent &event)
{
    mTransferScanCancelUi->onFolderTransferUpdate(event);
}

void InfoDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        updateUpgradeButtonText();
        // if (mPreferences->logged())
        // {
        //     setUsage();
        //     mState = StatusInfo::TRANSFERS_STATES::STATE_STARTING;
        //     updateDialogState();
        // }
    }
    QDialog::changeEvent(event);
}

bool InfoDialog::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == ui->wStorageUsage && e->type() == QEvent::MouseButtonPress)
    {
        on_bStorageDetails_clicked();
        return true;
    }

#ifdef Q_OS_LINUX
    static bool firstime = true;
    if (qEnvironmentVariableIsSet("START_MEGASYNC_MINIMIZED") && firstime &&
        (obj == this && e->type() == QEvent::Paint))
    {
        MegaApi::log(MegaApi::LOG_LEVEL_DEBUG, QString::fromUtf8("Minimizing info dialog (reason: %1)...").arg(e->type()).toUtf8().constData());
        showMinimized();
        firstime = false;
    }

    if (doNotActAsPopup)
    {
        if (obj == this && e->type() == QEvent::Close)
        {
            e->ignore(); //This prevents the dialog from closing
            app->tryExitApplication();
            return true;
        }
    }
    else if (obj == this)
    {
        if (e->type() == QEvent::WindowDeactivate)
        {
            hide();
            return true;
        }
        else if(e->type() == QEvent::FocusOut)
        {
            hide();
            return true;
        }
    }

#endif
#ifdef __APPLE__
    auto current = QOperatingSystemVersion::current();
    if (current <= QOperatingSystemVersion::OSXMavericks) //manage spontaneus mouse press events
    {
        if (obj == this && e->type() == QEvent::MouseButtonPress && e->spontaneous())
        {
            return true;
        }
    }
#endif

    return QDialog::eventFilter(obj, e);
}

void InfoDialog::on_bStorageDetails_clicked()
{
    auto dialog = new AccountDetailsDialog();
    DialogOpener::showNonModalDialog<AccountDetailsDialog>(dialog);
}

void InfoDialog::animateStates(bool opt)
{
    if (opt) //Enable animation for scanning/waiting states
    {
        ui->lUploadToMega->setIcon(Utilities::getCachedPixmap(QString::fromUtf8("://images/init_scanning.png")));
        ui->lUploadToMega->setIconSize(QSize(352,234));
        ui->lUploadToMegaDesc->setStyleSheet(QString::fromUtf8("font-size: 14px;"));

        if (!opacityEffect)
        {
            opacityEffect = new QGraphicsOpacityEffect();
            ui->lUploadToMega->setGraphicsEffect(opacityEffect);
        }

        if (!animation)
        {
            animation = new QPropertyAnimation(opacityEffect, "opacity");
            animation->setDuration(2000);
            animation->setStartValue(1.0);
            animation->setEndValue(0.5);
            animation->setEasingCurve(QEasingCurve::InOutQuad);
            connect(animation, SIGNAL(finished()), SLOT(onAnimationFinished()));
        }

        if (animation->state() != QAbstractAnimation::Running)
        {
            animation->start();
        }
    }
    else //Disable animation
    {
        ui->lUploadToMega->setIcon(Utilities::getCachedPixmap(QString::fromUtf8("://images/upload_to_mega.png")));
        ui->lUploadToMega->setIconSize(QSize(352,234));
        ui->lUploadToMegaDesc->setStyleSheet(QString::fromUtf8("font-size: 18px;"));
        ui->lUploadToMegaDesc->setText(tr("Upload to MEGA now"));

        if (animation)
        {
            if (opacityEffect) //Reset opacity
            {
                opacityEffect->setOpacity(1.0);
            }

            if (animation->state() == QAbstractAnimation::Running)
            {
                animation->stop();
            }
        }
    }
}

void InfoDialog::resetLoggedInMode()
{
    loggedInMode = STATE_NONE;
}

void InfoDialog::on_tTransfers_clicked()
{
    ui->lTransfers->setStyleSheet(QString::fromUtf8("background-color: #3C434D;"));
    ui->lRecents->setStyleSheet(QString::fromUtf8("background-color : transparent;"));

    ui->tTransfers->setStyleSheet(QString::fromUtf8("color : #1D1D1D;"));
    ui->tNotifications->setStyleSheet(QString::fromUtf8("color : #989899;"));

    ui->sTabs->setCurrentWidget(ui->pTransfersTab);

    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::TRANSFER_TAB_CLICKED,
                                                          sender(), ui->tTransfers, true);
}

void InfoDialog::on_tNotifications_clicked()
{
    app->getNotificationController()->requestNotifications();

    ui->lTransfers->setStyleSheet(QString::fromUtf8("background-color : transparent;"));
    ui->lRecents->setStyleSheet(QString::fromUtf8("background-color: #3C434D;"));

    ui->tNotifications->setStyleSheet(QString::fromUtf8("color : #1D1D1D;"));
    ui->tTransfers->setStyleSheet(QString::fromUtf8("color : #989899;"));

    ui->sTabs->setCurrentWidget(ui->pNotificationsTab);

    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::NOTIFICATION_TAB_CLICKED, true);
}

void InfoDialog::onActualFilterClicked()
{
    if (!notificationsReady || !filterMenu)
    {
        return;
    }

    QPoint p = ui->wFilterAndSettings->mapToGlobal(QPoint(4, 4));
    filterMenu->move(p);
    filterMenu->show();
}

void InfoDialog::applyFilterOption(MessageType opt)
{
    if (filterMenu && filterMenu->isVisible())
    {
        filterMenu->hide();
    }

    switch (opt)
    {
        case MessageType::ALERT_CONTACTS:
        {
            ui->wSortNotifications->setActualFilter(opt);

            if (app->getNotificationController()->hasElementsOfType(MessageType::ALERT_CONTACTS))
            {
                ui->sNotifications->setCurrentWidget(ui->pNotifications);
            }
            else
            {
                ui->lNoNotifications->setText(tr("No notifications for contacts"));
                ui->sNotifications->setCurrentWidget(ui->pNoNotifications);
            }

            break;
        }
        case MessageType::ALERT_SHARES:
        {
            ui->wSortNotifications->setActualFilter(opt);

            if (app->getNotificationController()->hasElementsOfType(MessageType::ALERT_SHARES))
            {
                ui->sNotifications->setCurrentWidget(ui->pNotifications);
            }
            else
            {
                ui->lNoNotifications->setText(tr("No notifications for incoming shares"));
                ui->sNotifications->setCurrentWidget(ui->pNoNotifications);
            }

            break;
        }
        case MessageType::ALERT_PAYMENTS:
        {
            ui->wSortNotifications->setActualFilter(opt);

            if (app->getNotificationController()->hasElementsOfType(MessageType::ALERT_PAYMENTS))
            {
                ui->sNotifications->setCurrentWidget(ui->pNotifications);
            }
            else
            {
                ui->lNoNotifications->setText(tr("No notifications for payments"));
                ui->sNotifications->setCurrentWidget(ui->pNoNotifications);
            }
            break;
        }
        case MessageType::ALL:
        case MessageType::ALERT_TAKEDOWNS:
        default:
        {
            ui->wSortNotifications->setActualFilter(opt);

            if (app->getNotificationController()->hasNotifications())
            {
                ui->sNotifications->setCurrentWidget(ui->pNotifications);
            }
            else
            {
                ui->lNoNotifications->setText(tr("No notifications"));
                ui->sNotifications->setCurrentWidget(ui->pNoNotifications);
            }
            break;
        }
    }

    app->getNotificationController()->applyFilter(opt);
}

void InfoDialog::on_bNotificationsSettings_clicked()
{
    Utilities::openUrl(QUrl(QString::fromUtf8("mega://#fm/account/notifications")));
    MegaSyncApp->getStatsEventHandler()->sendTrackedEvent(AppStatsEvents::EventType::NOTIFICATION_SETTINGS_CLICKED, true);
}

void InfoDialog::on_bDiscard_clicked()
{
    if(transferQuotaState == QuotaState::FULL || transferQuotaState == QuotaState::OVERQUOTA)
    {
        transferOverquotaAlertEnabled = false;
        emit transferOverquotaMsgVisibilityChange(transferOverquotaAlertEnabled);
    }
    else if(transferQuotaState == QuotaState::WARNING)
    {
        transferAlmostOverquotaAlertEnabled = false;
        emit almostTransferOverquotaMsgVisibilityChange(transferAlmostOverquotaAlertEnabled);
    }

    if(storageState == Preferences::STATE_ALMOST_OVER_STORAGE ||
            storageState == Preferences::STATE_OVER_STORAGE)
    {
        updateOverStorageState(Preferences::STATE_OVER_STORAGE_DISMISSED);
        emit dismissStorageOverquota(overQuotaState);
    }
    else
    {
        updateDialogState();
    }
}

void InfoDialog::on_bBuyQuota_clicked()
{
    on_bUpgrade_clicked();
}

void InfoDialog::onAnimationFinished()
{
    if (animation->direction() == QAbstractAnimation::Forward)
    {
        animation->setDirection(QAbstractAnimation::Backward);
        animation->start();
    }
    else
    {
        animation->setDirection(QAbstractAnimation::Forward);
        animation->start();
    }
}

void InfoDialog::sTabsChanged(int tab)
{
    static int lastTab = -1;
    if (tab != ui->sTabs->indexOf(ui->pNotificationsTab)
            && lastTab == ui->sTabs->indexOf(ui->pNotificationsTab)
            && ui->wSortNotifications->allFilterHasBeenSelected())
    {
        app->getNotificationController()->ackSeenUserMessages();
        ui->wSortNotifications->resetAllFilterHasBeenSelected();
    }
    lastTab = tab;
}

void InfoDialog::hideSomeIssues()
{
    mShownSomeIssuesOccurred = false;
    ui->wSomeIssuesOccurred->hide();
}

void InfoDialog::showSomeIssues()
{
    if (mShownSomeIssuesOccurred)
    {
        return;
    }

    ui->wSomeIssuesOccurred->show();
    animationGroupSomeIssues.start();
    mShownSomeIssuesOccurred = true;
}

void InfoDialog::updateUpgradeButtonText()
{
    ui->bUpgrade->setText(QCoreApplication::translate("SettingsDialog", "Upgrade"));
}

void InfoDialog::on_bDismissSyncSettings_clicked()
{
    mSyncInfo->dismissUnattendedDisabledSyncs(mega::MegaSync::TYPE_TWOWAY);
}

void InfoDialog::on_bOpenSyncSettings_clicked()
{
    MegaSyncApp->openSettings(SettingsDialog::SYNCS_TAB);
    mSyncInfo->dismissUnattendedDisabledSyncs(mega::MegaSync::TYPE_TWOWAY);
}

void InfoDialog::on_bDismissBackupsSettings_clicked()
{
    mSyncInfo->dismissUnattendedDisabledSyncs(mega::MegaSync::TYPE_BACKUP);
}

void InfoDialog::on_bOpenBackupsSettings_clicked()
{
    MegaSyncApp->openSettings(SettingsDialog::BACKUP_TAB);
    mSyncInfo->dismissUnattendedDisabledSyncs(mega::MegaSync::TYPE_BACKUP);
}

void InfoDialog::on_bDismissAllSyncsSettings_clicked()
{
    mSyncInfo->dismissUnattendedDisabledSyncs(SyncInfo::AllHandledSyncTypes);
}

void InfoDialog::on_bOpenAllSyncsSettings_clicked()
{
    MegaSyncApp->openSettings(SettingsDialog::SYNCS_TAB);
    mSyncInfo->dismissUnattendedDisabledSyncs(SyncInfo::AllHandledSyncTypes);
}

int InfoDialog::getLoggedInMode() const
{
    return loggedInMode;
}

void InfoDialog::showNotifications()
{
    on_tNotifications_clicked();
}

void InfoDialog::move(int x, int y)
{
   qtBugFixer.onStartMove();
   QDialog::move(x, y);
}

void InfoDialog::setUnseenNotifications(long long value)
{
    assert(value >= 0);

    if (value > 0)
    {
        ui->bNumberUnseenNotifications->setText(QString::number(value));
        ui->bNumberUnseenNotifications->show();
    }
    else
    {
        ui->bNumberUnseenNotifications->hide();
    }
}

double InfoDialog::computeRatio(long long completed, long long remaining)
{
    return static_cast<double>(completed) / static_cast<double>(remaining);
}

void InfoDialog::enableUserActions(bool newState)
{
    ui->bAvatar->setEnabled(newState);
    ui->bUpgrade->setEnabled(newState);
    ui->bUpload->setEnabled(newState);

    // To set the state of the Syncs and Backups button,
    // we have to first create them if they don't exist
    auto buttonIt (mSyncsMenus.begin());
    while (buttonIt != mSyncsMenus.end())
    {
        auto* syncMenu (buttonIt.value());
        if (!syncMenu)
        {
            auto type (buttonIt.key() == ui->bAddSync ? MegaSync::TYPE_TWOWAY : MegaSync::TYPE_BACKUP);
            syncMenu = initSyncsMenu(type, newState);
            *buttonIt = syncMenu;
        }
        if (syncMenu)
        {
            syncMenu->setEnabled(newState);
            buttonIt.key()->setEnabled(syncMenu->getAction()->isEnabled());
        }

        *buttonIt++;
    }
}

void InfoDialog::changeStatusState(StatusInfo::TRANSFERS_STATES newState,
                                   bool animate)
{
    if (mState != newState)
    {
        mState = newState;
        animateStates(animate);
    }
}

void InfoDialog::setTransferManager(TransferManager *transferManager)
{
    mTransferManager = transferManager;
    mTransferManager->setTransferState(mState);
}

void InfoDialog::fixMultiscreenResizeBug(int& posX, int& posY)
{
    // An issue occurred with certain multiscreen setup that caused Qt to missplace the info dialog.
    // This works around that by ensuring infoDialog does not get incorrectly resized. in which case,
    // it is reverted to the correct size.

    ensurePolished();
    auto initialDialogWidth  = width();
    auto initialDialogHeight = height();
    QTimer::singleShot(1, this, [this, initialDialogWidth, initialDialogHeight, posX, posY](){
        if (width() > initialDialogWidth || height() > initialDialogHeight) //miss scaling detected
        {
            MegaApi::log(MegaApi::LOG_LEVEL_ERROR,
                         QString::fromUtf8("A dialog. New size = %1,%2. should be %3,%4 ")
                         .arg(width()).arg(height()).arg(initialDialogWidth).arg(initialDialogHeight)
                         .toUtf8().constData());

            resize(initialDialogWidth,initialDialogHeight);

            auto iDPos = pos();
            if (iDPos.x() != posX || iDPos.y() != posY )
            {
                MegaApi::log(MegaApi::LOG_LEVEL_ERROR,
                             QString::fromUtf8("Missplaced info dialog. New pos = %1,%2. should be %3,%4 ")
                             .arg(iDPos.x()).arg(iDPos.y()).arg(posX).arg(posY)
                             .toUtf8().constData());
                move(posX, posY);

                QTimer::singleShot(1, this, [this, initialDialogWidth, initialDialogHeight](){
                    if (width() > initialDialogWidth || height() > initialDialogHeight) //miss scaling detected
                    {
                        MegaApi::log(MegaApi::LOG_LEVEL_ERROR,
                                     QString::fromUtf8("Missscaled info dialog after second move. New size = %1,%2. should be %3,%4 ")
                                     .arg(width()).arg(height()).arg(initialDialogWidth).arg(initialDialogHeight)
                                     .toUtf8().constData());

                        resize(initialDialogWidth,initialDialogHeight);
                    }
                });
            }
        }
    });
}

void InfoDialog::repositionInfoDialog()
{
    int posx, posy;
    Platform::getInstance()->calculateInfoDialogCoordinates(rect(), &posx, &posy);

    fixMultiscreenResizeBug(posx, posy);

    MegaApi::log(MegaApi::LOG_LEVEL_DEBUG, QString::fromUtf8("Moving Info Dialog to posx = %1, posy = %2")
                 .arg(posx)
                 .arg(posy)
                 .toUtf8().constData());

    if(posx != this->x() || posy != this->y())
    {
        move(posx, posy);
    }
}

void InfoDialog::initNotificationArea()
{
    mNotificationsViewHoverManager.setView(ui->tvNotifications);

    ui->tvNotifications->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->tvNotifications->verticalScrollBar()->setSingleStep(12);
    ui->tvNotifications->setModel(app->getNotificationController()->getModel());
    ui->tvNotifications->sortByColumn(0, Qt::AscendingOrder);
    auto delegate = new UserMessageDelegate(app->getNotificationController()->getModel(),
                                            ui->tvNotifications);
    ui->tvNotifications->setItemDelegate(delegate);

    applyFilterOption(MessageType::ALL);
    connect(app->getNotificationController(), &UserMessageController::userMessagesReceived, this, [this]()
    {
        // We need to check if there is any user message to display or not
        // with the actual selected filter.
        applyFilterOption(filterMenu->getCurrentFilter());
    });

    notificationsReady = true;
}
