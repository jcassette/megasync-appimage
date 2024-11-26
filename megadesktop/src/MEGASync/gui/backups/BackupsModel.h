#ifndef BACKUPSMODEL_H
#define BACKUPSMODEL_H

#include "BackupsController.h"

#include "FileFolderAttributes.h"
#include "SyncController.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QTimer>

class BackupFolder : public QObject
{
    Q_OBJECT

public:

    // Front (with role)
    QString mName;
    QString mSize;
    bool mSelected;
    bool mDone;
    bool mFolderSizeReady;
    int mError;

    // Back (without role)
    long long mFolderSize;
    int mSdkError;
    int mSyncError;

    BackupFolder();

    BackupFolder(const BackupFolder& folder);

    BackupFolder(const QString& folder,
                 const QString& displayName,
                 bool selected = true, QObject* parent = nullptr);

    LocalFileFolderAttributes* mFolderAttr;
    void setSize(long long size);
    void setFolder(const QString& folder);
    void setError(int error);
    QString getFolder() const {return mFolder;}
    void calculateFolderSize();

private:
    bool createFileFolderAttributes();
    QString mFolder;
};

class BackupsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString totalSize
               READ getTotalSize
               NOTIFY totalSizeChanged)
    Q_PROPERTY(bool totalSizeReady
               READ getIsTotalSizeReady
               NOTIFY totalSizeReadyChanged)
    Q_PROPERTY(Qt::CheckState checkAllState
               READ getCheckAllState
               WRITE setCheckAllState
               NOTIFY checkAllStateChanged)
    Q_PROPERTY(QString conflictsNotificationText
               READ getConflictsNotificationText
               NOTIFY globalErrorChanged)
    Q_PROPERTY(int globalError
               READ getGlobalError
               NOTIFY globalErrorChanged)

public:

    enum BackupFolderRoles
    {
        NAME_ROLE = Qt::UserRole + 1,
        FOLDER_ROLE,
        SIZE_ROLE,
        SIZE_READY_ROLE,
        SELECTED_ROLE,
        SELECTABLE_ROLE,
        DONE_ROLE,
        ERROR_ROLE
    };

    enum BackupErrorCode
    {
        NONE = 0,
        DUPLICATED_NAME = 1,
        EXISTS_REMOTE = 2,
        SYNC_CONFLICT = 3,
        PATH_RELATION = 4,
        UNAVAILABLE_DIR = 5,
        SDK_CREATION = 6
    };
    Q_ENUM(BackupErrorCode)

    explicit BackupsModel(QObject* parent = nullptr);
    ~BackupsModel();

    QHash<int,QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant data(const QModelIndex & index, int role = NAME_ROLE) const override;
    QString getTotalSize() const;
    bool getIsTotalSizeReady() const;
    Qt::CheckState getCheckAllState() const;
    void setCheckAllState(Qt::CheckState state, bool fromModel = false);
    BackupsController* backupsController() const;
    bool getExistConflicts() const;
    QString getConflictsNotificationText() const;
    int getGlobalError() const;
    int getRow(const QString& folder);
    void calculateFolderSizes();
    void updateSelectedAndTotalSize();

public slots:
    void insert(const QString& folder);
    void check();
    int rename(const QString& folder, const QString& name);
    void remove(const QString& folder);
    void change(const QString& oldFolder, const QString& newFolder);
    bool checkDirectories();
    void clean(bool resetErrors = false);

signals:
    void totalSizeChanged();
    void checkAllStateChanged();
    void noneSelected();
    void globalErrorChanged();
    void totalSizeReadyChanged();
    void backupsCreationFinished(bool succes);
    void newFolderAdded(int newFolderIndex);

private:
    static int CHECK_DIRS_TIME;

    QList<BackupFolder*> mBackupFolderList;
    int mSelectedRowsTotal;
    long long mBackupsTotalSize;
    bool mTotalSizeReady;
    int mConflictsSize;
    Qt::CheckState mCheckAllState;
    int mGlobalError;
    QTimer mCheckDirsTimer;
    int mSdkCount;
    int mRemoteCount;

    void populateDefaultDirectoryList();
    void checkSelectedAll();
    bool isLocalFolderSyncable(const QString& inputPath);
    bool selectIfExistsInsertion(const QString& inputPath);
    QList<QList<BackupFolder*>::const_iterator> getRepeatedNameItList(const QString& name);

    bool folderContainsOther(const QString& folder,
                             const QString& other) const;
    bool isRelatedFolder(const QString& folder,
                         const QString& existingPath) const;
    QModelIndex getModelIndex(QList<BackupFolder*>::iterator item);
    void setAllSelected(bool selected);
    bool checkPermissions(const QString& inputPath);
    void checkDuplicatedBackups(const QStringList &candidateList);
    void reviewConflicts();
    void changeConflictsNotificationText(const QString& text);
    bool existOtherRelatedFolder(const int currentRow);
    bool existsFolder(const QString& inputPath);
    void setGlobalError(BackupErrorCode error);
    void setTotalSizeReady(bool ready);
    QString getSdkErrorString() const;
    QString getSyncErrorString() const;

private slots:
    void onSyncRemoved(std::shared_ptr<SyncSettings> syncSettings);
    void onBackupsCreationFinished(bool success);
    void onBackupFinished(const QString& folder, int errorCode, int syncErrorCode);

};

class BackupsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

    Q_PROPERTY(bool selectedFilterEnabled READ selectedFilterEnabled
               WRITE setSelectedFilterEnabled NOTIFY selectedFilterEnabledChanged)

public:
    explicit BackupsProxyModel(QObject* parent = nullptr);
    bool selectedFilterEnabled() const;
    void setSelectedFilterEnabled(bool enabled);

    Q_INVOKABLE QStringList getSelectedFolders() const;

public slots:
    void createBackups(SyncInfo::SyncOrigin origin = SyncInfo::SyncOrigin::MAIN_APP_ORIGIN);

signals:
    void selectedFilterEnabledChanged();
    void backupsCreationFinished(bool success);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    bool mSelectedFilterEnabled;
    BackupsModel* backupsModel();
};

#endif // BACKUPSMODEL_H
