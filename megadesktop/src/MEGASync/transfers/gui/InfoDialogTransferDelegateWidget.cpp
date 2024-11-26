#include "InfoDialogTransferDelegateWidget.h"
#include "ui_InfoDialogTransferDelegateWidget.h"
#include "MegaApplication.h"
#include "Utilities.h"
#include "Platform.h"
#include <TransferItem.h>

#include <QImageReader>
#include <QtConcurrent/QtConcurrent>

using namespace mega;

const QRect InfoDialogTransferDelegateWidget::FullRect = QRect(0,0,400,60);

InfoDialogTransferDelegateWidget::InfoDialogTransferDelegateWidget(QWidget *parent) :
    TransferBaseDelegateWidget(parent),
    mUi(new Ui::InfoDialogTransferDelegateWidget),
    mMegaApi(MegaSyncApp->getMegaApi())
{
    mUi->setupUi(this);

    QSizePolicy retainShowInFolder = mUi->lShowInFolder->sizePolicy();
    retainShowInFolder.setRetainSizeWhenHidden(true);
    mUi->lShowInFolder->setSizePolicy(retainShowInFolder);

    mUi->bClockDown->setVisible(false);
    mUi->lShowInFolder->hide();

    mUi->lTransferType->installEventFilter(this);

    mUi->lFileNameCompleted->installEventFilter(this);
    mUi->lFileName->installEventFilter(this);
    mUi->lElapsedTime->installEventFilter(this);
}

InfoDialogTransferDelegateWidget::~InfoDialogTransferDelegateWidget()
{
    delete mUi;
}

void InfoDialogTransferDelegateWidget::updateTransferState()
{
    if(stateHasChanged())
    {
        if (getData()->getState() & (TransferData::TransferState::TRANSFER_COMPLETED
                                  | TransferData::TransferState::TRANSFER_FAILED))
        {
            mUi->sTransferState->setCurrentWidget(mUi->completedTransfer);
        }
        else
        {
            mUi->sTransferState->setCurrentWidget(mUi->activeTransfer);
        }
    }

    switch (getData()->getState())
    {
        case TransferData::TransferState::TRANSFER_COMPLETED:
        case TransferData::TransferState::TRANSFER_FAILED:
            if(stateHasChanged())
            {
                finishTransfer();
            }

            updateFinishedTime();
            break;
        case TransferData::TransferState::TRANSFER_ACTIVE:
        {
            mUi->bClockDown->setVisible(getData()->mRemainingTime > 0);
            mUi->lRemainingTime->setText(Utilities::getTimeString(getData()->mRemainingTime));

            // Update current transfer speed
            QString downloadString;

            if (!getData()->mTransferredBytes)
            {
                downloadString = getState(TRANSFER_STATES::STATE_STARTING);
            }
            else
            {
                QString pattern(QString::fromUtf8("%1/s"));
                downloadString = pattern.arg(Utilities::getSizeString(getData()->mSpeed));
            }

            mUi->lSpeed->setText(downloadString);
            break;
        }
        case TransferData::TransferState::TRANSFER_PAUSED:
        {
            if(getData()->mTransferredBytes != 0)
            {
                updateTransferControlsOnHold(getState(TRANSFER_STATES::STATE_PAUSED));
            }
            else
            {
                QString pausedInQueue(QString::fromLatin1("%1 %2").arg(getState(TRANSFER_STATES::STATE_PAUSED),getState(TRANSFER_STATES::STATE_INQUEUE_PARENTHESIS)));
                updateTransferControlsOnHold(pausedInQueue);
            }
            break;
        }
        case TransferData::TransferState::TRANSFER_QUEUED:
            updateTransferControlsOnHold(getState(TRANSFER_STATES::STATE_INQUEUE));
            break;
        case TransferData::TransferState::TRANSFER_RETRYING:
            if (getData()->mErrorCode == MegaError::API_EOVERQUOTA)
            {
                if (getData()->mErrorValue)
                {
                    updateTransferControlsOnHold(getState(TRANSFER_STATES::STATE_OUT_OF_TRANSFER_QUOTA));
                }
                else
                {
                    updateTransferControlsOnHold(getState(TRANSFER_STATES::STATE_OUT_OF_STORAGE_SPACE));
                }
            }
            else
            {
                updateTransferControlsOnHold(getState(TRANSFER_STATES::STATE_RETRYING));
            }
            break;
        case TransferData::TransferState::TRANSFER_COMPLETING:
            updateTransferControlsOnHold(getState(TRANSFER_STATES::STATE_COMPLETING));
            break;
        default:
            updateTransferControlsOnHold(QString());
            break;
    }

    // Update progress bar
    int permil = static_cast<int>((getData()->mTotalSize > 0) ? ((1000 * getData()->mTransferredBytes) / getData()->mTotalSize) : 0);
    mUi->pbTransfer->setValue(permil);
}

