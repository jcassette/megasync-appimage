#include "StalledIssuesUtilities.h"

#include <MegaApplication.h>
#include <mega/types.h>
#include <MegaDownloader.h>
#include <QTMegaRequestListener.h>
#include <MegaApiSynchronizedRequest.h>
#include <QMegaMessageBox.h>
#include <DialogOpener.h>
#include <StalledIssuesDialog.h>
#include <MoveToMEGABin.h>


#include <QFile>
#include <QDir>

const Text::Decorator StalledIssuesBoldTextDecorator::boldTextDecorator = Text::Decorator(new Text::Bold());
const Text::Decorator StalledIssuesNewLineTextDecorator::newLineTextDecorator = Text::Decorator(new Text::NewLine());

////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::unique_ptr<MegaDownloader> StalledIssuesUtilities::mDownloader = nullptr;
std::unique_ptr<MegaUploader> StalledIssuesUtilities::mUploader = nullptr;

StalledIssuesUtilities::StalledIssuesUtilities()
{}

std::shared_ptr<mega::MegaError> StalledIssuesUtilities::removeRemoteFile(const QString& path)
{
    std::unique_ptr<mega::MegaNode> fileNode(
        MegaSyncApp->getMegaApi()->getNodeByPath(path.toUtf8().constData()));
    return removeRemoteFile(fileNode.get());
}

std::shared_ptr<mega::MegaError> StalledIssuesUtilities::removeRemoteFile(mega::MegaNode *node)
{
    std::shared_ptr<mega::MegaError> error;

    if(node)
    {
        auto moveToBinError = MoveToMEGABin::moveToBin(node->getHandle(), QLatin1String("SyncDebris"), true);
        if(moveToBinError.binFolderCreationError)
        {
            error = moveToBinError.binFolderCreationError;
        }
        else if(moveToBinError.moveError)
        {
            error = moveToBinError.moveError;
        }
    }

    return error;
}

bool StalledIssuesUtilities::removeLocalFile(const QString& path, const mega::MegaHandle& syncId)
{
    bool result(false);

    QFile file(path);
    if (file.exists())
    {
        if (syncId != mega::INVALID_HANDLE)
        {
            MegaApiSynchronizedRequest::runRequestWithResult(
                &mega::MegaApi::moveToDebris,
                MegaSyncApp->getMegaApi(),
                [=, &result
#ifdef Q_OS_WIN
                    ,&file
#endif
                ](mega::MegaRequest*, mega::MegaError* e)
                {
                    // In case of error, move to OS trash
                    if (e->getErrorCode() != mega::MegaError::API_OK)
                    {
                        mega::MegaApi::log(
                            mega::MegaApi::LOG_LEVEL_ERROR,
                            QString::fromUtf8("Unable to move file to debris: %1. Error: %2")
                                .arg(path, Utilities::getTranslatedError(e))
                                .toUtf8()
                                .constData());
                        result = QFile::moveToTrash(path);
#ifdef Q_OS_WIN
                        //When the file has no rights, the QFile::moveToTrash fails on Windows
                        if(result && file.exists())
                        {
                            result = false;
                        }
#endif
                    }
                    else
                    {
                        result = true;
                    }
                },
                path.toUtf8().constData(),
                syncId);
        }
        else
        {
            result = QFile::moveToTrash(path);
        }
    }

    return result;
}

QIcon StalledIssuesUtilities::getLocalFileIcon(const QFileInfo &fileInfo, bool hasProblem)
{
    bool isFile(false);

    if(fileInfo.exists())
    {
        isFile = fileInfo.isFile();
    }
    else
    {
        isFile = !fileInfo.completeSuffix().isEmpty();
    }

    return getIcon(isFile, fileInfo, hasProblem);
}

QIcon StalledIssuesUtilities::getRemoteFileIcon(mega::MegaNode *node, const QFileInfo& fileInfo, bool hasProblem)
{
    if(node)
    {
        return getIcon(node->isFile(), fileInfo, hasProblem);
    }
    else
    {
        return Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/help-circle.png"));
    }
}

