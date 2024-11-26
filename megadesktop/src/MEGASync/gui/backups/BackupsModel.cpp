#include "BackupsModel.h"

#include "QmlManager.h"
#include "StandardIconProvider.h"
#include "SyncController.h"
#include "SyncInfo.h"

#include "Utilities.h"

#include "megaapi.h"

#include <QQmlContext>

BackupFolder::BackupFolder(const QString& folder,
                           const QString& displayName,
                           bool selected,
                           QObject* parent)
    : QObject(parent)
    , mName(displayName)
    , mSize()
    , mSelected(selected)
    , mDone(false)
    , mFolderSizeReady(false)
    , mError(0)
    , mFolderSize(FileFolderAttributes::NOT_READY)
    , mSdkError(-1)
    , mSyncError(-1)
    , mFolderAttr(nullptr)
    , mFolder(folder)
{
}

void BackupFolder::setSize(long long size)
{
    QVector<int> changedRoles;
    changedRoles.append(BackupsModel::SIZE_READY_ROLE);

    mFolderSizeReady = (size != FileFolderAttributes::NOT_READY);

    if(size > FileFolderAttributes::NOT_READY)
    {
        mFolderSize = size;
        mSize = Utilities::getSizeStringLocalized(mFolderSize);
        changedRoles.append(BackupsModel::SIZE_ROLE);
    }

    if(auto model = dynamic_cast<BackupsModel*>(parent()))
    {
        auto changedIndex = model->index(model->getRow(mFolder), 0);
        model->updateSelectedAndTotalSize();
        emit model->dataChanged(changedIndex, changedIndex, changedRoles);
    }
}

void BackupFolder::setFolder(const QString &folder)
{
    mFolder = folder;
    if(!createFileFolderAttributes())
    {
        mFolderAttr->setPath(mFolder);
    }
    mFolderAttr->requestSize(this, [&](long long size)
    {
        setSize(size);
    });
}

void BackupFolder::setError(int error)
{
    if(mSelected && !mDone && mError == BackupsModel::BackupErrorCode::NONE)
    {
        mError = error;
    }
}

void BackupFolder::calculateFolderSize()
{
    createFileFolderAttributes();
    mFolderAttr->requestSize(this, [&](long long size)
    {
        setSize(size);
    });
}

bool BackupFolder::createFileFolderAttributes()
{
    if(!mFolderAttr)
    {
        mFolderAttr = new LocalFileFolderAttributes(mFolder, this);
        mFolderAttr->setValueUpdatesDisable();
        return true;
    }
    return false;
}

int BackupsModel::CHECK_DIRS_TIME = 1000;

BackupsModel::BackupsModel(QObject* parent)
    : QAbstractListModel(parent)
    , mSelectedRowsTotal(0)
    , mBackupsTotalSize(0)
    , mTotalSizeReady(false)
    , mConflictsSize(0)
    , mCheckAllState(Qt::CheckState::Unchecked)
    , mGlobalError(BackupErrorCode::NONE)
{
    // Append mBackupFolderList with the default dirs
    populateDefaultDirectoryList();

    connect(SyncInfo::instance(), &SyncInfo::syncRemoved,
            this, &BackupsModel::onSyncRemoved);
    connect(&BackupsController::instance(), &BackupsController::backupsCreationFinished,
            this, &BackupsModel::onBackupsCreationFinished);
    connect(&BackupsController::instance(), &BackupsController::backupFinished,
            this, &BackupsModel::onBackupFinished);
    connect(&mCheckDirsTimer, &QTimer::timeout, this, &BackupsModel::checkDirectories);

    QmlManager::instance()->setRootContextProperty(this);
    QmlManager::instance()->addImageProvider(QLatin1String("standardicons"), new StandardIconProvider);

    mCheckDirsTimer.setInterval(CHECK_DIRS_TIME);
    mCheckDirsTimer.start();
}

