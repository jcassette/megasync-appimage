#include "megaapi.h"
#include "CommonMessages.h"
#include "DesktopNotifications.h"
#include "EmailRequester.h"
#include "MegaApplication.h"
#include "RequestListenerManager.h"
#include "mega/user.h"
#include "Platform.h"
#include "FullName.h"
#include "MegaApplication.h"
#include "TransferMetaData.h"
#include "TransferNotificationBuilder.h"
#include "Notificator.h"

#include <QCoreApplication>
#include <QtConcurrent/QtConcurrent>

const QString iconPrefix{QStringLiteral("://images/")};
const QString iconFolderName{QStringLiteral("icons")};
const QString newContactIconName{QStringLiteral("new_contact@3x.png")};
const QString storageQuotaFullIconName{QStringLiteral("Storage_Quota_full@3x.png")};
const QString storageQuotaWarningIconName{QStringLiteral("Storage_Quota_almost_full@3x.png")};
const QString failedToDownloadIconName{QStringLiteral("Failed_to_download@3x.png")};
const QString folderIconName{QStringLiteral("Folder@3x.png")};
const QString fileDownloadSucceedIconName{QStringLiteral("File_download_succeed@3x.png")};
constexpr int maxNumberOfUnseenNotifications{3};

bool checkIfActionIsValid(DesktopAppNotification::Action action)
{
    return action == DesktopAppNotification::Action::firstButton
            || action == DesktopAppNotification::Action::legacy
        #ifndef _WIN32
            || action == DesktopAppNotification::Action::content
        #endif
            ;
}

void copyIconsToAppFolder(QString folderPath)
{
    QStringList iconNames;
    iconNames << newContactIconName << storageQuotaFullIconName << storageQuotaWarningIconName
              << failedToDownloadIconName << folderIconName << fileDownloadSucceedIconName;

    for(const auto& iconName : iconNames)
    {
        QFile iconFile(iconPrefix + iconName);
        iconFile.copy(folderPath + QDir::separator() + iconName);
    }
}

QString getIconsPath()
{
    return MegaApplication::applicationDataPath() + QDir::separator() + iconFolderName + QDir::separator();
}

DesktopNotifications::NotificationInfo::NotificationInfo()
    : title(MegaSyncApp->getMEGAString())
{
}

DesktopNotifications::DesktopNotifications(const QString &appName, QSystemTrayIcon *trayIcon)
    :mNewContactIconPath(getIconsPath() + newContactIconName),
     mStorageQuotaFullIconPath(getIconsPath() + storageQuotaFullIconName),
     mStorageQuotaWarningIconPath(getIconsPath() + storageQuotaWarningIconName),
     mFolderIconPath(getIconsPath() + folderIconName),
     mFileDownloadSucceedIconPath(getIconsPath() + fileDownloadSucceedIconName),
     mPreferences(Preferences::instance()),
     mIsFirstTime(true)
{
    mNotificator = std::unique_ptr<Notificator>(new Notificator(appName, trayIcon, this));

    QDir appDir{MegaApplication::applicationDataPath()};
    appDir.mkdir(iconFolderName);
    copyIconsToAppFolder(getIconsPath());

    QObject::connect(&mDelayedNotificator, &NotificationDelayer::sendClusteredAlert, this, &DesktopNotifications::receiveClusteredAlert);
}

QString DesktopNotifications::getItemsAddedText(mega::MegaUserAlert *info)
{
    const int updatedItems = static_cast<int>(info->getNumber(1) + info->getNumber(0));
    auto FullNameRequest = UserAttributes::FullName::requestFullName(info->getEmail());
    QString message(tr("[A] added %n item", "", updatedItems));
    if(FullNameRequest)
    {
        return message
                .replace(QString::fromUtf8("[A]"), FullNameRequest->getFullName());
    }
    else
    {
        return message
                .replace(QString::fromUtf8("[A]"), QString::fromUtf8(info->getEmail()));
    }
}

QString DesktopNotifications::createDeletedShareMessage(mega::MegaUserAlert* info)
{
    QString message;
    QString name;

    const bool someoneLeftTheFolder{info->getNumber(0) == 0};
    auto FullNameRequest = UserAttributes::FullName::requestFullName(info->getEmail());
    if(FullNameRequest)
    {
        name = FullNameRequest->getFullName();
    }
    else
    {
        name = QString::fromUtf8(info->getEmail());
    }

    if (someoneLeftTheFolder)
    {
        message = tr("[A] has left the shared folder")
                .replace(QString::fromUtf8("[A]"), name);
    }
    else //Access for the user was removed by share owner
    {
        message = name.isEmpty() ? tr("Access to shared folder was removed") :
                                       tr("Access to shared folder was removed by [A]")
                                       .replace(QString::fromUtf8("[A]"), name);
    }

    return message;
}

