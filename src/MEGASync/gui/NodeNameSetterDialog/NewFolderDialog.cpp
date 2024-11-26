#include "NewFolderDialog.h"

#include <Utilities.h>
#include <MegaApplication.h>
#include <mega/types.h>

///NEW FOLDER REIMPLEMENTATION
NewFolderDialog::NewFolderDialog(std::shared_ptr<mega::MegaNode> parentNode, QWidget *parent)
    :NodeNameSetterDialog(parent)
    , mParentNode(parentNode)
{
}

void NewFolderDialog::onDialogAccepted()
{
    QString newFolderName = getName();

    if(!checkAlreadyExistingNode(newFolderName, mParentNode))
    {
        setEnabled(false);
        MegaSyncApp->getMegaApi()->createFolder(newFolderName.toUtf8().constData(), mParentNode.get(), mDelegateListener.get());
    }
}

void NewFolderDialog::onRequestFinish(mega::MegaApi *, mega::MegaRequest *request, mega::MegaError *e)
{
    if(request->getType() == mega::MegaRequest::TYPE_CREATE_FOLDER)
    {
        int result(QDialog::Rejected);

        if (e->getErrorCode() == mega::MegaError::API_OK)
        {
            mNewNode = std::unique_ptr<mega::MegaNode>(MegaSyncApp->getMegaApi()->getNodeByHandle(request->getNodeHandle()));
            if (mNewNode)
            {
                result = QDialog::Accepted;
            }
        }

        done(result);
    }
}

QString NewFolderDialog::dialogText()
{
    return tr("Enter the new folder name");
}

void NewFolderDialog::title()
{
    setWindowTitle(tr("New folder"));
}

std::unique_ptr<mega::MegaNode> NewFolderDialog::getNewNode()
{
    return std::move(mNewNode);
}