BackupsModel::~BackupsModel()
{
    QmlManager::instance()->removeImageProvider(QLatin1String("standardicons"));
}

QHash<int, QByteArray> BackupsModel::roleNames() const
{
    static QHash<int, QByteArray> roles {
        { NAME_ROLE, "name" },
        { FOLDER_ROLE, "folder" },
        { SIZE_ROLE, "size" },
        { SIZE_READY_ROLE, "sizeReady" },
        { SELECTED_ROLE, "selected" },
        { DONE_ROLE, "done" },
        { ERROR_ROLE, "error" }
    };

    return roles;
}

int BackupsModel::rowCount(const QModelIndex& parent) const
{
    // When implementing a table based model, rowCount() should return 0 when the parent is valid.
    return parent.isValid() ? 0 : mBackupFolderList.size();
}

bool BackupsModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    bool result = hasIndex(index.row(), index.column(), index.parent()) && value.isValid();

    if (result)
    {
        BackupFolder* item = mBackupFolderList[index.row()];
        switch (role)
        {
            case NAME_ROLE:
                item->mName = value.toString();
                break;
            case FOLDER_ROLE:
                item->setFolder(value.toString());
                break;
            case SIZE_ROLE:
                item->mSize = value.toInt();
                break;
            case SELECTED_ROLE:
            {
                if(checkPermissions(item->getFolder()))
                {
                    item->mSelected = value.toBool();
                }
                checkSelectedAll();
                break;
            }
            case DONE_ROLE:
                item->mDone = value.toBool();
                break;
            case ERROR_ROLE:
                item->mError = value.toInt();
                break;
            default:
                result = false;
                break;
        }

        if(result)
        {
            emit dataChanged(index, index, { role } );
        }
    }

    return result;
}

QVariant BackupsModel::data(const QModelIndex &index, int role) const
{
    QVariant field;

    if (hasIndex(index.row(), index.column(), index.parent()))
    {
        const BackupFolder* item = mBackupFolderList.at(index.row());
        switch (role)
        {
            case NAME_ROLE:
                field = item->mName;
                break;
            case FOLDER_ROLE:
                field = item->getFolder();
                break;
            case SIZE_ROLE:
                field = item->mSize;
                break;
            case SIZE_READY_ROLE:
                field = item->mFolderSizeReady;
                break;
            case SELECTED_ROLE:
                field = item->mSelected;
                break;
            case DONE_ROLE:
                field = item->mDone;
                break;
            case ERROR_ROLE:
                field = item->mError;
                break;
            default:
                break;
        }
    }

    return field;
}

QString BackupsModel::getTotalSize() const
{
    return Utilities::getSizeStringLocalized(mBackupsTotalSize);
}

bool BackupsModel::getIsTotalSizeReady() const
{
    return mTotalSizeReady;
}

Qt::CheckState BackupsModel::getCheckAllState() const
{
    return mCheckAllState;
}

void BackupsModel::setCheckAllState(Qt::CheckState state, bool fromModel)
{
    if(!fromModel && mCheckAllState == Qt::CheckState::Unchecked && state == Qt::CheckState::PartiallyChecked)
    {
        state = Qt::CheckState::Checked;
    }

    if(mCheckAllState == state)
    {
        return;
    }

    if(mCheckAllState != Qt::CheckState::Checked && state == Qt::CheckState::Checked)
    {
        setAllSelected(true);
    }
    else if(mCheckAllState != Qt::CheckState::Unchecked && state == Qt::CheckState::Unchecked)
    {
        setAllSelected(false);
    }
    mCheckAllState = state;
    emit checkAllStateChanged();
}

BackupsController* BackupsModel::backupsController() const
{
    return &BackupsController::instance();
}

bool BackupsModel::getExistConflicts() const
{
    return mConflictsSize > 0;
}

