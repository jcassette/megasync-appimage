#include "TransferBaseDelegateWidget.h"
#include "QMegaMessageBox.h"
#include <MegaTransferView.h>
#include "MegaApplication.h"

#include <QPointer>
#include <QLayout>

TransferBaseDelegateWidget::TransferBaseDelegateWidget(QWidget *parent)
    : QWidget(parent),
      mPreviousState(TransferData::TransferState::TRANSFER_NONE)
{
    connect(MegaSyncApp->getTransfersModel(), &TransfersModel::rowsAboutToBeRemoved, this, &TransferBaseDelegateWidget::onTransferRemoved);
}

void TransferBaseDelegateWidget::updateUi(const QExplicitlySharedDataPointer<TransferData> transferData, int)
{
    if(!mData || mData->mTag != transferData->mTag)
    {
        mPreviousState = TransferData::TransferState::TRANSFER_NONE;
    }

    bool sameTag(mData && mData->mTag == transferData->mTag ? true : false);

    mData = transferData;

    if(!sameTag)
    {
        mLastActionTransferIconName.clear();
        setType();
        setFileNameAndType();
    }
    updateTransferState();

    mPreviousState = mData->getState();
}

bool TransferBaseDelegateWidget::stateHasChanged()
{
    return mPreviousState != mData->getState();
}

QExplicitlySharedDataPointer<TransferData> TransferBaseDelegateWidget::getData()
{
    return mData;
}

QModelIndex TransferBaseDelegateWidget::getCurrentIndex() const
{
    return mCurrentIndex;
}

void TransferBaseDelegateWidget::setCurrentIndex(const QModelIndex &currentIndex)
{
    mCurrentIndex = currentIndex;
}

void TransferBaseDelegateWidget::render(const QStyleOptionViewItem&, QPainter *painter, const QRegion &sourceRegion)
{
    QWidget::render(painter,QPoint(0,0),sourceRegion);
}

bool TransferBaseDelegateWidget::setActionTransferIcon(QToolButton *button, const QString &iconName)
{
    bool update(false);

    auto oldIconName = mLastActionTransferIconName.value(button, QString());

    if (oldIconName.isEmpty() || oldIconName != iconName)
    {
        button->setIcon(Utilities::getCachedPixmap(iconName));
        mLastActionTransferIconName.insert(button, iconName);

        update = true;
    }

    return update;
}

bool TransferBaseDelegateWidget::isMouseHoverInAction(QToolButton *button, const QPoint& mousePos)
{   
    if(button->testAttribute(Qt::WA_WState_Hidden))
    {
        return false;
    }

    auto actionGlobalPos = button->mapTo(this, QPoint(0,0));
    QRect actionGeometry(actionGlobalPos, button->size());

    return actionGeometry.contains(mousePos);
}

void TransferBaseDelegateWidget::onRetryTransfer()
{
    emit retryTransfer();
}

QString TransferBaseDelegateWidget::getState(TRANSFER_STATES state)
{
    switch(state)
    {
        case TRANSFER_STATES::STATE_STARTING:
        {
            return tr("Starting…");
        }
        case TRANSFER_STATES::STATE_RETRYING:
        {
            return tr("Retrying");
        }
        case TRANSFER_STATES::STATE_UPLOADING:
        {
            return tr("Uploading…");
        }
        case TRANSFER_STATES::STATE_DOWNLOADING:
        {
            return tr("Downloading…");
        }
        case TRANSFER_STATES::STATE_SYNCING:
        {
            return tr("Syncing…");
        }
        case TRANSFER_STATES::STATE_COMPLETING:
        {
            return tr("Completing");
        }
        case TRANSFER_STATES::STATE_COMPLETED:
        {
            return tr("Completed");
        }
        case TRANSFER_STATES::STATE_PAUSED:
        {
            return tr("Paused");
        }
        case TRANSFER_STATES::STATE_FAILED:
        {
            return tr("Failed");
        }
        case TRANSFER_STATES::STATE_INQUEUE:
        {
            return tr("In queue");
        }
        case TRANSFER_STATES::STATE_INQUEUE_PARENTHESIS:
        {
            return tr("(in queue)");
        }
        case TRANSFER_STATES::STATE_RETRY:
        {
            return tr("Retry");
        }
        case TRANSFER_STATES::STATE_OUT_OF_STORAGE_SPACE:
        {
            return tr("Out of storage space");
        }
        case TRANSFER_STATES::STATE_OUT_OF_TRANSFER_QUOTA:
        {
            return tr("Out of transfer quota");
        }
        default:
        {
            return QString();
        }
    }
}

int TransferBaseDelegateWidget::getNameAvailableSize(QWidget *nameContainer, QWidget *syncLabel, QSpacerItem *spacer)
{
    return nameContainer->contentsRect().width() - syncLabel->contentsRect().width() - nameContainer->layout()->spacing()*2 - spacer->sizeHint().width();
}

QString TransferBaseDelegateWidget::getErrorText()
{
    return (getData() && getData()->mFailedTransfer && getData()->mFailedTransfer->isForeignOverquota())
                ? QCoreApplication::translate("MegaError", "Destination storage is full.")
                : getErrorInContext();
}

QString TransferBaseDelegateWidget::getErrorInContext()
{
    auto context (mega::MegaError::ErrorContexts::API_EC_DEFAULT);
    switch (getData()->mType)
    {
        case TransferData::TRANSFER_DOWNLOAD:
        case TransferData::TRANSFER_LTCPDOWNLOAD:
        {
            context = mega::MegaError::ErrorContexts::API_EC_DOWNLOAD;
            break;
        }
        case TransferData::TRANSFER_UPLOAD:
        {
            context = mega::MegaError::ErrorContexts::API_EC_UPLOAD;
            break;
        }
        default:
            break;
    }

    // SNC-4190/CON-556
    const char* errorStr = mega::MegaError::getErrorString(getData()->mErrorCode, context);
    const char* BUSINESS_ACCOUNT_EXPIRED_STR = "Business account has expired";

    if (strcmp(errorStr, BUSINESS_ACCOUNT_EXPIRED_STR) == 0)
    {
        // Change "Business account" to a more generic string, to also cover Pro-Flexi accounts
        return QCoreApplication::translate("MegaError", "Your account has expired");
    }

    return QCoreApplication::translate("MegaError", errorStr);
}

void TransferBaseDelegateWidget::onTransferRemoved(const QModelIndex &parent, int first, int last)
{
    if(getData())
    {
        for(int row = first; row <= last; ++row)
        {
            QModelIndex index = MegaSyncApp->getTransfersModel()->index(row, 0, parent);

            const auto d (qvariant_cast<TransferItem>(index.data()).getTransferData());

            if(d && (d->mTag >= getData()->mTag))
            {
                reset();
                //Found
                break;
            }
        }
    }
}

void TransferBaseDelegateWidget::reset()
{
    mPreviousState = TransferData::TransferState::TRANSFER_NONE;
    mData = nullptr;
    mLastActionTransferIconName.clear();
}

void TransferBaseDelegateWidget::changeEvent(QEvent* event)
{
    if(event->type() == QEvent::LanguageChange)
    {
        //Reset to allow the delegate to repaint all items
        mPreviousState = TransferData::TransferState::TRANSFER_NONE;
    }

    QWidget::changeEvent(event);
}
