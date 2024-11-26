#pragma once

#include "SyncSettings.h"
#include "SyncInfo.h"

#include "megaapi.h"

#include <QString>
#include <QDir>

/**
 * @brief Sync Controller class
 *
 * Interface object used to control Syncs and report back on results using Qt Signals.
 * Uses SyncInfo.h class as the data model.
 *
 */

class SyncController: public QObject
{
    Q_OBJECT

public:

    enum Syncability
    {
       CAN_SYNC = 0,
       WARN_SYNC,
       CANT_SYNC,
    };

    struct SyncConfig
    {
        QString localFolder;
        mega::MegaHandle remoteHandle = mega::INVALID_HANDLE;
        QString syncName;
        mega::MegaSync::SyncType type = mega::MegaSync::TYPE_TWOWAY;
        SyncInfo::SyncOrigin origin = SyncInfo::SyncOrigin::MAIN_APP_ORIGIN;
    };

    static SyncController& instance()
    {
        static SyncController instance;
        return instance;
    }

    SyncController(const SyncController&) = delete;
    SyncController& operator=(const SyncController&) = delete;
    ~SyncController(){}

    void addBackup(const QString& localFolder,
                   const QString& syncName,
                   SyncInfo::SyncOrigin origin);
    void addSync(SyncConfig& sync);
    void removeSync(std::shared_ptr<SyncSettings> syncSetting, const mega::MegaHandle& remoteHandle = mega::INVALID_HANDLE);

    void setSyncToRun(std::shared_ptr<SyncSettings> syncSetting);
    void setSyncToPause(std::shared_ptr<SyncSettings> syncSetting);
    void setSyncToSuspend(std::shared_ptr<SyncSettings> syncSetting);
    void setSyncToDisabled(std::shared_ptr<SyncSettings> syncSetting);
    // Change state and then set to running
    void resetSync(std::shared_ptr<SyncSettings> syncSetting,
                   mega::MegaSync::SyncRunningState initialState);

    // Local folder checks
    QString getIsLocalFolderAlreadySyncedMsg(const QString& path, const mega::MegaSync::SyncType& syncType);
    Syncability isLocalFolderAlreadySynced(const QString& path, const mega::MegaSync::SyncType& syncType, QString& message);
    QString getIsLocalFolderAllowedForSyncMsg(const QString& path, const mega::MegaSync::SyncType& syncType);
    Syncability isLocalFolderAllowedForSync(const QString& path, const mega::MegaSync::SyncType& syncType, QString& message);
    Syncability isLocalFolderSyncable(const QString& path, const mega::MegaSync::SyncType& syncType, QString& message);

    // Remote folder check
    Syncability isRemoteFolderSyncable(std::shared_ptr<mega::MegaNode> node, QString& message);

    QString getSyncNameFromPath(const QString& path);

    //Error strings
    QString getErrStrCurrentBackupOverExistingBackup();
    QString getErrStrCurrentBackupInsideExistingBackup();
    QString getErrorString(int errorCode, int syncErrorCode) const;
    QString getRemoteFolderErrorMessage(int errorCode, int syncErrorCode);

signals:
    void syncAddStatus(int errorCode, int syncErrorCode, QString name);
    void syncRemoveStatus(int errorCode);
    void syncRemoveError(std::shared_ptr<mega::MegaError> err);
    void signalSyncOperationBegins();
    void signalSyncOperationEnds();
    void signalSyncOperationError(std::shared_ptr<SyncSettings> sync);
    void backupMoveOrRemoveRemoteFolderError(std::shared_ptr<mega::MegaError> err);

protected:
    SyncController(QObject* parent = nullptr);

private:
    void updateSyncSettings(const mega::MegaError& e, std::shared_ptr<SyncSettings> syncSetting);
    void createPendingBackups(SyncInfo::SyncOrigin origin);
    static QString getSyncAPIErrorMsg(int megaError);
    static QString getSyncTypeString(const mega::MegaSync::SyncType& syncType);
    QMap<QString, QString> mPendingBackups;

    //Sync/Backup operation signals
    void syncOperationBegins();
    void syncOperationEnds();
    uint mActiveOperations;

    mega::MegaApi* mApi;

    SyncInfo* mSyncInfo;
};

Q_DECLARE_METATYPE(std::shared_ptr<mega::MegaError>)
Q_DECLARE_METATYPE(mega::MegaSync::Error)