QString BackupsModel::getSdkErrorString() const
{
    QString message = tr("Folder wasn't backed up. Try again.", "", mSdkCount);
    auto itFound = std::find_if(mBackupFolderList.cbegin(), mBackupFolderList.cend(), [](const BackupFolder* backupFolder)
                                {
                                    return (backupFolder->mSelected && backupFolder->mError == SDK_CREATION);
                                });

    if (itFound != mBackupFolderList.cend())
    {
        message = BackupsController::instance().getErrorString((*itFound)->mSdkError, (*itFound)->mSyncError);
    }

    return message;
}

QString BackupsModel::getSyncErrorString() const
{
    QString message;
    auto itFound = std::find_if(mBackupFolderList.cbegin(), mBackupFolderList.cend(), [](const BackupFolder* backupFolder)
                                {
                                    return (backupFolder->mSelected && backupFolder->mError == SYNC_CONFLICT);
                                });

    if (itFound != mBackupFolderList.cend())
    {
        BackupsController::instance().isLocalFolderSyncable((*itFound)->getFolder(), mega::MegaSync::TYPE_BACKUP, message);
    }

    return message;
}

QString BackupsModel::getConflictsNotificationText() const
{
    switch(mGlobalError)
    {
        case DUPLICATED_NAME:
            return tr("You can't back up folders with the same name. "
                        "Rename them to continue with the backup. "
                        "Folder names won't change on your computer.");
        case EXISTS_REMOTE:
            return tr("A folder with the same name already exists in your Backups. "
                        "Rename the new folder to continue with the backup. "
                        "Folder name will not change on your computer.",
                        "", mRemoteCount);
        case SYNC_CONFLICT:
            return getSyncErrorString();
        case PATH_RELATION:
            return tr("Backup folders can't contain or be contained by other backup folder");
        case UNAVAILABLE_DIR:
            return tr("Folder can't be backed up as it can't be located. "
                        "It may have been moved or deleted, or you might not have access.");
        case SDK_CREATION:
            return getSdkErrorString();
        default:
            return QString::fromUtf8("");
    }
}

int BackupsModel::getGlobalError() const
{
    return mGlobalError;
}

void BackupsModel::insert(const QString &folder)
{
    QString inputPath(QDir::toNativeSeparators(QDir(folder).absolutePath()));
    if(selectIfExistsInsertion(inputPath))
    {
        // If the folder exists in the table then select the item and return
        return;
    }

    BackupFolder* data = new BackupFolder(inputPath, BackupsController::instance().getSyncNameFromPath(inputPath), true, this);

    auto newBackupFolderModelIndex = mBackupFolderList.size();
    beginInsertRows(QModelIndex(), newBackupFolderModelIndex, newBackupFolderModelIndex);
    mBackupFolderList.append(data);
    endInsertRows();

    emit newFolderAdded(newBackupFolderModelIndex);

    checkSelectedAll();
}

void BackupsModel::setAllSelected(bool selected)
{
    QListIterator<BackupFolder*> it(mBackupFolderList);

    while(it.hasNext())
    {
        auto backupFolder = it.next();
        if(checkPermissions(backupFolder->getFolder()))
        {
            backupFolder->mSelected = selected;
        }
    }
    emit dataChanged(index(0), index(mBackupFolderList.size() - 1), {SELECTED_ROLE});

    updateSelectedAndTotalSize();
}

bool BackupsModel::checkPermissions(const QString &inputPath)
{
    QDir dir(inputPath);
    dir.isEmpty(); //this triggers permission request on macOS, don´t remove
    if(dir.exists() && !dir.isReadable())//this didn´t trigger permission request
    {
        return false;
    }
    return true;
}