int DesktopNotifications::countUnseenAlerts(mega::MegaUserAlertList *alertList)
{
    auto count = 0;
    for(int iAlert = 0; iAlert < alertList->size(); iAlert++)
    {
        auto alert = alertList->get(iAlert);
        if(!alert->getSeen() && !alert->isRemoved())
        {
            count++;
        }
    }
    return count;
}

void DesktopNotifications::onUserAlertsUpdated(mega::MegaUserAlertList* alertList)
{
    if(mPreferences->isAnyNotificationEnabled())
    {
        const auto unseenAlertsCount = countUnseenAlerts(alertList);
        const bool tooManyAlertsUnseen{unseenAlertsCount > maxNumberOfUnseenNotifications};
        if(tooManyAlertsUnseen || (mIsFirstTime && unseenAlertsCount))
        {
            mIsFirstTime = false;
            notifyUnreadNotifications();
            return;
        }
        mIsFirstTime = false;
    }

    for(int iAlert = 0; iAlert < alertList->size(); iAlert++)
    {
        const auto alert = alertList->get(iAlert);

        // alerts are sent again after seen state updated, so lets only notify the unseen alerts
        if(!alert->getSeen() && !alert->isRemoved())
        {
            sendAlert(alert);
        }
    }
}

void DesktopNotifications::sendAlert(mega::MegaUserAlert* alert)
{
    auto userHandle = alert->getUserHandle();

    if (userHandle != mega::INVALID_HANDLE)
    {
        if(alert->getEmail() != nullptr)
        {
            auto email = QString::fromUtf8(alert->getEmail());

            requestFullName(alert, email);
        }
        else
        {
            requestEmail(alert);
        }
    }
    else
    {
        processAlert(alert);
    }
}

void DesktopNotifications::requestEmail(mega::MegaUserAlert* alert)
{
    auto email = EmailRequester::instance()->getEmail(alert->getUserHandle());

    if (email.isEmpty())
    {
        mega::MegaUserAlert* alertCopy = alert->copy();

        auto requestInfo = EmailRequester::getRequest(alert->getUserHandle());

        connect(requestInfo, &RequestInfo::emailChanged, this, [this, alertCopy](QString email) {
                std::unique_ptr<mega::MegaUserAlert> alert(alertCopy);
                if (!email.isEmpty())
                {
                    requestFullName(alertCopy, email);
                }
            }, Qt::QueuedConnection);
    }
    else
    {
        requestFullName(alert, email);
    }
}

void DesktopNotifications::requestFullName(mega::MegaUserAlert* alert, QString email)
{
    auto fullNameUserAttributes = UserAttributes::FullName::requestFullName(email.toUtf8().constData());
    if(fullNameUserAttributes)
    {
        connect(fullNameUserAttributes.get(), &UserAttributes::FullName::fullNameReady,
                this, &DesktopNotifications::OnUserAttributesReady, Qt::UniqueConnection);
    }

    if(fullNameUserAttributes && !fullNameUserAttributes->isAttributeReady())
    {
        mPendingUserAlerts.insert(email, alert->copy());
    }
    else
    {
        processAlert(alert, email);
    }
}

