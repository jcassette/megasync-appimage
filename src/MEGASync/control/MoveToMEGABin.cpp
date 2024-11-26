#include "MoveToMEGABin.h"

#include <MEGAPathCreator.h>
#include <MegaApiSynchronizedRequest.h>
#include <MegaApplication.h>

#include <QDate>

//Move to BIN
/// This method is run synchronously. So, it waits for the SDK to create the folders, and only then we move the files
/// As the SDK createFolder is async, we need to use QEventLoop to stop the thread (the UI thread will be frozen only if you run this method in the UI thread)
/// When the createFolder returns we continue or stop the eventloop.
/// When all files are moved, we stop the eventloop and we continue
MoveToMEGABin::MoveToBinError MoveToMEGABin::moveToBin(mega::MegaHandle handle,
                                                       const QString& binFolderName,
                                                       bool addDateFolder)
{
    MoveToBinError error;

    auto moveLambda = [handle, &error](std::shared_ptr<mega::MegaNode> rubbishNode)
    {
        std::shared_ptr<mega::MegaNode> nodeToMove(
            MegaSyncApp->getMegaApi()->getNodeByHandle(handle));
        if (nodeToMove)
        {
            MegaApiSynchronizedRequest::runRequestLambdaWithResult(
                [](mega::MegaNode* node,
                   mega::MegaNode* targetNode,
                   mega::MegaRequestListener* listener)
                {
                    MegaSyncApp->getMegaApi()->moveNode(node, targetNode, listener);
                },
                MegaSyncApp->getMegaApi(),
                [nodeToMove, &error](mega::MegaRequest*, mega::MegaError* e)
                {
                    if (e->getErrorCode() != mega::MegaError::API_OK)
                    {
                        mega::MegaApi::log(
                            mega::MegaApi::LOG_LEVEL_ERROR,
                            QString::fromUtf8("Unable to move MEGA item to bin: %1. Error: %2")
                                .arg(QString::fromUtf8(nodeToMove->getName()),
                                     Utilities::getTranslatedError(e))
                                .toUtf8()
                                .constData());
                        error.moveError = std::shared_ptr<mega::MegaError>(e->copy());
                    }
                },
                nodeToMove.get(),
                rubbishNode.get());
        }
    };

    QString folderPath(binFolderName);
    if (addDateFolder)
    {
        QString dateFolder(QDate::currentDate().toString(Qt::DateFormat::ISODate));
        folderPath = QString::fromLatin1("%1/%2").arg(folderPath, dateFolder);
    }

    auto folderNode = MEGAPathCreator::mkDir(QString::fromLatin1("//bin"),
                                             folderPath,
                                             error.binFolderCreationError);
    if (folderNode)
    {
        moveLambda(folderNode);
    }
    else
    {
        mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_ERROR,
                           QString::fromUtf8("Unable to create %1 folder into MEGA bin: %1")
                               .arg(folderPath)
                               .toUtf8()
                               .constData());
    }

    return error;
}