void BackupsModel::populateDefaultDirectoryList()
{
    // Default directories definition
    static QVector<QStandardPaths::StandardLocation> defaultPaths =
    {
        QStandardPaths::DesktopLocation,
        QStandardPaths::DocumentsLocation,
        QStandardPaths::MusicLocation,
        QStandardPaths::PicturesLocation
    };

    // Iterate defaultPaths to add to mBackupFolderList if the path is not empty
    for (auto type : qAsConst(defaultPaths))
    {
        const auto standardPaths (QStandardPaths::standardLocations(type));
        QDir dir (QDir::cleanPath(standardPaths.first()));
        QString path (QDir::toNativeSeparators(dir.canonicalPath()));
        if(dir.exists() && dir != QDir::home() && isLocalFolderSyncable(path))
        {
            BackupFolder* folder = new BackupFolder(path, BackupsController::instance().getSyncNameFromPath(path), false, this);
            mBackupFolderList.append(folder);
        }
        else
        {
            mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_WARNING,
                               QString::fromUtf8("Default path %1 is not valid.").arg(path).toUtf8().constData());
        }
    }
}

void BackupsModel::updateSelectedAndTotalSize()
{
    mSelectedRowsTotal = 0;
    auto lastTotalSize = mBackupsTotalSize;
    long long totalSize = 0;

    int selectedAndSizeReadyFolders = 0;

    for(auto backupFolderIt = mBackupFolderList.cbegin(); backupFolderIt != mBackupFolderList.cend(); ++backupFolderIt)
    {
        const auto& backupFolder = *backupFolderIt;

        if (backupFolder->mSelected)
        {
            ++mSelectedRowsTotal;

            if (backupFolder->mFolderSizeReady)
            {
                ++selectedAndSizeReadyFolders;
                totalSize += backupFolder->mFolderSize;
            }
        }
    }

    if (selectedAndSizeReadyFolders == mSelectedRowsTotal)
    {
        if (totalSize != lastTotalSize)
        {
            mBackupsTotalSize = totalSize;
            emit totalSizeChanged();
        }

        setTotalSizeReady(true);
    }
    else
    {
        setTotalSizeReady(false);
    }

    if (mSelectedRowsTotal == 0)
    {
        emit noneSelected();
    }
}

void BackupsModel::checkSelectedAll()
{
    updateSelectedAndTotalSize();

    Qt::CheckState state = Qt::CheckState::PartiallyChecked;
    if(mSelectedRowsTotal == 0)
    {
        state = Qt::CheckState::Unchecked;
    }
    else if(mSelectedRowsTotal == mBackupFolderList.size())
    {
        state = Qt::CheckState::Checked;
    }
    setCheckAllState(state, true);
}

bool BackupsModel::isLocalFolderSyncable(const QString& inputPath)
{
    QString message;
    return (BackupsController::instance().isLocalFolderSyncable(inputPath, mega::MegaSync::TYPE_BACKUP, message) != SyncController::CANT_SYNC);
}

bool BackupsModel::selectIfExistsInsertion(const QString& inputPath)
{
    auto foundFolderIt = std::find_if(mBackupFolderList.cbegin(), mBackupFolderList.cend(), [&inputPath](const BackupFolder* const folder)
                                      {
                                          return folder->getFolder() == inputPath;
                                      });

    if (foundFolderIt != mBackupFolderList.cend())
    {
        if(!(*foundFolderIt)->mSelected)
        {
            setData(index(static_cast<int>(std::distance(mBackupFolderList.cbegin(), foundFolderIt)), 0), QVariant(true), SELECTED_ROLE);
        }

        return true;
    }

    return false;
}

bool BackupsModel::folderContainsOther(const QString& folder,
                                            const QString& other) const
{
    if(folder == other)
    {
        return true;
    }
    return folder.startsWith(other) && folder[other.size()] == QDir::separator();
}

bool BackupsModel::isRelatedFolder(const QString& folder,
                                        const QString& existingPath) const
{
    return folderContainsOther(folder, existingPath) || folderContainsOther(existingPath, folder);
}

QModelIndex BackupsModel::getModelIndex(QList<BackupFolder*>::iterator item)
{
    int row = static_cast<int>(std::distance(mBackupFolderList.begin(), item));
    return QModelIndex(index(static_cast<int>(row), 0));
}

