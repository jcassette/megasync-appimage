#include "StalledIssueFilePath.h"
#include "ui_StalledIssueFilePath.h"

#include "Utilities.h"
#include "Platform.h"
#include "QMegaMessageBox.h"
#include <DialogOpener.h>
#include <StalledIssuesDialog.h>

#include <QPainter>
#include <QPoint>

static const char* ITS_HOVER = "ItsHover";
static const char* HAS_PROBLEM = "hasProblem";

StalledIssueFilePath::StalledIssueFilePath(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StalledIssueFilePath),
    mShowFullPath(false)
{
    ui->setupUi(this);

    mOpenIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/ic-open-outside.png"));

    connect(ui->helpIcon, &QPushButton::clicked, this, &StalledIssueFilePath::onHelpIconClicked);
}

StalledIssueFilePath::~StalledIssueFilePath()
{
    delete ui;
}

void StalledIssueFilePath::setIndent(int indent)
{
    ui->indent->changeSize(indent,0,QSizePolicy::Fixed, QSizePolicy::Preferred);
    ui->gridLayout->invalidate();
}

void StalledIssueFilePath::updateUi(StalledIssueDataPtr newData)
{
    if(!mData && newData)
    {
        ui->lines->installEventFilter(this);
        ui->moveLines->installEventFilter(this);
        ui->movePathProblemLines->installEventFilter(this);

        ui->filePathContainer->installEventFilter(this);
        ui->moveFilePathContainer->installEventFilter(this);
    }
    else if(!newData)
    {
        ui->lines->removeEventFilter(this);
        ui->moveLines->removeEventFilter(this);
        ui->movePathProblemLines->removeEventFilter(this);

        ui->filePathContainer->removeEventFilter(this);
        ui->moveFilePathContainer->removeEventFilter(this);
    }

    mData = newData;

    if(mData->isCloud())
    {
        auto remoteIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/cloud_default.png"));
        ui->LocalOrRemoteIcon->setPixmap(remoteIcon.pixmap(QSize(16,16)));

        ui->LocalOrRemoteText->setText(tr("on MEGA:"));
    }
    else
    {
        auto localIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/monitor_default.png"));
        ui->LocalOrRemoteIcon->setPixmap(localIcon.pixmap(QSize(16,16)));

        ui->LocalOrRemoteText->setText(tr("Local:"));
    }

    updateFileIcons();
    updateMoveFileIcons();
    fillFilePath();
    fillMoveFilePath();
}

void StalledIssueFilePath::showFullPath()
{
    mShowFullPath = true;
}

void StalledIssueFilePath::hideLocalOrRemoteTitle()
{
    ui->LocalOrRemoteIcon->hide();
    ui->LocalOrRemoteText->hide();
}

void StalledIssueFilePath::fillFilePath()
{
    if(!mData->getPath().isEmpty())
    {
        ui->file->show();
        const auto pathProblem(mData->getPath().pathProblem);

        auto hasProblem(pathProblem != mega::MegaSyncStall::SyncPathProblem::NoProblem && showError(pathProblem));

        if(hasProblem)
        {
            ui->pathProblemMessage->setText(getSyncPathProblemString(pathProblem));
            ui->filePathContainer->setCursor(Qt::ArrowCursor);
            auto helpLink = getHelpLink(pathProblem);
            helpLink.isEmpty() ? ui->helpIcon->hide() : ui->helpIcon->show();
        }
        else
        {
            ui->pathProblemContainer->hide();
            ui->filePathContainer->setCursor(Qt::PointingHandCursor);
            ui->helpIcon->hide();
        }

        ui->filePathContainer->setProperty(HAS_PROBLEM,hasProblem);
        setStyleSheet(styleSheet());

        auto filePath = getFilePath();
        if(!filePath.isEmpty())
        {
            ui->filePathEdit->setText(filePath);
        }
        else
        {
            ui->filePathEdit->setText(QString::fromUtf8("-"));
        }
    }
    else
    {
        ui->file->hide();
    }
}

