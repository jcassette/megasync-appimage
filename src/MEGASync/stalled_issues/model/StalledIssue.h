#ifndef STALLEDISSUE_H
#define STALLEDISSUE_H

#include <FileFolderAttributes.h>

#include <megaapi.h>

#include <QSharedData>
#include <QObject>
#include <QFileInfo>
#include <QSize>
#include <QDebug>

#include <QFileSystemWatcher>

#include <memory>

enum class StalledIssueFilterCriterion
{
    ALL_ISSUES = 0,
    NAME_CONFLICTS,
    ITEM_TYPE_CONFLICTS,
    OTHER_CONFLICTS,
    FAILED_CONFLICTS,
    SOLVED_CONFLICTS
};

class StalledIssueData : public QSharedData
{
public:
    struct Path
    {
        QString path;
        mega::MegaSyncStall::SyncPathProblem pathProblem = mega::MegaSyncStall::SyncPathProblem::NoProblem;
        bool showDirectoryInHyperLink = false;

        Path(){}
        bool isEmpty() const {return path.isEmpty() && pathProblem == mega::MegaSyncStall::SyncPathProblem::NoProblem;}
    };

    StalledIssueData();
    virtual ~StalledIssueData() = default;

    const Path& getPath() const;
    const Path& getMovePath() const;
    virtual bool isCloud() const {return false;}

    virtual bool isFile() const {return false; }

    QString getFilePath() const;
    QString getMoveFilePath() const;

    QString getNativeFilePath() const;
    QString getNativeMoveFilePath() const;

    QString getNativePath() const;
    QString getNativeMovePath() const;

    QString getFileName() const;
    QString getMoveFileName() const;

    const std::shared_ptr<const FileFolderAttributes> getAttributes() const {return mAttributes;}
    std::shared_ptr<FileFolderAttributes> getAttributes() {return mAttributes;}

    void checkTrailingSpaces(QString& name) const;

    template <class Type>
    QExplicitlySharedDataPointer<const Type> convert() const
    {
        return QExplicitlySharedDataPointer<const Type>(dynamic_cast<const Type*>(this));
    }

    virtual void initFileFolderAttributes()
    {}

    void setRenamedFileName(const QString& newRenamedFileName)
    {
        mRenamedFileName = newRenamedFileName;
    }

    const QString& renamedFileName() const
    {
        return mRenamedFileName;
    }

protected:
    friend class StalledIssue;
    friend class NameConflictedStalledIssue;
    friend class MoveOrRenameCannotOccurIssue;
    friend class FolderMatchedAgainstFileIssue;

    Path mMovePath;
    Path mPath;

    std::shared_ptr<FileFolderAttributes> mAttributes;

private:
    QString calculateFileName(const QString& path) const;

    QString mRenamedFileName;
};