QList<QList<BackupFolder*>::const_iterator> BackupsModel::getRepeatedNameItList(const QString& name)
{
    QList<QList<BackupFolder*>::const_iterator> ret;
    for (auto it = mBackupFolderList.cbegin() ; it != mBackupFolderList.cend(); ++it)
    {
        if((*it)->mName == name)
        {
            ret.append(it);
        }
    }
    return ret;
}

void BackupsModel::reviewConflicts()
{
    auto item = mBackupFolderList.cbegin();
    int duplicatedCount = 0;
    int syncConflictCount = 0;
    int pathRelationCount = 0;
    int unavailableCount = 0;

    mSdkCount = 0;
    mRemoteCount = 0;

    while (item != mBackupFolderList.cend())
    {
        if((*item)->mSelected)
        {
            switch((*item)->mError)
            {
                case DUPLICATED_NAME:
                    duplicatedCount++;
                    break;
                case EXISTS_REMOTE:
                    mRemoteCount++;
                    break;
                case SYNC_CONFLICT:
                    syncConflictCount++;
                    break;
                case PATH_RELATION:
                    pathRelationCount++;
                    break;
                case UNAVAILABLE_DIR:
                    unavailableCount++;
                    break;
                case SDK_CREATION:
                    mSdkCount++;
                    break;
                default:
                    break;
            }
        }
        item++;
    }

    mConflictsSize = duplicatedCount
                        + mRemoteCount
                        + syncConflictCount
                        + pathRelationCount
                        + unavailableCount;

    if(mSdkCount > 0)
    {
        setGlobalError(BackupErrorCode::SDK_CREATION);
    }
    else if(syncConflictCount > 0)
    {
        setGlobalError(BackupErrorCode::SYNC_CONFLICT);
    }
    else if(duplicatedCount > 0)
    {
        setGlobalError(BackupErrorCode::DUPLICATED_NAME);
    }
    else if(mRemoteCount > 0)
    {
        setGlobalError(BackupErrorCode::EXISTS_REMOTE);
    }
    else if(pathRelationCount > 0)
    {
        setGlobalError(BackupErrorCode::PATH_RELATION);
    }
    else if(unavailableCount > 0)
    {
        setGlobalError(BackupErrorCode::UNAVAILABLE_DIR);
    }
}

void BackupsModel::checkDuplicatedBackups(const QStringList& candidateList)
{
    QSet<QString> remoteSet = BackupsController::instance().getRemoteFolders();
    QSet<QString> localSet;
    QStringListIterator it(candidateList);
    while(it.hasNext())
    {
        auto name = it.next();

        BackupErrorCode error = BackupErrorCode::NONE;

        int remoteSize = remoteSet.size();
        remoteSet.insert(name); //if it fails means that the name already exist in the QSet
        if(remoteSize == remoteSet.size())
        {
            error = BackupErrorCode::EXISTS_REMOTE;
        }
        else
        {
            remoteSet.remove(name); //It went ok so we don´t want the name in the set
        }

        if(error == BackupErrorCode::NONE)
        {
            int localSize = localSet.size();
            localSet.insert(name); //if it fails means that the name already exist in the QSet
            if(localSize == localSet.size())
            {
                error = BackupErrorCode::DUPLICATED_NAME;
            }
        }

        if(error != BackupErrorCode::NONE)
        {
            foreach(auto foldIt, getRepeatedNameItList(name))
            {
                (*foldIt)->setError(error);
            }
        }
    }
}

bool BackupsModel::existOtherRelatedFolder(const int currentRow)
{
    if(currentRow > mBackupFolderList.size())
    {
        return false;
    }

    bool found = false;
    QString folder(mBackupFolderList[currentRow]->getFolder());
    int conflictRow = 1;
    while(!found && conflictRow < rowCount())
    {
        if((found = mBackupFolderList[conflictRow]->mSelected
            && conflictRow!=currentRow &&isRelatedFolder(folder, mBackupFolderList[conflictRow]->getFolder())))
        {
            mBackupFolderList[currentRow]->mError = BackupErrorCode::PATH_RELATION;
            mBackupFolderList[conflictRow]->mError = BackupErrorCode::PATH_RELATION;
        }
        conflictRow++;
    }
    return found;
}

