#ifndef BACKUPSCONTROLLER_H
#define BACKUPSCONTROLLER_H

#include "SyncController.h"

class BackupsController : public SyncController
{
    Q_OBJECT

public:
    typedef QPair<QString, QString> BackupInfo;
    typedef QList<BackupInfo> BackupInfoList;

    static BackupsController& instance()
    {
        static BackupsController instance;
        return instance;
    }

    BackupsController(const BackupsController&) = delete;
    BackupsController& operator=(const BackupsController&) = delete;

    void addBackups(const BackupInfoList& localPathList,
                    SyncInfo::SyncOrigin origin = SyncInfo::SyncOrigin::NONE);

    QSet<QString> getRemoteFolders() const;

    QString getErrorString(int errorCode, int syncErrorCode) const;

signals:
    void backupFinished(const QString& folder, int errorCode, int syncErrorCode);
    void backupsCreationFinished(bool success);

private:
    BackupsController(QObject *parent = 0);

    mega::MegaApi* mMegaApi;
    int mBackupsToDoSize;
    int mBackupsProcessedWithError;

    // The first field contains the full path and the second contains the backup name
    BackupInfoList mBackupsToDoList;
    SyncInfo::SyncOrigin mBackupsOrigin;

    bool existsName(const QString& name) const;

private slots:
    void onBackupAddRequestStatus(int errorCode, int syncErrorCode, QString name);

};

#endif // BACKUPSCONTROLLER_H