Q_DECLARE_TYPEINFO(StalledIssueData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(StalledIssueData)

using StalledIssueDataPtr = QExplicitlySharedDataPointer<const StalledIssueData>;
using StalledIssuesDataList = QList<StalledIssueDataPtr>;

Q_DECLARE_METATYPE(StalledIssueDataPtr)
Q_DECLARE_METATYPE(StalledIssuesDataList)

//CLOUD DATA
class CloudStalledIssueData : public StalledIssueData
{
public:
    CloudStalledIssueData()
        : StalledIssueData(),
          mPathHandle(mega::INVALID_HANDLE),
          mMovePathHandle(mega::INVALID_HANDLE)
    {}

    ~CloudStalledIssueData(){}

    bool isCloud() const override
    {
        return true;
    }

    bool isFile() const override
    {
        auto node(getNode());
        if(node)
        {
            return node->isFile();
        }

        return StalledIssueData::isFile();
    }

    std::shared_ptr<mega::MegaNode> getNode(bool refresh = false) const;

    void initFileFolderAttributes() override
    {
        if(mPathHandle != mega::INVALID_HANDLE)
        {
            mAttributes = std::make_shared<RemoteFileFolderAttributes>(mPathHandle, nullptr, false);
        }
        else
        {
            mAttributes = std::make_shared<RemoteFileFolderAttributes>(mPath.path, nullptr, false);
        }
    }

    std::shared_ptr<RemoteFileFolderAttributes> getFileFolderAttributes() const
    {
        return std::dynamic_pointer_cast<RemoteFileFolderAttributes>(mAttributes);
    }

    mega::MegaHandle getPathHandle() const;
    mega::MegaHandle getMovePathHandle() const;

    void setPathHandle(mega::MegaHandle newPathHandle);

private:
    friend class StalledIssue;
    friend class NameConflictedStalledIssue;
    friend class MoveOrRenameCannotOccurIssue;

    mutable std::shared_ptr<mega::MegaNode> mRemoteNode;

    mega::MegaHandle mPathHandle;
    mega::MegaHandle mMovePathHandle;
};

Q_DECLARE_TYPEINFO(CloudStalledIssueData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(CloudStalledIssueData)

using CloudStalledIssueDataPtr = QExplicitlySharedDataPointer<const CloudStalledIssueData>;
using CloudStalledIssuesDataList = QList<CloudStalledIssueDataPtr>;

Q_DECLARE_METATYPE(CloudStalledIssueDataPtr)
Q_DECLARE_METATYPE(CloudStalledIssuesDataList)

//LOCAL DATA
class LocalStalledIssueData : public StalledIssueData
{
public:

    LocalStalledIssueData()
        : StalledIssueData()
    {

    }
    ~LocalStalledIssueData(){}

    bool isCloud() const override
    {
        return false;
    }

    bool isFile() const override
    {
        QFileInfo info(mPath.path);
        if(info.exists())
        {
            return info.isFile();
        }

        return StalledIssueData::isFile();
    }

    void initFileFolderAttributes() override
    {
        mAttributes = std::make_shared<LocalFileFolderAttributes>(mPath.path, nullptr);
    }

    std::shared_ptr<LocalFileFolderAttributes> getFileFolderAttributes() const
    {
        return std::dynamic_pointer_cast<LocalFileFolderAttributes>(mAttributes);
    }
};

Q_DECLARE_TYPEINFO(LocalStalledIssueData, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(LocalStalledIssueData)

using LocalStalledIssueDataPtr = QExplicitlySharedDataPointer<const LocalStalledIssueData>;
using LocalStalledIssueDataList = QList<LocalStalledIssueDataPtr>;

Q_DECLARE_METATYPE(LocalStalledIssueDataPtr)
Q_DECLARE_METATYPE(LocalStalledIssueDataList)

struct UploadTransferInfo;
struct DownloadTransferInfo;

class MultiStepIssueSolverBase;

class StalledIssue : public QObject
{
    Q_OBJECT
    class FileSystemSignalHandler : public QObject
    {
    public:
        FileSystemSignalHandler(StalledIssue* issue)
            :mIssue(issue)
        {}

        ~FileSystemSignalHandler()
        {}

        void createFileWatcher()
        {
            if(!mFileWatcher && !mIssue->getLocalFiles().isEmpty())
            {
                auto deleter = [](QFileSystemWatcher* object){
                    object->deleteLater();
                };

                auto onChanged = [this](){
                    mIssue->resetUIUpdated();
#ifdef Q_OS_LINUX
                    auto paths = mIssue->getLocalFiles();
                    foreach(auto& path, paths)
                    {
                        if (QFile::exists(path))
                        {
                            mFileWatcher->addPath(path);
                        }
                    }
#endif
                };

                mFileWatcher = std::shared_ptr<QFileSystemWatcher>(new QFileSystemWatcher(mIssue->getLocalFiles()), deleter);
                connect(mFileWatcher.get(), &QFileSystemWatcher::directoryChanged, this, [onChanged](const QString&){
                    onChanged();
                });

                connect(mFileWatcher.get(), &QFileSystemWatcher::fileChanged, this, [onChanged](const QString&){
                    onChanged();
                });
            }
        }

        void removeFileWatcher()
        {
            mFileWatcher.reset();
        }

    private:
        StalledIssue* mIssue;
        std::shared_ptr<QFileSystemWatcher> mFileWatcher;
    };

public:
    StalledIssue(const mega::MegaSyncStall* stallIssue);
    virtual ~StalledIssue(){}

    virtual bool isValid() const{ return true;}

    const LocalStalledIssueDataPtr consultLocalData() const;
    const CloudStalledIssueDataPtr consultCloudData() const;

    const QExplicitlySharedDataPointer<LocalStalledIssueData>& getLocalData();
    const QExplicitlySharedDataPointer<CloudStalledIssueData>& getCloudData();

    virtual bool containsHandle(mega::MegaHandle handle){return getCloudData() && getCloudData()->getPathHandle() == handle;}
    virtual void updateHandle(mega::MegaHandle handle){if(getCloudData()){getCloudData()->setPathHandle(handle);}}
    virtual void updateName(){}

    virtual bool checkForExternalChanges();

    mega::MegaSyncStall::SyncStallReason getReason() const;
    QString getFileName(bool preferCloud) const;
    static StalledIssueFilterCriterion getCriterionByReason(mega::MegaSyncStall::SyncStallReason reason);

    int getPathProblem() const;

    bool operator==(const StalledIssue &data);

    virtual void updateIssue(const mega::MegaSyncStall* stallIssue);

    enum SolveType
    {
        UNSOLVED,
        FAILED,
        BEING_SOLVED,
        POTENTIALLY_SOLVED,
        SOLVED
    };

    bool isUnsolved() const;
    bool isSolved() const;
    bool isPotentiallySolved() const;
    bool isBeingSolved() const;
    bool isFailed() const;

    SolveType getIsSolved() const {return mIsSolved;}
    virtual void setIsSolved(SolveType type);

    enum class AutoSolveIssueResult
    {
        SOLVED,
        ASYNC_SOLVED,
        FAILED,
    };

    virtual AutoSolveIssueResult autoSolveIssue()
    {
        return AutoSolveIssueResult::FAILED;
    }
    virtual bool isAutoSolvable() const;
    bool isBeingSolvedByUpload(std::shared_ptr<UploadTransferInfo> info, bool isSourcePath) const;
    bool isBeingSolvedByDownload(std::shared_ptr<DownloadTransferInfo> info) const;

    virtual void finishAsyncIssueSolving(){}
    virtual void startAsyncIssueSolving();

    bool missingFingerprint() const;
    static bool isCloudNodeBlocked(const mega::MegaSyncStall* stall);
    virtual QStringList getLocalFiles();

    bool isFile() const;
    uint8_t filesCount() const;
    uint8_t foldersCount() const;

    enum Type
    {
        Header = 0,
        Body
    };

    QSize getDelegateSize(Type type) const;
    void setDelegateSize(const QSize& newDelegateSize, Type type);
    void removeDelegateSize(Type type);
    void resetDelegateSize();

    const std::shared_ptr<mega::MegaSyncStall>& getOriginalStall() const;

    virtual void fillIssue(const mega::MegaSyncStall* stall);
    void fillBasicInfo(const mega::MegaSyncStall* stall);
    //In order to show the filepath or the directory path when the path is used for a hyperlink
    virtual bool showDirectoryInHyperlink() const {return false;}

    virtual void endFillingIssue();

    template <class Type>
    static const std::shared_ptr<const Type> convert(const std::shared_ptr<const StalledIssue> data)
    {
        return std::dynamic_pointer_cast<const Type>(data);
    }

    bool needsUIUpdate(Type type) const;
    void UIUpdated(Type type);
    void resetUIUpdated();
    virtual bool UIShowFileAttributes() const;
    void createFileWatcher();
    void removeFileWatcher();

    mega::MegaHandle firstSyncId() const;
    const QSet<mega::MegaHandle>& syncIds() const;
    //In case there are two syncs, use the first one
    mega::MegaSync::SyncType getSyncType() const;

    virtual bool shouldBeIgnored() const {return false;}

    bool wasAutoResolutionApplied() const;
    void setAutoResolutionApplied(bool newAutoResolutionApplied);

    virtual bool isExpandable() const;

    bool detectedCloudSide() const;

signals:
    void asyncIssueSolvingStarted();
    void asyncIssueSolvingFinished(StalledIssue*);
    void dataUpdated(StalledIssue*);

protected:
    bool initLocalIssue();
    void fillSourceLocalPath(const mega::MegaSyncStall* stall);
    void fillTargetLocalPath(const mega::MegaSyncStall* stall);
    QExplicitlySharedDataPointer<LocalStalledIssueData> mLocalData;

    bool initCloudIssue();
    void fillSourceCloudPath(const mega::MegaSyncStall* stall);
    void fillTargetCloudPath(const mega::MegaSyncStall* stall);
    QExplicitlySharedDataPointer<CloudStalledIssueData> mCloudData;

    void setIsFile(const QString& path, bool isLocal);

    void performFinishAsyncIssueSolving(bool hasFailed);

    std::shared_ptr<mega::MegaSyncStall> originalStall;
    mega::MegaSyncStall::SyncStallReason mReason = mega::MegaSyncStall::SyncStallReason::NoReason;
    QSet<mega::MegaHandle> mSyncIds;
    mutable SolveType mIsSolved = SolveType::UNSOLVED;
    uint8_t mFiles = 0;
    uint8_t mFolders = 0;

    bool mDetectedCloudSide;
    QSize mHeaderDelegateSize;
    QSize mBodyDelegateSize;
    QPair<bool, bool> mNeedsUIUpdate = qMakePair(false, false);
    std::shared_ptr<FileSystemSignalHandler> mFileSystemWatcher;
    bool mAutoResolutionApplied;
};

using StalledIssueSPtr = std::shared_ptr<StalledIssue>;
using StalledIssuesList = QList<StalledIssueSPtr>;

class StalledIssueVariant
{
public:
    StalledIssueVariant(){}
    StalledIssueVariant(const StalledIssueVariant& tdr) : mData(tdr.mData) {}
    StalledIssueVariant(std::shared_ptr<StalledIssue> data, const mega::MegaSyncStall* stall = nullptr)
        : mData(data)
    {
        if(stall)
        {
            mData->fillIssue(stall);
        }
    }

    const std::shared_ptr<const StalledIssue> consultData() const
    {
        return mData;
    }
    bool isValid() const
    {
        return mData != nullptr;
    }

    void updateData(const mega::MegaSyncStall* stallIssue)
    {
        mData->updateIssue(stallIssue);
    }

    void reset()
    {
        mData.reset();
    }

    bool operator==(const StalledIssueVariant& issue)
    {
        return issue.mData == this->mData;
    }

    StalledIssueVariant& operator=(const StalledIssueVariant& other) = default;

    QSize getDelegateSize(StalledIssue::Type type) const
    {
        if(mData)
        {
            return mData->getDelegateSize(type);
        }
        return QSize();
    }
    void setDelegateSize(const QSize &newDelegateSize, StalledIssue::Type type)
    {
        mData->setDelegateSize(newDelegateSize, type);
    }
    void removeDelegateSize(StalledIssue::Type type)
    {
        mData->removeDelegateSize(type);
    }

    bool shouldBeIgnored() const
    {
        return mData->shouldBeIgnored();
    }

    template <class Type>
    const std::shared_ptr<const Type> convert() const
    {
        return StalledIssue::convert<Type>(mData);
    }

private:
    friend class StalledIssuesModel;
    friend class StalledIssuesCreator;
    friend class MoveOrRenameCannotOccurFactory;

    StalledIssueSPtr& getData()
    {
        return mData;
    }

    template <class Type>
    std::shared_ptr<Type> convert()
    {
        return std::dynamic_pointer_cast<Type>(mData);
    }

    StalledIssueSPtr mData = nullptr;
};

Q_DECLARE_METATYPE(StalledIssueVariant)

using StalledIssuesVariantList = QList<StalledIssueVariant>;
Q_DECLARE_METATYPE(StalledIssuesVariantList)

#endif // STALLEDISSUE_H