void BackupsModel::check()
{
    // Clean errors
    for (int row = 0; row < rowCount(); row++)
    {
        if (mBackupFolderList[row]->mSelected
            && mBackupFolderList[row]->mError != BackupErrorCode::SDK_CREATION)
        {
            mBackupFolderList[row]->mError = BackupErrorCode::NONE;
        }
    }

    mGlobalError = BackupErrorCode::NONE;

    QStringList candidateList;
    for (int row = 0; row < rowCount(); row++)
    {
        if (mBackupFolderList[row]->mSelected && !mBackupFolderList[row]->mDone)
        {
            QString message;
            candidateList.append(mBackupFolderList[row]->mName);
            if (mBackupFolderList[row]->mError == BackupErrorCode::NONE
                && !existOtherRelatedFolder(row)
                && BackupsController::instance().isLocalFolderSyncable(mBackupFolderList[row]->getFolder(), mega::MegaSync::TYPE_BACKUP, message)
                    != SyncController::CAN_SYNC)
            {
                QDir dir(mBackupFolderList[row]->getFolder());
                mBackupFolderList[row]->mError = dir.exists() ? BackupErrorCode::SYNC_CONFLICT : BackupErrorCode::UNAVAILABLE_DIR;
            }
            else
            {
                mBackupFolderList[row]->calculateFolderSize();
            }
        }
    }

    checkDuplicatedBackups(candidateList);

    reviewConflicts();

    // Change final errors
    for (int row = 0; row < rowCount(); row++)
    {
        if (mBackupFolderList[row]->mSelected)
        {
            emit dataChanged(index(row, 0), index(row, 0), { ERROR_ROLE } );
        }
    }

    if(mGlobalError == BackupErrorCode::NONE)
    {
        emit globalErrorChanged();
    }
}


int BackupsModel::getRow(const QString& folder)
{
    int row = 0;
    bool found = false;
    while(!found && row < rowCount())
    {
        found = mBackupFolderList[row]->getFolder() == folder;
        if (!found)
        {
            row++;
        }
    }

    return row;
}

void BackupsModel::calculateFolderSizes()
{
    for(auto& backupFolder : mBackupFolderList)
    {
        if(backupFolder->mSelected)
        {
            backupFolder->calculateFolderSize();
        }
    }
}

int BackupsModel::rename(const QString& folder, const QString& name)
{
    int row = getRow(folder);
    QString originalName = mBackupFolderList[row]->mName;
    bool hasError = false;
    QSet<QString> candidateSet;
    candidateSet.insert(name);

    QSet<QString> duplicatedSet = BackupsController::instance().getRemoteFolders();
    duplicatedSet.intersect(candidateSet);

    if(!duplicatedSet.isEmpty())
    {
        mBackupFolderList[row]->mError = BackupErrorCode::EXISTS_REMOTE;
        hasError = true;
    }

    if(!hasError)
    {
        int i = -1;
        while(!hasError && ++i < rowCount())
        {
            hasError = (i != row && mBackupFolderList[row]->mSelected && name == mBackupFolderList[i]->mName);
            if (hasError)
            {
                mBackupFolderList[row]->mError = BackupErrorCode::DUPLICATED_NAME;
            }
        }
    }

    if(!hasError)
    {
        setData(index(row, 0), QVariant(name), NAME_ROLE);
        check();
    }
    else
    {
        mBackupFolderList[row]->mName = originalName;
    }

    return mBackupFolderList[row]->mError;
}