void DesktopNotifications::processAlert(mega::MegaUserAlert* alert, const QString& email)
{
    QString contactEmail = QString::fromUtf8(alert->getEmail());
    if (contactEmail.isEmpty())
    {
        contactEmail = email;
    }

    QString fullName = contactEmail;
    if (!contactEmail.isEmpty())
    {
        auto FullNameRequest = UserAttributes::FullName::requestFullName(contactEmail.toUtf8().constData());
        if (FullNameRequest)
        {
            fullName = FullNameRequest->getFullName();
        }
    }

    switch (alert->getType())
    {
    case mega::MegaUserAlert::TYPE_INCOMINGPENDINGCONTACT_REQUEST:
    {
        if(mPreferences->isNotificationEnabled(Preferences::NotificationsTypes::NEW_CONTACT_REQUESTS))
        {
            auto notification = CreateContactNotification(tr("New Contact Request"),
                                                         tr("[A] sent you a contact request").replace(QString::fromUtf8("[A]"), fullName),
                                                         email,
                                                         QStringList() << tr("Accept") << tr("Reject"));

            QObject::connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::replyIncomingPendingRequest);

        }
        break;
    }
    case mega::MegaUserAlert::TYPE_INCOMINGPENDINGCONTACT_CANCELLED:
    {
        //Kept to remind decision about this notification
        //This notification is sent when the user cancels a incoming pending notification
        //The current implementation on the SDK filters this kind of notifications, and
        //all "own-caused-user" notifications are blocked.
        //However, as in MEGA Desktop App this notification has been developed,
        //only the last step (notification sending) has been removed just in case it needs to be used again.

        break;
    }
    case mega::MegaUserAlert::TYPE_INCOMINGPENDINGCONTACT_REMINDER:
    {
        if(mPreferences->isNotificationEnabled(Preferences::NotificationsTypes::PENDING_CONTACT_REQUEST_REMINDER))
        {
            auto notification = CreateContactNotification(tr("New Contact Request"),
                                                         tr("Reminder: You have a contact request"),
                                                         email,
                                                         QStringList() << tr("View"));

            QObject::connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::viewContactOnWebClient);
        }
        break;
    }
    case mega::MegaUserAlert::TYPE_CONTACTCHANGE_CONTACTESTABLISHED:
    {
        if(mPreferences->isNotificationEnabled(Preferences::NotificationsTypes::CONTACT_ESTABLISHED))
        {
            auto notification =
                CreateContactNotification(tr("New Contact Established"),
                                          tr("New contact with [A] has been established")
                                              .replace(QString::fromUtf8("[A]"), fullName),
                                          email,
                                          QStringList() << tr("Chat"));

            QObject::connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::viewContactOnWebClient);
        }
        break;
    }
    case mega::MegaUserAlert::TYPE_NEWSHARE:
    {
        if(mPreferences->isNotificationEnabled(Preferences::NotificationsTypes::NEW_FOLDERS_SHARED_WITH_ME))
        {
            const QString message{tr("New shared folder from [A]")
                        .replace(QString::fromUtf8("[A]"), fullName)};

            //Temporary fix. Found a race condition that causes that all nodes are not key decrypted when the SDK event arrives.
            //Delaying the notification 2 seconds fixes the problem, for the moment.
            std::shared_ptr<mega::MegaUserAlert> alertCopy(alert->copy());
            QTimer::singleShot(2000, this, [this, alertCopy, message]()
            {
                notifySharedUpdate(alertCopy.get(), message, NEW_SHARE);
            });
        }
        break;
    }
    case mega::MegaUserAlert::TYPE_DELETEDSHARE:
    {
        if(mPreferences->isNotificationEnabled(Preferences::NotificationsTypes::FOLDERS_SHARED_WITH_ME_DELETED))
        {
            notifySharedUpdate(alert, createDeletedShareMessage(alert), DELETE_SHARE);
        }
        break;
    }
    case mega::MegaUserAlert::TYPE_NEWSHAREDNODES:
    {
        if(mPreferences->isNotificationEnabled(Preferences::NotificationsTypes::NODES_SHARED_WITH_ME_CREATED_OR_REMOVED))
        {
            notifySharedUpdate(alert, getItemsAddedText(alert), NEW_SHARED_NODES);
        }
        break;
    }
    case mega::MegaUserAlert::TYPE_REMOVEDSHAREDNODES:
    case mega::MegaUserAlert::TYPE_UPDATEDSHAREDNODES:
    {
        if(mPreferences->isNotificationEnabled(Preferences::NotificationsTypes::NODES_SHARED_WITH_ME_CREATED_OR_REMOVED))
        {
            mDelayedNotificator.addUserAlert(alert, fullName);
        }
        break;
    }
    case mega::MegaUserAlert::TYPE_PAYMENTREMINDER:
    {
        auto notification = new DesktopAppNotification();
        notification->setTitle(tr("Payment Info"));
        constexpr int paymentReminderIndex{1};
        notification->setText(CommonMessages::createPaymentReminder(alert->getTimestamp(paymentReminderIndex)));
        notification->setActions(QStringList() << tr("Upgrade"));
        connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::redirectToUpgrade);
        mNotificator->notify(notification);
        break;
    }
    case mega::MegaUserAlert::TYPE_TAKEDOWN:
    {
        notifyTakeDown(alert, false);
        break;
    }
    case mega::MegaUserAlert::TYPE_TAKEDOWN_REINSTATED:
    {
        notifyTakeDown(alert, true);
        break;
    }
    default:
        break;
    }
}

DesktopAppNotification* DesktopNotifications::CreateContactNotification(const QString& title,
                                                                 const QString& message,
                                                                 const QString& email,
                                                                 const QStringList& actions)
{
    //No need to delete it after using, the class itself deletes it when activated or closed

    auto notification = new DesktopAppNotification();
    notification->setTitle(title);
    notification->setText(message);
    notification->setData(email);
    notification->setImagePath(mNewContactIconPath);
    notification->setActions(actions);

    mNotificator->notify(notification);

    return notification;
}