QIcon StalledIssuesUtilities::getIcon(bool isFile, const QFileInfo& fileInfo, bool hasProblem)
{
    QIcon fileTypeIcon;

    if(isFile)
    {
        //Without extension
        if(fileInfo.completeSuffix().isEmpty())
        {
            fileTypeIcon = Utilities::getCachedPixmap(QLatin1String(":/images/drag_generic.png"));
        }
        else
        {
            fileTypeIcon = Utilities::getCachedPixmap(Utilities::getExtensionPixmapName(
                                                          fileInfo.fileName(), QLatin1String(":/images/drag_")));
        }
    }
    else
    {
        if(hasProblem)
        {
            fileTypeIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/folder_error_default.png"));
        }
        else
        {
            fileTypeIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/folder_orange_default.png"));
        }
    }

    return fileTypeIcon;
}

void StalledIssuesUtilities::openLink(bool isCloud, const QString& path)
{
    auto dialog = DialogOpener::findDialog<StalledIssuesDialog>();

    if(isCloud)
    {
        auto url(getLink(isCloud, path));
        if (!url.isEmpty())
        {
            QtConcurrent::run(QDesktopServices::openUrl, QUrl(url));
        }
        else
        {
            QMegaMessageBox::MessageBoxInfo msgInfo;
            msgInfo.parent = dialog ? dialog->getDialog() : nullptr;
            msgInfo.title = QMegaMessageBox::warningTitle();
            msgInfo.text = QString::fromUtf8("Node %1 does not exist.").arg(path);
            QMegaMessageBox::warning(msgInfo);
        }
    }
    else
    {
        QFile file(path);
        if(file.exists())
        {
            QtConcurrent::run([=]
                {
                    Platform::getInstance()->showInFolder(path);
                });
        }
        else
        {
            QMegaMessageBox::MessageBoxInfo msgInfo;
            msgInfo.parent = dialog ? dialog->getDialog() : nullptr;
            msgInfo.title = QMegaMessageBox::warningTitle();
            msgInfo.text =  QString::fromUtf8("Path %1 does not exist.").arg(path);
            QMegaMessageBox::warning(msgInfo);
        }
    }
}

QString StalledIssuesUtilities::getLink(bool isCloud, const QString& path)
{
    QString url;

    if(isCloud)
    {
        std::unique_ptr<mega::MegaNode> node(MegaSyncApp->getMegaApi()->getNodeByPath(path.toUtf8().constData()));
        if(node)
        {
            std::unique_ptr<char[]> handle(node->getBase64Handle());
            url = QString::fromUtf8("mega://#fm/") + QString::fromUtf8(handle.get());
        }
    }

    return url;
}

StalledIssuesUtilities::KeepBothSidesState StalledIssuesUtilities::KeepBothSides(
    std::shared_ptr<mega::MegaNode> node, const QString& localFilePath)
{
    KeepBothSidesState result;

    if(node)
    {
        std::unique_ptr<mega::MegaNode> parentNode(MegaSyncApp->getMegaApi()->getParentNode(node.get()));
        if(parentNode)
        {
            result.newName = Utilities::getNonDuplicatedNodeName(node.get(), parentNode.get(), QString::fromUtf8(node->getName()), true, QStringList());

            result.error = MegaApiSynchronizedRequest::runRequest(&mega::MegaApi::renameNode,
                MegaSyncApp->getMegaApi(),
                node.get(),
                result.newName.toUtf8().constData());

            if(result.error)
            {
                mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Unable to rename file: %1. Error: %2")
                                                                       .arg(QString::fromUtf8(node->getName()), Utilities::getTranslatedError(result.error.get()))
                                                                       .toUtf8().constData());

                QFileInfo currentFile(localFilePath);
                QFile file(currentFile.filePath());
                if(file.exists())
                {
                    result.newName = Utilities::getNonDuplicatedLocalName(currentFile, true, QStringList());
                    currentFile.setFile(currentFile.path(), result.newName);
                    if(file.rename(QDir::toNativeSeparators(currentFile.filePath())))
                    {
                        result.sideRenamed = KeepBothSidesState::Side::LOCAL;

                        //Error no longer needed, at least we have renamed the local file
                        result.error.reset();
                    }
                }
            }
            else
            {
                result.sideRenamed = KeepBothSidesState::Side::REMOTE;
            }
        }
    }

    return result;
}