void BackupsModel::remove(const QString& folder)
{
    QList<BackupFolder*>::iterator item = mBackupFolderList.begin();
    bool found = false;
    QString name;
    while (!found && item != mBackupFolderList.end())
    {
        if((found = (*item)->getFolder() == folder))
        {
            name = (*item)->mName;
            //QList::size is an int, so it is safe to cast iterator_traits<_InputIter>::difference_type to int
            const auto row = static_cast<int>(std::distance(mBackupFolderList.begin(), item));
            if(row >= 0)
            {
                beginRemoveRows(QModelIndex(), row, row);
                item = mBackupFolderList.erase(item);
                endRemoveRows();
            }
        }
        else
        {
            item++;
        }
    }

    if(found)
    {
        check();
        checkSelectedAll();
    }
}

bool BackupsModel::existsFolder(const QString& inputPath)
{
    bool exists = false;
    QList<BackupFolder*>::iterator item = mBackupFolderList.begin();
    while (!exists && item != mBackupFolderList.end())
    {
        exists = (inputPath == (*item)->getFolder());
        if(!exists)
        {
            item++;
        }
    }
    return exists;
}

void BackupsModel::change(const QString& oldFolder, const QString& newFolder)
{
    if(oldFolder == newFolder)
    {
        return;
    }

    QList<BackupFolder*>::iterator item = mBackupFolderList.begin();
    bool found = false;
    while (!found && item != mBackupFolderList.end())
    {
        if((*item)->mSelected && (found = ((*item)->getFolder() == oldFolder)))
        {
            if(existsFolder(newFolder))
            {
                remove(newFolder);
            }
            setData(index(getRow(oldFolder), 0), QVariant(BackupsController::instance().getSyncNameFromPath(newFolder)), NAME_ROLE);
            setData(index(getRow(oldFolder), 0), QVariant(newFolder), FOLDER_ROLE);

            if((*item)->mError == BackupErrorCode::SDK_CREATION)
            {
                (*item)->mError = BackupErrorCode::NONE;
            }

            checkSelectedAll();
            check();
        }
        item++;
    }
}

void BackupsModel::onSyncRemoved(std::shared_ptr<SyncSettings> syncSettings)
{
    Q_UNUSED(syncSettings);
    check();
}

void BackupsModel::clean(bool resetErrors)
{
    QList<BackupFolder*>::iterator item = mBackupFolderList.begin();
    while (item != mBackupFolderList.end())
    {
        if((*item)->mSelected)
        {
            if((*item)->mDone)
            {
                //QList::size is an int, so it is safe to cast iterator_traits<_InputIter>::difference_type to int
                const auto row = static_cast<int>(std::distance(mBackupFolderList.begin(), item));
                if(row >= 0)
                {
                    beginRemoveRows(QModelIndex(), row, row);
                    item = mBackupFolderList.erase(item);
                    endRemoveRows();
                }
            }
            else
            {
                if(resetErrors)
                {
                    setData(index(getRow((*item)->getFolder()), 0), QVariant(BackupErrorCode::NONE), ERROR_ROLE);
                }
                item++;
            }
        }
        else
        {
            item++;
        }
    }
}

void BackupsModel::setGlobalError(BackupErrorCode error)
{
    if(mGlobalError != error)
    {
        mGlobalError = error;
        emit globalErrorChanged();
    }
}

void BackupsModel::setTotalSizeReady(bool ready)
{
    if(mTotalSizeReady != ready)
    {
        mTotalSizeReady = ready;
        emit totalSizeReadyChanged();
    }
}

void BackupsModel::onBackupsCreationFinished(bool success)
{
    if(success)
    {
        clean();
    }
    else
    {
        reviewConflicts();
    }

    emit backupsCreationFinished(success);
}

void BackupsModel::onBackupFinished(const QString& folder, int errorCode, int syncErrorCode)
{
    int row = getRow(folder);
    if(errorCode == mega::MegaError::API_OK)
    {
        setData(index(row, 0), QVariant(true), DONE_ROLE);
    }
    else
    {
        mBackupFolderList[row]->mSdkError = errorCode;
        mBackupFolderList[row]->mSyncError = syncErrorCode;
        setData(index(row, 0), QVariant(BackupErrorCode::SDK_CREATION), ERROR_ROLE);
    }
}