void DesktopNotifications::replyIncomingPendingRequest(DesktopAppNotification::Action action)
{
    DesktopAppNotification *notification = qobject_cast<DesktopAppNotification *>(QObject::sender());
    if (!notification)
    {
        return;
    }
    const auto megaApp = static_cast<MegaApplication*>(qApp);
    const auto requestList = megaApp->getMegaApi()->getIncomingContactRequests();
    const auto sourceEmail = notification->getData().toString();

    for (int iRequest=0; iRequest < requestList->size(); iRequest++)
    {
        const auto request = requestList->get(iRequest);
        if (QString::fromUtf8(request->getSourceEmail()) == sourceEmail)
        {
            if (action == DesktopAppNotification::Action::firstButton)
            {
                auto listener = RequestListenerManager::instance().registerAndGetCustomFinishListener(
                    this,
                    [=](mega::MegaRequest* request, mega::MegaError* e) {
                        if (e->getErrorCode() == mega::MegaError::API_OK)
                        {
                            UserAttributes::UserAttributesManager::instance()
                                .updateEmptyAttributesByUser(sourceEmail.toUtf8().constData());
                        }
                    });

                megaApp->getMegaApi()->replyContactRequest(request,
                                                           mega::MegaContactRequest::REPLY_ACTION_ACCEPT,
                                                           listener.get());
            }
            else if (action == DesktopAppNotification::Action::secondButton)
            {
                megaApp->getMegaApi()->replyContactRequest(request, mega::MegaContactRequest::REPLY_ACTION_DENY);
            }
        }
    }
}

std::unique_ptr<mega::MegaNode> getMegaNode(mega::MegaUserAlert* alert)
{
    const auto megaApi = static_cast<MegaApplication*>(qApp)->getMegaApi();
    return std::unique_ptr<mega::MegaNode>(megaApi->getNodeByHandle(alert->getNodeHandle()));
}

void DesktopNotifications::notifySharedUpdate(mega::MegaUserAlert *alert, const QString& message, int type) const
{
    auto notification = new DesktopAppNotification();
    const auto node = getMegaNode(alert);

    QString sharedFolderName;

    if (node && node->isNodeKeyDecrypted())
    {
        sharedFolderName = QString::fromUtf8(node->getName());
    }

    if(sharedFolderName.isEmpty())
    {
        switch (type) {
            case NEW_SHARE:
                sharedFolderName = tr("Shared Folder Received");
                break;
            case DELETE_SHARE:
                sharedFolderName = tr("Shared Folder Removed");
                break;
            case NEW_SHARED_NODES:
            case REMOVED_SHARED_NODES:
                sharedFolderName = tr("Shared Folder Updated");
                break;
            default:
                sharedFolderName = tr("Shared Folder Activity");
                break;
        }
    }
    notification->setTitle(sharedFolderName);
    notification->setText(message);
    if(node)
    {
        notification->setData(QString::fromUtf8(node->getBase64Handle()));
        const auto megaApi = static_cast<MegaApplication*>(qApp)->getMegaApi();
        const auto fullAccess = megaApi->getAccess(node.get()) >= mega::MegaShare::ACCESS_FULL;
        QStringList actions (tr("Show in MEGA"));
        if(type == NEW_SHARE
                && fullAccess
                && node->isNodeKeyDecrypted())
        {
            actions << tr("Sync");
            QObject::connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::replyNewShareReceived);
        }
        else
        {
            QObject::connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::viewShareOnWebClient);
        }
        notification->setActions(actions);
    }

    notification->setImagePath(mFolderIconPath);
    mNotificator->notify(notification);
}

void DesktopNotifications::notifyUnreadNotifications() const
{
    auto notification = new DesktopAppNotification();
    notification->setText(tr("You have unread notifications"));

    notification->setActions(QStringList() << tr("View"));
    QObject::connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::viewOnInfoDialogNotifications);
    mNotificator->notify(notification);
}

QString DesktopNotifications::createTakeDownMessage(mega::MegaUserAlert* alert, bool isReinstated) const
{
    const auto megaApi = static_cast<MegaApplication*>(qApp)->getMegaApi();
    std::unique_ptr<mega::MegaNode> node(megaApi->getNodeByHandle(alert->getNodeHandle()));
    if (node)
    {
        if (node->getType() == mega::MegaNode::TYPE_FILE)
        {
            QString message = isReinstated ? tr("Your publicly shared file ([A]) has been reinstated")
                                             .replace(QString::fromUtf8("[A]"), QString::fromUtf8(node->getName()))
                                           : tr("Your publicly shared file ([A]) has been taken down")
                                             .replace(QString::fromUtf8("[A]"), QString::fromUtf8(node->getName()));
            return message;
        }
        else if (node->getType() == mega::MegaNode::TYPE_FOLDER)
        {

            QString message = isReinstated ? tr("Your publicly shared folder ([A]) has been reinstated")
                                             .replace(QString::fromUtf8("[A]"), QString::fromUtf8(node->getName()))
                                           : tr("Your publicly shared folder ([A]) has been taken down")
                                             .replace(QString::fromUtf8("[A]"), QString::fromUtf8(node->getName()));
            return message;
        }
        else
        {
            QString message = isReinstated ? tr("Your taken down has been reinstated")
                                           : tr("Your publicly shared has been taken down");
            return message;
        }
    }
    else
    {
        QString message = isReinstated ? tr("Your taken down has been reinstated")
                                       : tr("Your publicly shared has been taken down");
        return message;
    }
}