void InfoDialogTransferDelegateWidget::updateTransferControlsOnHold(const QString& speedText)
{
    if (stateHasChanged())
    {
        mUi->lSpeed->setText(speedText);
        mUi->bClockDown->setVisible(false);
        mUi->lRemainingTime->clear();
    }
}

void InfoDialogTransferDelegateWidget::setFileNameAndType()
{
    mUi->lFileName->ensurePolished();
    mUi->lFileName->setText(getData()->mFilename);
    mUi->lFileName->setToolTip(getData()->mFilename.toHtmlEscaped());
    mUi->lFileName->adjustSize();

    mUi->lFileNameCompleted->ensurePolished();
    mUi->lFileNameCompleted->setText(getData()->mFilename);
    mUi->lFileNameCompleted->setToolTip(getData()->mFilename.toHtmlEscaped());
    mUi->lFileNameCompleted->adjustSize();

    QIcon icon = Utilities::getExtensionPixmapMedium(getData()->mFilename);
    mUi->lFileType->setIcon(icon);
    mUi->lFileType->setIconSize(QSize(48, 48));
    mUi->lFileTypeCompleted->setIcon(icon);
    mUi->lFileTypeCompleted->setIconSize(QSize(48, 48));
}

void InfoDialogTransferDelegateWidget::setType()
{
    QIcon icon;

    auto transferType = getData()->mType;

    if(transferType & TransferData::TRANSFER_DOWNLOAD || transferType & TransferData::TRANSFER_LTCPDOWNLOAD)
    {
        icon = Utilities::getCachedPixmap(QString::fromLatin1(":/images/transfer_manager/transfers_states/download_item_ico.png"));
        mUi->pbTransfer->setStyleSheet(QString::fromLatin1("QProgressBar#pbTransfer{background-color: transparent;}"
                                                        "QProgressBar#pbTransfer::chunk {background-color: %1;}").arg(DOWNLOAD_TRANSFER_COLOR.name()));
    }
    else if(transferType & TransferData::TRANSFER_UPLOAD)
    {
        icon = Utilities::getCachedPixmap(QString::fromLatin1(":/images/transfer_manager/transfers_states/upload_item_ico.png"));
        mUi->pbTransfer->setStyleSheet(QString::fromLatin1("QProgressBar#pbTransfer{background-color: transparent;}"
                                                        "QProgressBar#pbTransfer::chunk {background-color: %1;}").arg(UPLOAD_TRANSFER_COLOR.name()));
    }

    mUi->lTransferType->setPixmap(icon.pixmap(mUi->lTransferType->size()));

    mUi->lSyncIcon->setVisible(getData()->isSyncTransfer());

    if(getData()->isSyncTransfer())
    {
        auto sync_icon = Utilities::getCachedPixmap(QString::fromLatin1(":/images/transfer_manager/transfers_states/synching_ico.png"));
        mUi->lSyncIcon->setPixmap(sync_icon.pixmap(mUi->lSyncIcon->size()));
    }
}

QString InfoDialogTransferDelegateWidget::getTransferName()
{
    return mUi->lFileName->text();
}

void InfoDialogTransferDelegateWidget::updateFinishedIco(TransferData::TransferTypes transferType, bool error)
{
    QIcon iconCompleted;

    if(transferType & TransferData::TRANSFER_DOWNLOAD || transferType & TransferData::TRANSFER_LTCPDOWNLOAD)
    {
        iconCompleted = Utilities::getCachedPixmap(error ? QString::fromLatin1(":/images/transfer_manager/transfers_states/download_fail_item_ico.png")
                                                         : QString::fromLatin1(":/images/transfer_manager/transfers_states/downloaded_item_ico.png"));
    }
    else if(transferType & TransferData::TRANSFER_UPLOAD)
    {
        iconCompleted = Utilities::getCachedPixmap(error ? QString::fromLatin1(":/images/transfer_manager/transfers_states/upload_fail_item_ico.png")
                                                         : QString::fromLatin1(":/images/transfer_manager/transfers_states/uploaded_item_ico.png"));
    }

    mUi->lTransferTypeCompleted->setPixmap(iconCompleted.pixmap(mUi->lTransferTypeCompleted->size()));
    mUi->lSyncIconCompleted->setVisible(getData()->isSyncTransfer());

    if(getData()->isSyncTransfer())
    {
        auto sync_icon = Utilities::getCachedPixmap(QString::fromLatin1(":/images/transfer_manager/transfers_states/synching_ico.png"));
        mUi->lSyncIconCompleted->setPixmap(sync_icon.pixmap(mUi->lSyncIconCompleted->size()));
    }
}

