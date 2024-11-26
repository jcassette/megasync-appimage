#include "MergeMEGAFolders.h"

#include <MegaApiSynchronizedRequest.h>
#include <MegaApplication.h>
#include <MoveToMEGABin.h>
#include <Utilities.h>


std::shared_ptr<mega::MegaError> MergeMEGAFolders::merge(ActionForDuplicates action)
{
    std::shared_ptr<mega::MegaError> error;

    if (mFolderTarget && mFolderToMerge)
    {
        error.reset(mergeTwoFolders(action).get());
    }
    else if (mFolderTarget)
    {
        std::unique_ptr<mega::MegaNode> parentNode(
            MegaSyncApp->getMegaApi()->getNodeByHandle(mFolderTarget->getParentHandle()));
        if (parentNode)
        {
            QString targetNodeName(QString::fromUtf8(mFolderTarget->getName()));
            std::unique_ptr<mega::MegaNodeList> folderToMergeNodes(
                MegaSyncApp->getMegaApi()->getChildren(parentNode.get()));
            for (int index = 0; index < folderToMergeNodes->size(); ++index)
            {
                auto node(folderToMergeNodes->get(index));
                QString checkNodeName(QString::fromUtf8(node->getName()));
                if (targetNodeName.compare(checkNodeName, Qt::CaseInsensitive) == 0 &&
                    node->getHandle() != mFolderTarget->getHandle())
                {
                    mFolderToMerge = node;
                    error.reset(mergeTwoFolders(action).get());
                    if (error)
                    {
                        break;
                    }
                }
            }
        }
    }

    return error;
}

std::shared_ptr<mega::MegaError> MergeMEGAFolders::mergeTwoFolders(ActionForDuplicates action)
{
    std::shared_ptr<mega::MegaError> error(nullptr);

    QStringList itemsBeingRenamed;

    std::unique_ptr<mega::MegaNodeList> folderTargetNodes(MegaSyncApp->getMegaApi()->getChildren(mFolderTarget));
    QMultiMap<QString, int> nodesIndexesByName;
    for(int index = 0; index < folderTargetNodes->size(); ++index)
    {
        auto node(folderTargetNodes->get(index));
        nodesIndexesByName.insert(QString::fromUtf8(node->getName()), index);
    }

    std::unique_ptr<mega::MegaNodeList> folderToMergeNodes(MegaSyncApp->getMegaApi()->getChildren(mFolderToMerge));
    for(int index = 0; index < folderToMergeNodes->size(); ++index)
    {
        auto node(folderToMergeNodes->get(index));
        QString nodeName(QString::fromUtf8(node->getName()));
        auto targetIndexes(nodesIndexesByName.values(nodeName));
        if(!targetIndexes.isEmpty())
        {
            bool duplicateFound(false);
            foreach(auto targetIndex, targetIndexes)
            {
                auto targetNode(folderTargetNodes->get(targetIndex));
                if(node->isFile() && targetNode->isFile())
                {
                    auto nodeFp(QString::fromUtf8(node->getFingerprint()));
                    auto targetNodeFp(QString::fromUtf8(targetNode->getFingerprint()));
                    if(nodeFp == targetNodeFp)
                    {
                        duplicateFound = true;
                        break;
                    }
                }
                else if(node->isFolder() && targetNode->isFolder())
                {
                    mDepth++;
                    MergeMEGAFolders folderMerge(targetNode, node);
                    folderMerge.merge(action);
                    mDepth--;
                    std::unique_ptr<mega::MegaNodeList>folderChild(MegaSyncApp->getMegaApi()->getChildren(node));
                    if(action == ActionForDuplicates::IgnoreAndRemove || folderChild->size() == 0)
                    {
                        error = MegaApiSynchronizedRequest::runRequest(&mega::MegaApi::remove, MegaSyncApp->getMegaApi(), node);
                        if(error)
                        {
                            break;
                        }
                    }
                    else
                    {
                        duplicateFound = true;
                    }
                }
            }

            if(!duplicateFound || action == ActionForDuplicates::Rename)
            {
                auto newName = Utilities::getNonDuplicatedNodeName(
                    node, mFolderTarget, nodeName, true, itemsBeingRenamed);

                error = MegaApiSynchronizedRequest::runRequestLambda(
                    [](mega::MegaNode* node,
                       mega::MegaNode* targetNode,
                       const char* newName,
                       mega::MegaRequestListener* listener) {
                        MegaSyncApp->getMegaApi()->moveNode(node, targetNode, newName, listener);
                    },
                    MegaSyncApp->getMegaApi(),
                    node,
                    mFolderTarget,
                    newName.toUtf8().constData());

                if(error)
                {
                    break;
                }
            }
        }
        else
        {
            error = MegaApiSynchronizedRequest::runRequestLambda(
                [](mega::MegaNode* node,
                   mega::MegaNode* targetNode,
                   mega::MegaRequestListener* listener) {
                    MegaSyncApp->getMegaApi()->moveNode(node, targetNode, listener);
                },
                MegaSyncApp->getMegaApi(),
                node,
                mFolderTarget);

            if(error)
            {
                break;
            }
        }
    }

    //Only done when the recursion finishes and there is no error
    if(!error && mDepth == 0)
    {
        std::unique_ptr<mega::MegaNodeList>folderChild(MegaSyncApp->getMegaApi()->getChildren(mFolderToMerge));
        if(folderChild->size() != 0)
        {
            MoveToMEGABin toBin;
            auto moveToBinError = toBin.moveToBin(mFolderToMerge->getHandle(), QLatin1String("FoldersMerge"), true);
            if(moveToBinError.binFolderCreationError)
            {
                error = moveToBinError.binFolderCreationError;
            }
            else if(!moveToBinError.moveError)
            {
                error = moveToBinError.moveError;
            }
        }
        else
        {
            error = MegaApiSynchronizedRequest::runRequest(&mega::MegaApi::remove, MegaSyncApp->getMegaApi(), mFolderToMerge);
        }
    }

    if(error)
    {
        mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_ERROR,
                           QString::fromUtf8("Merge folders failed. Error: %1")
                               .arg(Utilities::getTranslatedError(error.get()))
                               .toUtf8()
                               .constData());
    }
    return error;
}
