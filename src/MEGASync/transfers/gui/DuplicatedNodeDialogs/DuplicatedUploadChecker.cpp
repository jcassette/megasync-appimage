#include "DuplicatedUploadChecker.h"

#include "DuplicatedNodeDialog.h"
#include "Preferences.h"

#include <MegaApplication.h>

#include <QDir>
#include <QFileInfo>

///BASE
///
void DuplicatedUploadBase::onNodeItemSelected()
{
    if(auto nodeItem = dynamic_cast<DuplicatedNodeItem*>(sender()))
    {
        mUploadInfo->setSolution(nodeItem->getType());
        emit selectionDone();
    }
}

QStringList &DuplicatedUploadBase::getCheckedNames()
{
    return checkedNames;
}

QString DuplicatedUploadBase::getHeader(std::shared_ptr<DuplicatedNodeInfo> conflict)
{
    return conflict->isRemoteFile() ? DuplicatedNodeDialog::tr("A file named [A] already exists at this destination")
           : DuplicatedNodeDialog::tr("A folder named [A] already exists at this destination");
}

QString DuplicatedUploadBase::getSkipText(bool isFile)
{
    return isFile ? DuplicatedNodeDialog::tr("The file at this destination will be maintained.")
           : DuplicatedNodeDialog::tr("The folder at this destination will be maintained.");
}

////FILE
void DuplicatedUploadFile::fillUi(DuplicatedNodeDialog *dialog, std::shared_ptr<DuplicatedNodeInfo> conflict)
{
    mUploadInfo = conflict;

    dialog->setHeader(getHeader(conflict),QString::fromUtf8(conflict->getRemoteConflictNode()->getName()));

    auto fileVersioningDisabled(Preferences::instance()->fileVersioningDisabled());

    if(!conflict->haveDifferentType() && !conflict->isNameConflict())
    {
        DuplicatedLocalItem* uploadItem = new DuplicatedLocalItem(dialog);

        if(fileVersioningDisabled)
        {
            uploadItem->setInfo(conflict, NodeItemType::FILE_UPLOAD_AND_REPLACE);
            QString description = DuplicatedNodeDialog::tr("The file at this destination will be replaced with the new file.");
            uploadItem->setDescription(description);
        }
        else
        {
            uploadItem->setInfo(conflict, NodeItemType::FILE_UPLOAD_AND_UPDATE);
            uploadItem->setDescription(DuplicatedNodeDialog::tr("The file at this destination will be updated if the new file is different."));
            uploadItem->showLearnMore(QLatin1String("https://help.mega.io/files-folders/restore-delete/file-version-history"));
        }

        connect(uploadItem, &DuplicatedNodeItem::actionClicked, this, &DuplicatedUploadFile::onNodeItemSelected);
        dialog->addNodeItem(uploadItem);
    }

    //Upload and merge item
    DuplicatedRenameItem* uploadAndRename = new DuplicatedRenameItem(dialog);
    uploadAndRename->setRenameInfo(conflict);
    uploadAndRename->setDescription(DuplicatedNodeDialog::tr("The file will be renamed as:"));
    connect(uploadAndRename, &DuplicatedNodeItem::actionClicked, this, &DuplicatedUploadFile::onNodeItemSelected);
    dialog->addNodeItem(uploadAndRename);

    DuplicatedRemoteItem* dontUploadItem = new DuplicatedRemoteItem(dialog);
    dontUploadItem->setInfo(conflict, NodeItemType::DONT_UPLOAD);
    dontUploadItem->setDescription(getSkipText(conflict->isRemoteFile()));
    connect(dontUploadItem, &DuplicatedNodeItem::actionClicked, this, &DuplicatedUploadFile::onNodeItemSelected);
    dialog->addNodeItem(dontUploadItem);
}

///FOLDER
void DuplicatedUploadFolder::fillUi(DuplicatedNodeDialog* dialog, std::shared_ptr<DuplicatedNodeInfo> conflict)
{
    mUploadInfo = conflict;

    dialog->setHeader(getHeader(conflict),QString::fromUtf8(conflict->getRemoteConflictNode()->getName()));

    if(!conflict->haveDifferentType())
    {
        //Upload and merge item
        DuplicatedLocalItem* uploadAndMergeItem = new DuplicatedLocalItem(dialog);
        uploadAndMergeItem->setInfo(conflict, NodeItemType::FOLDER_UPLOAD_AND_MERGE);
        uploadAndMergeItem->setDescription(DuplicatedNodeDialog::tr("The new folder will be merged with the folder at this destination."));
        connect(uploadAndMergeItem, &DuplicatedNodeItem::actionClicked, this, &DuplicatedUploadFolder::onUploadAndMergeSelected);
        dialog->addNodeItem(uploadAndMergeItem);
    }

    DuplicatedRemoteItem* dontUploadItem = new DuplicatedRemoteItem(dialog);
    dontUploadItem->setInfo(conflict, NodeItemType::DONT_UPLOAD);
    dontUploadItem->setDescription(getSkipText(conflict->isRemoteFile()));
    connect(dontUploadItem, &DuplicatedNodeItem::actionClicked, this, &DuplicatedUploadFolder::onNodeItemSelected);
    dialog->addNodeItem(dontUploadItem);
}

void DuplicatedUploadFolder::onUploadAndMergeSelected()
{
    //This is done to merge the local folder into the remote one (we need to upload it with the remote name)
    if(mUploadInfo->isNameConflict())
    {
        mUploadInfo->setNewName(QString::fromUtf8(mUploadInfo->getRemoteConflictNode()->getName()));
    }

    DuplicatedUploadFolder::onNodeItemSelected();
}