bool BackupsModel::checkDirectories()
{
    bool success = true;
    bool reviewErrors = false;
    for (int row = 0; row < rowCount(); row++)
    {
        if (mBackupFolderList[row]->mSelected
                && !mBackupFolderList[row]->mDone
                && mBackupFolderList[row]->mError != SDK_CREATION)
        {
            if(!QDir(mBackupFolderList[row]->getFolder()).exists())
            {
                setData(index(row, 0), QVariant(BackupErrorCode::UNAVAILABLE_DIR), ERROR_ROLE);
                success = false;
            }
            else if(mBackupFolderList[row]->mError == BackupErrorCode::UNAVAILABLE_DIR)
            {
                // If actual error is UNAVAILABLE_DIR and it could be located again
                // Then, clean this error
                setData(index(row, 0), QVariant(BackupErrorCode::NONE), ERROR_ROLE);
                reviewErrors = true;
            }
        }
    }

    if(reviewErrors)
    {
        // If one or more UNAVAILABLE_DIR errors have been reverted
        // Then we need to check all the conflicts again
        check();
    }
    else
    {
        // Only review the global conflict
        reviewConflicts();
    }

    return success;
}

// ************************************************************************************************
// * BackupsProxyModel
// ************************************************************************************************

BackupsProxyModel::BackupsProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , mSelectedFilterEnabled(false)
{
    setSourceModel(new BackupsModel(this));
    setDynamicSortFilter(true);

    connect(backupsModel(), &BackupsModel::backupsCreationFinished, this, &BackupsProxyModel::backupsCreationFinished);
}

bool BackupsProxyModel::selectedFilterEnabled() const
{
    return mSelectedFilterEnabled;
}

void BackupsProxyModel::setSelectedFilterEnabled(bool enabled)
{
    if(mSelectedFilterEnabled == enabled) {
        return;
    }

    if(enabled)
    {
        if(auto model = dynamic_cast<BackupsModel*>(sourceModel()))
        {
            model->calculateFolderSizes();
        }
    }
    mSelectedFilterEnabled = enabled;
    emit selectedFilterEnabledChanged();

    invalidateFilter();
}

bool BackupsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if(!mSelectedFilterEnabled) {
        return true;
    }

    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    return index.data(BackupsModel::BackupFolderRoles::SELECTED_ROLE).toBool();
}

QStringList BackupsProxyModel::getSelectedFolders() const
{
    QStringList selectedFolders;
    for (int row = 0; row < rowCount(); row++)
    {
        if(!index(row, 0).data(BackupsModel::BackupFolderRoles::DONE_ROLE).toBool())
        {
            QString folderPath(index(row, 0).data(BackupsModel::BackupFolderRoles::FOLDER_ROLE).toString());
            selectedFolders.append(folderPath);
        }
    }
    return selectedFolders;
}

void BackupsProxyModel::createBackups(SyncInfo::SyncOrigin origin)
{
    if(!backupsModel()->checkDirectories())
    {
        return;
    }

    // All expected errors have been handled
    BackupsController::BackupInfoList candidateList;
    for (int row = 0; row < rowCount(); row++)
    {
        if(!index(row, 0).data(BackupsModel::BackupFolderRoles::DONE_ROLE).toBool())
        {
            BackupsController::BackupInfo candidate;
            candidate.first = index(row, 0).data(BackupsModel::BackupFolderRoles::FOLDER_ROLE).toString();
            candidate.second = index(row, 0).data(BackupsModel::BackupFolderRoles::NAME_ROLE).toString();
            candidateList.append(candidate);
        }
    }
    backupsModel()->backupsController()->addBackups(candidateList, origin);
}

BackupsModel* BackupsProxyModel::backupsModel()
{
    return dynamic_cast<BackupsModel*>(sourceModel());
}