MegaDownloader* StalledIssuesUtilities::getMegaDownloader()
{
    if (!mDownloader)
    {
        mDownloader = std::make_unique<MegaDownloader>(MegaSyncApp->getMegaApi(), nullptr);
    }

    return mDownloader.get();
}

MegaUploader* StalledIssuesUtilities::getMegaUploader()
{
    if (!mUploader)
    {
        mUploader = std::make_unique<MegaUploader>(MegaSyncApp->getMegaApi(), nullptr);
    }

    return mUploader.get();
}

//////////////////////////////////////////////////
QMap<QVariant, mega::MegaHandle> StalledIssuesBySyncFilter::mSyncIdCache = QMap<QVariant, mega::MegaHandle>();
QHash<const mega::MegaSyncStall*, QSet<mega::MegaHandle>> StalledIssuesBySyncFilter::mSyncIdCacheByStall = QHash<const mega::MegaSyncStall*, QSet<mega::MegaHandle>>();

void StalledIssuesBySyncFilter::resetFilter()
{
    mSyncIdCache.clear();
    mSyncIdCacheByStall.clear();
}

QSet<mega::MegaHandle> StalledIssuesBySyncFilter::getSyncIdsByStall(const mega::MegaSyncStall* stall)
{
    if(!mSyncIdCacheByStall.contains(stall))
    {
        QSet<mega::MegaHandle> syncIds;

        auto addToSet = [&syncIds](mega::MegaHandle value){
            if(value != mega::INVALID_HANDLE)
            {
                syncIds.insert(value);
            }
        };


        auto localSourcePath = QString::fromUtf8(stall->path(false, 0));
        addToSet(filterByPath(localSourcePath, false));
        auto localTargetPath = QString::fromUtf8(stall->path(false, 1));
        addToSet(filterByPath(localTargetPath, false));

        auto cloudSourcePath = QString::fromUtf8(stall->path(true, 0));
        addToSet(filterByPath(cloudSourcePath, true));
        auto cloudTargetPath = QString::fromUtf8(stall->path(true, 1));
        addToSet(filterByPath(cloudTargetPath, true));

        mSyncIdCacheByStall.insert(stall, syncIds);
    }

    return mSyncIdCacheByStall.value(stall);
}

mega::MegaHandle StalledIssuesBySyncFilter::filterByPath(const QString& path, bool cloud)
{
    QVariant key;

    //Cache in case we already checked an issue with the same path
    if(cloud)
    {
        std::unique_ptr<mega::MegaNode> remoteNode(MegaSyncApp->getMegaApi()->getNodeByPath(path.toUtf8().constData()));
        if(remoteNode)
        {
            key = QVariant::fromValue(remoteNode->getParentHandle());

            if(mSyncIdCache.contains(key))
            {
                return mSyncIdCache.value(key);
            }
        }
    }
    else
    {
        QFileInfo fileDir(path);
        QFileInfo folderDir(fileDir.absolutePath());
        if(!folderDir.exists())
        {
            return mega::INVALID_HANDLE;
        }

        key = fileDir.path();
        if(mSyncIdCache.contains(key))
        {
            return mSyncIdCache.value(key);
        }
    }

    std::unique_ptr<mega::MegaSyncList> syncList(MegaSyncApp->getMegaApi()->getSyncs());
    for (int i = 0; i < syncList->size(); ++i)
    {
        auto syncId(syncList->get(i)->getBackupId());
        auto syncSetting = SyncInfo::instance()->getSyncSettingByTag(syncId);

        if(syncSetting)
        {
            if(cloud)
            {
                auto remoteFolder(syncSetting->getMegaFolder());
                auto commonPath = Utilities::getCommonPath(path, remoteFolder, cloud);
                if(commonPath == remoteFolder)
                {
                    mSyncIdCache.insert(key, syncId);
                    return syncId;
                }
            }
            else
            {
                auto localFolder(syncSetting->getLocalFolder());
                auto commonPath = Utilities::getCommonPath(path, localFolder, cloud);
                if(commonPath == localFolder)
                {
                    mSyncIdCache.insert(key, syncId);
                    return syncId;
                }
            }
        }
    }


    return mega::INVALID_HANDLE;
}