void DesktopNotifications::notifyTakeDown(mega::MegaUserAlert *alert, bool isReinstated) const
{
    auto notification = new DesktopAppNotification();
    const auto node = getMegaNode(alert);
    notification->setTitle(tr("Takedown Notice"));
    notification->setText(createTakeDownMessage(alert, isReinstated));
    if(node)
    {
        notification->setData(QString::fromUtf8(node->getBase64Handle()));
        notification->setActions(QStringList() << tr("Show"));
        QObject::connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::viewShareOnWebClient);
    }

    mNotificator->notify(notification);
}

void DesktopNotifications::viewContactOnWebClient(DesktopAppNotification::Action activationButton) const
{
    DesktopAppNotification *notification = qobject_cast<DesktopAppNotification *>(QObject::sender());
    if(!notification)
    {
        return;
    }
    const auto megaApi = static_cast<MegaApplication*>(qApp)->getMegaApi();
    const auto userMail = megaApi->getContact(notification->getData().toString().toUtf8());
    auto url = QUrl(QString::fromUtf8("mega://#fm/contacts"));
    const auto userVisible = userMail && userMail->getVisibility() == mega::MegaUser::VISIBILITY_VISIBLE;
    if (userVisible)
    {
        const QString userHandle{QString::fromUtf8(megaApi->userHandleToBase64(userMail->getHandle()))};
        const bool actionIsViewContact{checkIfActionIsValid(activationButton)};
        const bool actionIsOpenChat{activationButton == DesktopAppNotification::Action::secondButton};
        if(actionIsViewContact)
        {
            url = QUrl(QString::fromUtf8("mega://#fm/%1").arg(userHandle));
        }
        else if(actionIsOpenChat)
        {
            url = QUrl(QString::fromUtf8("mega://#fm/chat/p/%1").arg(userHandle));
        }
    }
    Utilities::openUrl(url);
    delete userMail;
}

void DesktopNotifications::sendOverStorageNotification(int state) const
{
    switch (state)
    {
    case Preferences::STATE_ALMOST_OVER_STORAGE:
    {
        auto notification = new DesktopAppNotification();
        notification->setTitle(tr("Your account is almost full."));
        notification->setText(tr("Upgrade now to a Pro account."));
        notification->setActions(QStringList() << tr("Get Pro"));
        notification->setImagePath(mStorageQuotaWarningIconPath);
        connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::redirectToUpgrade);
        mNotificator->notify(notification);
        break;
    }
    case Preferences::STATE_OVER_STORAGE:
    {
        auto notification = new DesktopAppNotification();
        notification->setTitle(tr("Your account is full."));
        notification->setText(tr("Upgrade now to a Pro account."));
        notification->setActions(QStringList() << tr("Get Pro"));
        notification->setImagePath(mStorageQuotaFullIconPath);
        connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::redirectToUpgrade);
        mNotificator->notify(notification);
        break;
    }
    case Preferences::STATE_PAYWALL:
    {
        auto notification = new DesktopAppNotification();
        notification->setTitle(tr("Your data is at risk"));
        const auto megaApi = static_cast<MegaApplication*>(qApp)->getMegaApi();
        int64_t remainDaysOut(0);
        Utilities::getDaysToTimestamp(megaApi->getOverquotaDeadlineTs(), remainDaysOut);
        notification->setText(tr("You have %n day left to save your data", "", static_cast<int>(remainDaysOut)));
        notification->setActions(QStringList() << tr("Get Pro"));
        connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::redirectToUpgrade);
        mNotificator->notify(notification);
        break;
    }
    default:
        break;
    }
}

void DesktopNotifications::sendOverTransferNotification(const QString &title) const
{
    const auto notification = new DesktopAppNotification();
    notification->setTitle(title);
    notification->setText(tr("Upgrade now to a Pro account."));
    notification->setActions(QStringList() << tr("Get Pro"));
    connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::redirectToUpgrade);
    mNotificator->notify(notification);
}

