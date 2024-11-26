#ifndef USER_MESSAGE_MODEL_H
#define USER_MESSAGE_MODEL_H

#include "UserMessageTypes.h"
#include "UserMessage.h"

#include <QAbstractItemModel>

namespace mega
{
class MegaUserAlert;
class MegaUserAlertList;
class MegaNotificationList;
class MegaNotification;
}

class UserMessageModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    using QAbstractItemModel::QAbstractItemModel;
    virtual ~UserMessageModel();

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void processAlerts(mega::MegaUserAlertList* alerts);
    void processNotifications(const mega::MegaNotificationList* notifications);
    bool hasAlertsOfType(MessageType type);
    UnseenUserMessagesMap getUnseenNotifications() const;
    uint32_t checkLocalLastSeenNotification();
    void setLastSeenNotification(uint32_t id);

private slots:
    void onExpired(unsigned id);

private:
    class SeenStatusManager
    {

    public:
        SeenStatusManager() = default;
        virtual ~SeenStatusManager() = default;

        void markAsUnseen(MessageType type);
        bool markNotificationAsUnseen(uint32_t id);
        void markAsSeen(MessageType type);
        UnseenUserMessagesMap getUnseenUserMessages() const;

        void setLastSeenNotification(uint32_t id);
        uint32_t getLastSeenNotification() const;
        void setLocalLastSeenNotification(uint32_t id);
        uint32_t getLocalLastSeenNotification() const;

    private:
        UnseenUserMessagesMap mUnseenNotifications;
        uint32_t mLastSeenNotification = 0;
        uint32_t mLocalLastSeenNotification = 0;

    };

    QList<UserMessage*> mUserMessages;
    SeenStatusManager mSeenStatusManager;

    void insertAlerts(const QList<mega::MegaUserAlert*>& alerts);
    void updateAlerts(const QList<mega::MegaUserAlert*>& alerts);
    void removeAlerts(const QList<mega::MegaUserAlert*>& alerts);

    void insertNotifications(const QList<mega::MegaNotification*>& notifications);
    void updateNotification(int row, const mega::MegaNotification* notification);
    void removeNotifications(const mega::MegaNotificationList* notifications);

    auto findById(unsigned id, UserMessage::Type type);

};

#endif // USER_MESSAGE_MODEL_H