TransferBaseDelegateWidget::ActionHoverType InfoDialogTransferDelegateWidget::mouseHoverTransfer(bool isHover, const QPoint &pos)
{
    bool update(false);
    ActionHoverType hoverType(ActionHoverType::NONE);

    if(!getData())
    {
        return hoverType;
    }

    mIsHover = isHover;

    // Assume the action widget is shown.
    mUi->lActionTransfer->show();
    mUi->lActionTransfer->setToolTip(QString());

    // Assume the show in folder is hidden
    mUi->lShowInFolder->hide();

    if(getData()->isFinished())
    {
        if (mIsHover)
        {
            if (getData()->mErrorCode < 0)
            {
                if (getData()->canBeRetried())
                {
                    bool in = isMouseHoverInAction(mUi->lActionTransfer, pos);
                    mUi->lActionTransfer->setToolTip(tr("Retry"));
                    update = setActionTransferIcon(mUi->lActionTransfer, QString::fromLatin1("://images/retry.png"));
                    if(update)
                    {
                        hoverType = (in) ? ActionHoverType::HOVER_ENTER : ActionHoverType::HOVER_LEAVE;
                    }
                }
                else
                {
                    update = setActionTransferIcon(mUi->lActionTransfer,
                                                   QString::fromLatin1("://images/error.png"));
                    //Double check that the mFailedTransfer is OK
                    if(getData()->isFailed())
                    {
                        mUi->lActionTransfer->setToolTip(tr("Failed: %1").arg(getErrorText()));
                    }

                    if(update)
                    {
                        hoverType = ActionHoverType::HOVER_LEAVE;
                    }
                }
            }
            else
            {
                bool inAction(false);

                if (getData()->isPublicNode())
                {
                    inAction = isMouseHoverInAction(mUi->lActionTransfer, pos);
                    setActionTransferIcon(mUi->lActionTransfer,
                                                   QString::fromLatin1("://images/link%1.png").arg(QString::fromLatin1(inAction ? "-hover" : "")));
                    mUi->lActionTransfer->setToolTip(tr("Copy link to file"));
                }
                else
                {
                    mUi->lActionTransfer->hide();
                    inAction = true;
                }

                mUi->lShowInFolder->show();

                bool inShowFolder = isMouseHoverInAction(mUi->lShowInFolder, pos);

                bool fileExists = QFile(getData()->path()).exists();

                const char* baseIconName = (fileExists) ? "://images/file-search%1.png" : "://images/file-question%1.png";
                setActionTransferIcon(mUi->lShowInFolder, QString::fromLatin1(baseIconName).arg(QString::fromLatin1(inShowFolder?"-hover":"")));
                QString tooltipText = (fileExists) ? tr("Show in folder") : tr("Deleted or moved file");
                mUi->lShowInFolder->setToolTip(tooltipText);

                hoverType = (inAction || inShowFolder) ? ActionHoverType::HOVER_ENTER : ActionHoverType::HOVER_LEAVE;
            }
        }
        else
        {
            const char* iconName = (getData()->mErrorCode < 0) ? "://images/error.png" : "://images/success.png";
            update = setActionTransferIcon(mUi->lActionTransfer, QString::fromLatin1(iconName));
            mUi->lActionTransfer->setIconSize(QSize(24,24));

            if(update)
            {
                hoverType = ActionHoverType::HOVER_LEAVE;
            }
        }
    }

    return hoverType;
}

bool InfoDialogTransferDelegateWidget::mouseHoverRetryingLabel(QPoint pos)
{
    return (getData()->getState() == TransferData::TransferState::TRANSFER_RETRYING
                && mUi->lSpeed->rect().contains(mUi->lSpeed->mapFrom(this, pos)));
}