QString StalledIssueFilePath::getFilePath() const
{
    QString filePath;
    if(mData)
    {
        QFileInfo fileInfo(mShowFullPath? mData->getNativeFilePath() : mData->getNativePath());
        filePath = mData->getPath().showDirectoryInHyperLink ? fileInfo.path() : fileInfo.filePath();
        mData->checkTrailingSpaces(filePath);
    }
    return filePath;
}

void StalledIssueFilePath::fillMoveFilePath()
{
    if(!mData->getMovePath().isEmpty())
    {
        auto hasProblem(mData->getMovePath().pathProblem != mega::MegaSyncStall::SyncPathProblem::NoProblem);
        hasProblem ?  ui->movePathProblemMessage->setText(getSyncPathProblemString(mData->getMovePath().pathProblem)) : ui->movePathProblemContainer->hide();
        hasProblem ? ui->moveFilePathContainer->setCursor(Qt::ArrowCursor) : ui->moveFilePathContainer->setCursor(Qt::PointingHandCursor);
        ui->moveFilePathContainer->setProperty(HAS_PROBLEM,hasProblem);
        setStyleSheet(styleSheet());

        ui->moveFile->show();

        auto filePath = getMoveFilePath();
        if(!filePath.isEmpty())
        {
            ui->moveFilePathEdit->setText(filePath);
        }
        else
        {
            ui->moveFilePathEdit->setText(QString::fromUtf8("-"));
        }
    }
    else
    {
        ui->moveFile->hide();
    }
}

QString StalledIssueFilePath::getMoveFilePath() const
{
    QFileInfo fileInfo(mShowFullPath? mData->getNativeMoveFilePath() : mData->getNativeMovePath());
    return mData->getMovePath().showDirectoryInHyperLink ? fileInfo.path() : fileInfo.filePath();
}

std::unique_ptr<mega::MegaNode> StalledIssueFilePath::getNode() const
{
    if(auto cloudIssue = mData->convert<CloudStalledIssueData>())
    {
        if(cloudIssue->getPathHandle() != mega::INVALID_HANDLE)
        {
            std::unique_ptr<mega::MegaNode> node(MegaSyncApp->getMegaApi()->getNodeByHandle(cloudIssue->getPathHandle()));
            if(node)
            {
                if(mShowFullPath)
                {
                    return node;
                }
                else
                {
                    std::unique_ptr<mega::MegaNode> parentNode(MegaSyncApp->getMegaApi()->getNodeByHandle(node->getParentHandle()));
                    return parentNode;
                }
            }
        }

        QFileInfo fileInfo(getFilePath());
        std::unique_ptr<mega::MegaNode> node(MegaSyncApp->getMegaApi()->getNodeByPath(fileInfo.filePath().toUtf8().constData()));
        return node;
    }

    return nullptr;
}

std::unique_ptr<mega::MegaNode> StalledIssueFilePath::getMoveNode() const
{
    if(auto cloudIssue = mData->convert<CloudStalledIssueData>())
    {
        if(cloudIssue->getPathHandle() != mega::INVALID_HANDLE)
        {
            std::unique_ptr<mega::MegaNode> node(MegaSyncApp->getMegaApi()->getNodeByHandle(cloudIssue->getMovePathHandle()));
            if(node)
            {
                if(mShowFullPath)
                {
                    return node;
                }
                else
                {
                    std::unique_ptr<mega::MegaNode> parentNode(MegaSyncApp->getMegaApi()->getNodeByHandle(node->getParentHandle()));
                    return parentNode;
                }
            }
        }

        QFileInfo fileInfo(getMoveFilePath());
        std::unique_ptr<mega::MegaNode> node(MegaSyncApp->getMegaApi()->getNodeByPath(fileInfo.filePath().toUtf8().constData()));
        return node;
    }

    return nullptr;
}

void StalledIssueFilePath::updateFileIcons()
{
    QIcon fileTypeIcon;
    QSize iconSize(ui->filePathIcon->size());

    QFileInfo fileInfo(getFilePath());
    auto hasProblem(mData->getPath().pathProblem != mega::MegaSyncStall::SyncPathProblem::NoProblem);
    if(mData->isCloud())
    {
        auto node(getNode());
        fileTypeIcon = StalledIssuesUtilities::getRemoteFileIcon(node.get(), fileInfo, hasProblem);
        if(!node)
        {
            iconSize = QSize(16,16);
        }
    }
    else
    {
        fileTypeIcon = StalledIssuesUtilities::getLocalFileIcon(fileInfo, hasProblem);
    }

    ui->filePathIcon->setPixmap(fileTypeIcon.pixmap(iconSize));
}