void DesktopNotifications::sendFinishedTransferNotification(unsigned long long appDataId) const
{
    auto data = TransferMetaDataContainer::getAppDataById(appDataId);
    if (data)
    {
        const auto megaApi = static_cast<MegaApplication*>(qApp)->getMegaApi();

        mPreferences->setLastTransferNotificationTimestamp();
        TransferNotificationBuilder messageBuilder(data);
        auto info = messageBuilder.buildNotification(megaApi->isLoggedIn());

        auto notification = new DesktopAppNotification();
        notification->setText(info.message);
        notification->setActions(info.actions);
        data->setNotification(notification);
        notification->setTitle(info.title);
        notification->setImagePath(info.imagePath);

        if(data->isDownload())
        {
            connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::actionPressedOnDownloadFinishedTransferNotification);
        }
        else if(data->isUpload())
        {
            connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::actionPressedOnUploadFinishedTransferNotification);
        }

        mNotificator->notify(notification);
    }
}

void DesktopNotifications::sendFinishedSetDownloadNotification(const QString& setName,
                                                               const QStringList& succeededDownloadedElements,
                                                               const QStringList& failedDownloadedElements,
                                                               const QString& destinationPath)
{
    (void) setName;
    QString title = TransferNotificationBuilder::getDownloadFailedTitle();
    QString msg = QString::fromUtf8("");
    int nrSuccessItems = succeededDownloadedElements.size();
    int nrFailedItems = failedDownloadedElements.size();
    QStringList actions;

    mPreferences->setLastTransferNotificationTimestamp();
    mSetDownloadPath = destinationPath;

    // Extract the directory out of the full destinationPath
    QDir dir(destinationPath);
    QString directory = dir.dirName();

    // Title
    if (nrSuccessItems == 0)
    {
        // All have failed
        if (nrFailedItems == 1)
        {
            // Single file failure
            msg = TransferNotificationBuilder::getSingleDownloadFailed(failedDownloadedElements.front(), directory);
        }
        else
        {
            // Multiple files failed
            msg = TransferNotificationBuilder::getDownloadFailedText(nrFailedItems, directory);
        }
    }
    else if (nrFailedItems > 0)
    {
        // Some have failed
        title = TransferNotificationBuilder::getDownloadSomeFailedTitle();
        msg = TransferNotificationBuilder::getSomeDownloadFailedText(nrSuccessItems, nrFailedItems);
        actions << TransferNotificationBuilder::getShowInFolderText();
    }
    else // All files successfully downloaded
    {
        title = TransferNotificationBuilder::getDownloadSuccessTitle();
        msg = TransferNotificationBuilder::getDownloadSuccessText(nrSuccessItems, directory);
        actions << TransferNotificationBuilder::getShowInFolderText();
    }

    auto notification = new DesktopAppNotification();
    notification->setText(msg);
    notification->setActions(actions);
    notification->setTitle(title);

    connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::actionPressedOnDownloadSetFinished);
    mNotificator->notify(notification);
}

void DesktopNotifications::redirectToUpgrade(DesktopAppNotification::Action activationButton) const
{
    if (checkIfActionIsValid(activationButton))
    {
        Utilities::upgradeClicked();
    }
}

void DesktopNotifications::sendBusinessWarningNotification(int businessStatus) const
{
    const auto megaApi = static_cast<MegaApplication*>(qApp)->getMegaApi();

    switch (businessStatus)
    {
    case mega::MegaApi::BUSINESS_STATUS_GRACE_PERIOD:
    {
        if (megaApi->isMasterBusinessAccount())
        {
            const auto notification = new DesktopAppNotification();
            notification->setTitle(tr("Payment Failed"));
            notification->setText(tr("Please resolve your payment issue to avoid suspension of your account."));
            notification->setActions(QStringList() << tr("Pay Now"));

            connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::redirectToPayBusiness);
            mNotificator->notify(notification);
        }
        break;
    }
    case mega::MegaApi::BUSINESS_STATUS_EXPIRED:
    {
        const auto notification = new DesktopAppNotification();

        if (megaApi->isProFlexiAccount())
        {
            const QString message = CommonMessages::getExpiredProFlexiMessage();
            notification->setTitle(tr("Pro Flexi Account deactivated"));
            notification->setText(tr(message.toLatin1()));
            notification->setActions(QStringList() << tr("Pay Now"));

            connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::redirectToPayBusiness);
        }
        else if (megaApi->isMasterBusinessAccount())
        {
            notification->setTitle(tr("Your Business account is expired"));
            notification->setText(tr("Your account is suspended as read only until you proceed with the needed payments."));
            notification->setActions(QStringList() << tr("Pay Now"));

            connect(notification, &DesktopAppNotification::activated, this, &DesktopNotifications::redirectToPayBusiness);
        }
        else
        {
            notification->setTitle(tr("Account Suspended"));
            notification->setImagePath(QString());
            notification->setText(tr("Contact your business account administrator to resolve the issue and activate your account."));
        }
        mNotificator->notify(notification);
        break;
    }
    default:
        break;
    }
}

void DesktopNotifications::sendInfoNotification(const QString& title, const QString& message) const
{
    NotificationInfo info;
    info.title = title;
    info.message = message;
    sendInfoNotification(info);
}