void InfoDialogTransferDelegateWidget::finishTransfer()
{
    mUi->sTransferState->setCurrentWidget(mUi->completedTransfer);
    if (getData()->mErrorCode < 0)
    {
        mUi->lActionTransfer->setIcon(QIcon(QString::fromLatin1("://images/error.png")));
        mUi->lActionTransfer->setIconSize(QSize(24,24));
        mUi->lElapsedTime->setStyleSheet(QString::fromUtf8("color: #F0373A"));

        mUi->lElapsedTime->setText(tr("Failed: %1").arg(getErrorText()));
        updateFinishedIco(getData()->mType, true);
    }
    else
    {
        mUi->lActionTransfer->setIcon(QIcon(QString::fromLatin1("://images/success.png")));
        mUi->lActionTransfer->setIconSize(QSize(24,24));
        updateFinishedIco(getData()->mType, false);
    }
}

void InfoDialogTransferDelegateWidget::updateFinishedTime()
{
    auto finishedTime = getData()->getSecondsSinceFinished();

    if (finishedTime < 0 || getData()->mErrorCode < 0)
    {
        return;
    }

    mUi->lElapsedTime->setStyleSheet(QLatin1String("color: #999999"));
    mUi->lElapsedTime->setText(Utilities::getAddedTimeString(finishedTime));
}

QSize InfoDialogTransferDelegateWidget::minimumSizeHint() const
{
    return FullRect.size();
}
QSize InfoDialogTransferDelegateWidget::sizeHint() const
{
    return FullRect.size();
}

bool InfoDialogTransferDelegateWidget::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == mUi->lTransferType && event->type() == QEvent::Resize)
    {
        setType();
    }

    if((watched == mUi->lFileName || watched == mUi->lFileNameCompleted) && event->type() == QEvent::Resize)
    {
        auto nameLabel = dynamic_cast<QLabel*>(watched);
        if(nameLabel)
        {
            auto availableSize(0);
            if(watched == mUi->lFileName)
            {
                availableSize = getNameAvailableSize(mUi->wFileName, mUi->lSyncIcon, mUi->fileNameSpacer);
            }
            else
            {
                availableSize = getNameAvailableSize(mUi->wFileNameCompleted, mUi->lSyncIconCompleted, mUi->fileNameCompletedSpacer);
            }

            nameLabel->setText(nameLabel->fontMetrics()
                                             .elidedText(getData()->mFilename, Qt::ElideMiddle,
                                                         availableSize));
            nameLabel->updateGeometry();
            nameLabel->parentWidget()->updateGeometry();
        }
    }

    if(getData() && watched == mUi->lElapsedTime && getData()->mErrorCode < 0 && (event->type() == QEvent::Resize || event->type() == QEvent::Paint))
    {
        int leftMargin;
        int rightMargin;

        mUi->wContainerCompletedData->layout()->getContentsMargins(&leftMargin, 0, &rightMargin, 0);

        int availableWidth = mUi->wContainerCompletedData->width() - mUi->lFileTypeCompleted->width() - mUi->wOptions->width()
                        - mUi->wContainerCompletedData->layout()->spacing() * 2 - rightMargin - leftMargin
                        - mUi->horizontalLayout_5->spacing() - 3;

        QString text = tr("Failed: %1").arg(getErrorText());
        QString elidedText = mUi->lElapsedTime->fontMetrics().elidedText(text, Qt::ElideMiddle, availableWidth);
        if(text != elidedText)
        {
            mUi->lElapsedTime->setText(elidedText);
            mUi->lElapsedTime->setToolTip(text);
        }
        else
        {
            mUi->lElapsedTime->setToolTip(QString());
        }
    }

    return TransferBaseDelegateWidget::eventFilter(watched, event);
}

void InfoDialogTransferDelegateWidget::reset()
{
    mIsHover = false;
    mTransferRemainingTime.reset();
    TransferBaseDelegateWidget::reset();
}

void InfoDialogTransferDelegateWidget::on_lShowInFolder_clicked()
{ 
    emit openTransferFolder();
}

void InfoDialogTransferDelegateWidget::on_lActionTransfer_clicked()
{
    if (getData()->mErrorCode < 0)
    {
        if (getData()->canBeRetried())
        {
            //Base implementation
            onRetryTransfer();
        }
    }
    else
    {
        emit copyTransferLink();
    }
}