void StalledIssueFilePath::updateMoveFileIcons()
{
    QIcon fileTypeIcon;
    QSize iconSize(ui->moveFilePathIcon->size());

    QFileInfo fileInfo(getMoveFilePath());
    auto hasProblem(mData->getMovePath().pathProblem != mega::MegaSyncStall::SyncPathProblem::NoProblem);

    if(mData->isCloud())
    {
        auto node(getMoveNode());
        fileTypeIcon = StalledIssuesUtilities::getRemoteFileIcon(node.get(), fileInfo, hasProblem);
        if(!node)
        {
            iconSize = QSize(16,16);
        }
    }
    else
    {
        fileTypeIcon = StalledIssuesUtilities::getLocalFileIcon(fileInfo, hasProblem);
    }

    ui->moveFilePathIcon->setPixmap(fileTypeIcon.pixmap(iconSize));
}

bool StalledIssueFilePath::eventFilter(QObject *watched, QEvent *event)
{
    //Not needed but used as extra protection
    if(mData)
    {
        if(event->type() == QEvent::Enter || event->type() == QEvent::Leave || event->type() == QEvent::MouseButtonRelease)
        {
            if(watched == ui->filePathContainer && !ui->filePathContainer->property(HAS_PROBLEM).toBool())
            {
                showHoverAction(event->type(), ui->filePathAction, getFilePath());
            }
            else if(watched == ui->moveFilePathContainer && !ui->moveFilePathContainer->property(HAS_PROBLEM).toBool())
            {
                showHoverAction(event->type(), ui->moveFilePathAction,  getMoveFilePath());
            }
        }
        else if(event->type() == QEvent::Resize)
        {
            if(watched == ui->lines)
            {
                auto hasProblem(mData->getPath().pathProblem != mega::MegaSyncStall::SyncPathProblem::NoProblem);
                if(hasProblem)
                {
                    auto fileTypeIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/tree_link_end_default.png"));
                    ui->lines->setPixmap(fileTypeIcon.pixmap(ui->lines->size()));
                }
                else
                {
                    auto fileTypeIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/tree_end_default.png"));
                    ui->lines->setPixmap(fileTypeIcon.pixmap(ui->lines->size()));
                }
            }
            else if(watched == ui->movePathProblemLines)
            {
                auto hasProblem(mData->getMovePath().pathProblem != mega::MegaSyncStall::SyncPathProblem::NoProblem);
                if(hasProblem)
                {
                    auto fileTypeIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/tree_double_link_default.png"));
                    //TODO avoid using a fixed size
                    ui->movePathProblemLines->setPixmap(fileTypeIcon.pixmap(QSize(24,24)));
                }
                else
                {
                    ui->movePathProblemLines->setPixmap(QPixmap());
                }
            }
            else if(watched == ui->moveLines)
            {
                auto fileTypeIcon = Utilities::getCachedPixmap(QLatin1String(":/images/StalledIssues/tree_link_default.png"));
                ui->moveLines->setPixmap(fileTypeIcon.pixmap(ui->moveLines->size()));
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void StalledIssueFilePath::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void StalledIssueFilePath::onHelpIconClicked()
{
    auto helpLink = QUrl(getHelpLink(mData->getPath().pathProblem));
    Utilities::openUrl(helpLink);
}

void StalledIssueFilePath::showHoverAction(QEvent::Type type, QLabel *actionWidget, const QString& path)
{
    if(type == QEvent::Enter)
    {
        actionWidget->setPixmap(mOpenIcon.pixmap(actionWidget->size()));
        actionWidget->parent()->setProperty(ITS_HOVER, true);
        setStyleSheet(styleSheet());
    }
    else if(type == QEvent::Leave)
    {
        actionWidget->setPixmap(QPixmap());
        actionWidget->parent()->setProperty(ITS_HOVER, false);
        setStyleSheet(styleSheet());
    }
    else if(type == QEvent::MouseButtonRelease)
    {
        StalledIssuesUtilities::openLink(mData->isCloud(), path);
    }
}

QString StalledIssueFilePath::getSyncPathProblemString(mega::MegaSyncStall::SyncPathProblem pathProblem)
{
    switch(pathProblem)
    {
        case mega::MegaSyncStall::FileChangingFrequently:
        {
            return tr("File is being frequently changing.");
        }
        case mega::MegaSyncStall::IgnoreRulesUnknown:
        {
            return tr("Ignore rules unknown.");
        }
        case mega::MegaSyncStall::DetectedHardLink:
        {
            return tr("Hard link or Reparse Point detected.");
        }
        case mega::MegaSyncStall::DetectedSymlink:
        {
            return tr("Detected Sym link.");
        }
        case mega::MegaSyncStall::DetectedSpecialFile:
        {
            return tr("Detected special file.");
        }
        case mega::MegaSyncStall::DifferentFileOrFolderIsAlreadyPresent:
        {
            return tr("Different file or folder is already present.");
        }
        case mega::MegaSyncStall::ParentFolderDoesNotExist:
        {
            return tr("Parent folder does not exist.");
        }
        case mega::MegaSyncStall::FilesystemErrorDuringOperation:
        {
            return tr("Filesystem error during operation.");
        }
        case mega::MegaSyncStall::NameTooLongForFilesystem:
        {
            return tr("Name too long for filesystem.");
        }
        case mega::MegaSyncStall::CannotFingerprintFile:
        {
            return tr("Cannot fingerprint file.");
        }
        case mega::MegaSyncStall::DestinationPathInUnresolvedArea:
        {
            return tr("The folder could not be found. Ensure that the path is correct and try again.");
        }
        case mega::MegaSyncStall::MACVerificationFailure:
        {
            return tr("MAC verification failure.");
        }
        case mega::MegaSyncStall::DeletedOrMovedByUser:
        {
            return tr("Deleted or moved by user.");
        }
        case mega::MegaSyncStall::FileFolderDeletedByUser:
        {
            return tr("Deleted by user.");
        }
        case mega::MegaSyncStall::MoveToDebrisFolderFailed:
        {
            return tr("Move to debris folder failed.");
        }
        case mega::MegaSyncStall::IgnoreFileMalformed:
        {
            return tr("Ignore file malformed.");
        }
        case mega::MegaSyncStall::FilesystemErrorListingFolder:
        {
            return tr("Error Listing folder in filesystem.");
        }
        case mega::MegaSyncStall::FilesystemErrorIdentifyingFolderContent:
        {
            return tr("Error identifying folder content in filesystem.");
        }
        case mega::MegaSyncStall::WaitingForScanningToComplete:
        {
            return tr("Waiting for scanning to complete.");
        }
        case mega::MegaSyncStall::WaitingForAnotherMoveToComplete:
        {
            return tr("Waiting for another move to complete.");
        }
        case mega::MegaSyncStall::SourceWasMovedElsewhere:
        {
            return tr("Source was moved elsewhere.");
        }
        case mega::MegaSyncStall::FilesystemCannotStoreThisName:
        {
            return tr("Local filesystem cannot store this name.");
        }
        case mega::MegaSyncStall::CloudNodeInvalidFingerprint:
        {
            return tr("Fingerprint is missing or invalid.");
        }
        case mega::MegaSyncStall::SyncPathProblem_LastPlusOne:
        {
            break;
        }
        default:
            break;
    }
    return tr("Error not detected");
}

bool StalledIssueFilePath::showError(mega::MegaSyncStall::SyncPathProblem pathProblem)
{
    switch(pathProblem)
    {
        case mega::MegaSyncStall::CloudNodeIsBlocked:
        {
            return false;
        }
        default:
        {
            return true;
        }
    }
}

QString StalledIssueFilePath::getHelpLink(mega::MegaSyncStall::SyncPathProblem pathProblem)
{
    switch(pathProblem)
    {
        case mega::MegaSyncStall::FilesystemCannotStoreThisName:
        {
            return QLatin1String("https://help.mega.io/");
        }
        default:
            break;
    }

    return QString();
}