bool StalledIssuesBySyncFilter::isBelow(mega::MegaHandle syncRootNode, mega::MegaHandle checkNode)
{
    if(syncRootNode == checkNode)
    {
        return true;
    }

    std::unique_ptr<mega::MegaNode> parentNode(MegaSyncApp->getMegaApi()->getNodeByHandle(checkNode));
    if(!parentNode)
    {
        return false;
    }

    return isBelow(syncRootNode, parentNode->getParentHandle());
}

bool StalledIssuesBySyncFilter::isBelow(const QString &syncRootPath, const QString &checkPath)
{
    if(syncRootPath == checkPath)
    {
        return true;
    }

    QDir fileDir(checkPath);
    //Get parent folder
    fileDir.cdUp();
    return isBelow(syncRootPath, fileDir.path());
}

/////////////////////////////////////////////////////
/// \brief FingerprintMissingSolver::FingerprintMissingSolver
FingerprintMissingSolver::FingerprintMissingSolver() {}

void FingerprintMissingSolver::solveIssues(const QList<StalledIssueVariant> &pathsToSolve)
{
    QDir dir(Preferences::instance()->getTempTransfersPath());
    if (dir.exists() ||
        dir.mkpath(QString::fromUtf8(".")))
    {
        auto tempPath(Preferences::instance()->getTempTransfersPath());

        QMap<QString, std::shared_ptr<QQueue<WrappedNode*>>> nodesToDownloadByPath;
        auto appendNodeToQueue = [&](const QString& targetPath, mega::MegaNode* node)
        {
            if(!nodesToDownloadByPath.contains(targetPath))
            {
                auto queue(std::make_shared<QQueue<WrappedNode*>>());
                queue->append(new WrappedNode(WrappedNode::TransferOrigin::FROM_APP, node));
                nodesToDownloadByPath.insert(targetPath, queue);
            }
            else
            {
                auto queue = nodesToDownloadByPath.value(targetPath);
                if(queue)
                {
                    queue->append(new WrappedNode(WrappedNode::TransferOrigin::FROM_APP, node));
                }
            }
        };

        foreach(auto issue, pathsToSolve)
        {
            std::shared_ptr<DownloadTransferInfo> info(new DownloadTransferInfo());
            if(!issue.consultData()->isBeingSolvedByDownload(info))
            {
                mega::MegaNode* node(MegaSyncApp->getMegaApi()->getNodeByHandle(info->nodeHandle));

                auto localPath = issue.consultData()->consultLocalData() ?
                            issue.consultData()->consultLocalData()->getNativeFilePath() :
                            QString();
                if(!localPath.isEmpty())
                {
                    QFile localPathFile(localPath);
                    if(!localPathFile.exists())
                    {
                        QFileInfo  localFilePathInfo(localPath);
                        QString localFolderPath(localFilePathInfo.absolutePath());
                        QDir createTarget;
                        createTarget.mkdir(localFolderPath);
                        QDir localFolderPathDir(localFolderPath);
                        if(localFolderPathDir.exists())
                        {
                            appendNodeToQueue(localFolderPath, node);
                            continue;
                        }
                    }
                }

                if(node)
                {
                     appendNodeToQueue(tempPath, node);
                }
            }
        }

        foreach(auto targetFolder, nodesToDownloadByPath.keys())
        {
            std::shared_ptr<QQueue<WrappedNode*>> nodesToDownload(nodesToDownloadByPath.value(targetFolder));
            StalledIssuesUtilities::getMegaDownloader()->processTempDownloadQueue(
                nodesToDownload.get(),
                targetFolder);
            qDeleteAll(*nodesToDownload.get());
        }
    }
}