void DesktopNotifications::sendInfoNotification(const NotificationInfo& info) const
{
    if(mPreferences->isNotificationEnabled(Preferences::NotificationsTypes::INFO_MESSAGES))
    {
        auto notification = new DesktopAppNotification();
        notification->setText(info.message);
        notification->setActions(info.actions);
        notification->setTitle(info.title);
        notification->setType(NotificatorBase::Information);
        notification->setImagePath(info.imagePath);

        if(info.activatedFunction)
        {
            connect(notification, &DesktopAppNotification::activated, this, info.activatedFunction);
        }

        //Protect against notification from other threads
        if(MegaSyncApp->thread() != MegaSyncApp->thread()->currentThread())
        {
            Utilities::queueFunctionInAppThread([this, notification](){
                mNotificator->notify(notification);
            });
        }
        else
        {
            mNotificator->notify(notification);
        }
    }
}

//Warning notifications are not in used. If in a future they are used, keep in mind whether they should
//be included in notifications settings or sent always
void DesktopNotifications::sendWarningNotification(const QString &title, const QString &message) const
{
     mNotificator->notify(NotificatorBase::Warning, title, message);
}

//Error notifications are not in used. If in a future they are used, keep in mind whether they should
//be included in notifications settings or sent always
void DesktopNotifications::sendErrorNotification(const QString &title, const QString &message) const
{
    mNotificator->notify(NotificatorBase::Warning, title, message);
}

void DesktopNotifications::redirectToPayBusiness(DesktopAppNotification::Action activationButton) const
{
    if (checkIfActionIsValid(activationButton))
    {
        QString url = QString::fromUtf8("mega://#repay");
        Utilities::getPROurlWithParameters(url);
        Utilities::openUrl(QUrl(url));
    }
}

void DesktopNotifications::actionPressedOnDownloadFinishedTransferNotification(DesktopAppNotification::Action action) const
{
    DesktopAppNotification *notification = qobject_cast<DesktopAppNotification *>(QObject::sender());
    if(!notification)
    {
        return;
    }

    if(notification->getData().isValid())
    {
        auto dataId = notification->getData().toULongLong();
        auto data = TransferMetaDataContainer::getAppDataById<DownloadTransferMetaData>(dataId);
        if(data)
        {
            switch(action)
            {
                case DesktopAppNotification::Action::firstButton:
                {
                    if(data->allHaveFailed())
                    {
                        data->unlinkNotification();
                        MegaSyncApp->getTransfersModel()->retryTransfersByAppDataId(data);
                    }
                    else
                    {
                        auto localPaths = data->getLocalPaths();
                        if(!localPaths.isEmpty())
                        {
                            Platform::getInstance()->showInFolder(localPaths.first());
                        }
                        else
                        {
                            QtConcurrent::run(QDesktopServices::openUrl,
                                              QUrl::fromLocalFile(data->getLocalTargetPath()));
                        }
                    }
                    break;
                }
                case DesktopAppNotification::Action::secondButton:
                {
                    if(data->isSingleTransfer())
                    {
                        auto localPaths = data->getLocalPaths();
                        if(!localPaths.isEmpty())
                        {
                            QtConcurrent::run(QDesktopServices::openUrl, QUrl::fromLocalFile(localPaths.first()));
                        }
                    }
                    else
                    {
                        if(data->someHaveFailed())
                        {
                            data->unlinkNotification();
                            MegaSyncApp->getTransfersModel()->retryTransfersByAppDataId(data);
                        }
                    }

                    break;
                }
                default:
                    break;
            }
        }
    }
}

void DesktopNotifications::actionPressedOnDownloadSetFinished(DesktopAppNotification::Action action)
{
    if (mSetDownloadPath.isEmpty()) { return; }
    Platform::getInstance()->showInFolder(mSetDownloadPath);

    // Reset
    mSetDownloadPath = QString::fromUtf8("");
}

void DesktopNotifications::actionPressedOnUploadFinishedTransferNotification(DesktopAppNotification::Action action) const
{
    DesktopAppNotification *notification = qobject_cast<DesktopAppNotification *>(QObject::sender());
    if(!notification)
    {
        return;
    }
    
    if(notification->getData().isValid())
    {
        auto dataId = notification->getData().toULongLong();
        auto data = TransferMetaDataContainer::getAppDataById(dataId);
        if(data)
        {
            switch(action)
            {
            case DesktopAppNotification::Action::firstButton:
            {
                if(data->allHaveFailed())
                {
                    data->unlinkNotification();
                    MegaSyncApp->getTransfersModel()->retryTransfersByAppDataId(data);
                }
                else
                {
                    auto node = UploadTransferMetaData::getDestinationNodeByData(data);
                    if(node)
                    {
                        viewShareOnWebClientByHandle(QString::fromUtf8(node->getBase64Handle()));
                    }
                }

                break;
            }
            case DesktopAppNotification::Action::secondButton:
            {
                if(data->someHaveFailed())
                {
                    data->unlinkNotification();
                    MegaSyncApp->getTransfersModel()->retryTransfersByAppDataId(data);
                }
                else if(data->isSingleTransfer())
                {
                    QList<std::shared_ptr<mega::MegaNode>> nodes = UploadTransferMetaData::getNodesByData(data);
                    getRemoteNodeLink(nodes);
                }
                break;
            }
            default:
                break;
            }
        }
    }
}

void DesktopNotifications::getRemoteNodeLink(const QList<std::shared_ptr<mega::MegaNode>>& nodes) const
{
    if(!nodes.isEmpty())
    {
        QList<mega::MegaHandle> exportList;
        QStringList linkList;

        foreach(auto node, nodes)
        {
            if(!node->isPublic())
            {
                exportList.append(node->getHandle());
            }
            else
            {
                char *handle = node->getBase64Handle();
                char *key = node->getBase64Key();
                if (handle && key)
                {
                    QString link = Preferences::BASE_URL + QString::fromUtf8("/#!%1!%2")
                            .arg(QString::fromUtf8(handle), QString::fromUtf8(key));
                    linkList.push_back(link);
                }
                delete [] key;
                delete [] handle;
            }
        }

        if (exportList.size() || linkList.size())
        {
            qobject_cast<MegaApplication*>(qApp)->exportNodes(exportList, linkList);
        }
    }
}

void DesktopNotifications::viewShareOnWebClient() const
{
    DesktopAppNotification *notification = qobject_cast<DesktopAppNotification *>(QObject::sender());
    if(!notification)
    {
        return;
    }
    auto nodeBase64Handle = notification->getData().toString();
    viewShareOnWebClientByHandle(nodeBase64Handle);
}

void DesktopNotifications::viewShareOnWebClientByHandle(const QString& nodeBase64Handle) const
{
    if(!nodeBase64Handle.isEmpty())
    {
        const auto url = QUrl(QString::fromUtf8("mega://#fm/%1").arg(nodeBase64Handle));
        Utilities::openUrl(url);
    }
}

void DesktopNotifications::receiveClusteredAlert(mega::MegaUserAlert *alert, const QString& message) const
{
    switch (alert->getType())
    {
        case mega::MegaUserAlert::TYPE_REMOVEDSHAREDNODES:
        {
            notifySharedUpdate(alert, message, REMOVED_SHARED_NODES);
            break;
        }
        case mega::MegaUserAlert::TYPE_UPDATEDSHAREDNODES:
        {
            notifySharedUpdate(alert, message, NEW_SHARED_NODES);
            break;
        }
    }
}

void DesktopNotifications::replyNewShareReceived(DesktopAppNotification::Action action) const
{
    const bool actionIsViewOnWebClient{checkIfActionIsValid(action)};
    DesktopAppNotification *notification = qobject_cast<DesktopAppNotification *>(QObject::sender());
    if(!notification)
    {
        return;
    }
    auto base64Handle = notification->getData().toString();

    const bool actionIsSyncShare{action == DesktopAppNotification::Action::secondButton};
    if(actionIsViewOnWebClient)
    {
        viewShareOnWebClientByHandle(base64Handle);
    }
    else if(actionIsSyncShare)
    {
        if(!base64Handle.isEmpty())
        {
            const auto megaFolderHandle =
                mega::MegaApi::base64ToUserHandle(base64Handle.toUtf8().constData());
            const auto megaApp = static_cast<MegaApplication*>(qApp);
            megaApp->openSettingsAddSync(megaFolderHandle);
        }
    }
}

void DesktopNotifications::viewOnInfoDialogNotifications(DesktopAppNotification::Action action) const
{
    if(checkIfActionIsValid(action))
    {
        const auto megaApp = static_cast<MegaApplication*>(qApp);
        megaApp->showInfoDialogNotifications();
    }
}

void DesktopNotifications::OnUserAttributesReady()
{
    auto UserAttribute = dynamic_cast<UserAttributes::FullName*>(sender());
    if(UserAttribute)
    {
        auto pendingAlerts = mPendingUserAlerts.values(UserAttribute->getEmail());
        if(!pendingAlerts.isEmpty())
        {
            foreach(auto alert, pendingAlerts)
            {
                processAlert(alert, UserAttribute->getEmail());
                delete alert;
            }
            mPendingUserAlerts.remove(UserAttribute->getEmail());
        }

        //Disconnect the full name attribute request as it still lives
        //in attributes manager
        disconnect(UserAttribute, &UserAttributes::FullName::fullNameReady,
                this, &DesktopNotifications::OnUserAttributesReady);
    }
}
