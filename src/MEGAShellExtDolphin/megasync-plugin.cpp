#include <kfileitem.h>
#include <kfileitemlistproperties.h>
#include <QDir>
#include <QFileInfo>
#include <QString>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif

#ifndef WITH_KF5
#include <kaction.h>
#include <kdemacros.h>
#include <KDE/KPluginFactory>
#include <KDE/KPluginLoader>
#else
#include <KPluginFactory>
#include <KPluginLoader>
#include <KIOWidgets/kabstractfileitemactionplugin.h>
#include <QtNetwork/QLocalSocket>
#include <KIOCore/kfileitem.h>
#include <KIOCore/KFileItemListProperties>
#include <QtWidgets/QAction>
#include <QtCore/QDir>
#include <QtCore/QTimer>

#define KAction QAction
#define KIcon QIcon

#endif

#include "megasync-plugin.h"

K_PLUGIN_FACTORY(MEGASyncPluginFactory, registerPlugin<MEGASyncPlugin>();)
K_EXPORT_PLUGIN(MEGASyncPluginFactory("megasync-plugin"))

typedef enum {
    STRING_UPLOAD = 0,
    STRING_GETLINK = 1,
    STRING_SHARE = 2,
    STRING_SEND = 3,

    STRING_VIEW_ON_MEGA = 5,
    STRING_VIEW_VERSIONS = 6
} StringID;

enum {
    RESPONSE_SYNCED  = 0,
    RESPONSE_PENDING = 1,
    RESPONSE_SYNCING = 2,
    RESPONSE_IGNORED = 3,
    RESPONSE_PAUSED  = 4,
    RESPONSE_DEFAULT = 9,
    RESPONSE_ERROR   = 10,
};

const char OP_PATH_STATE  = 'P'; //Path state
const char OP_INIT        = 'I'; //Init operation
const char OP_END         = 'E'; //End operation
const char OP_UPLOAD      = 'F'; //File-Folder upload
const char OP_LINK        = 'L'; //paste Link
const char OP_SHARE       = 'S'; //Share folder
const char OP_SEND        = 'C'; //Copy to user
const char OP_STRING      = 'T'; //Get Translated String
const char OP_VIEW        = 'V'; //View on MEGA
const char OP_PREVIOUS    = 'R'; //View previous versions


MEGASyncPlugin::MEGASyncPlugin(QObject* parent, const QList<QVariant> & args):
    KAbstractFileItemActionPlugin(parent)
{
    qDebug("MEGASYNCPLUGIN : Started");
    Q_UNUSED(args);
#if QT_VERSION < 0x050000
    sockPath = QDir::home().path();
    sockPath.append(QDir::separator()).append(".local/share/data/Mega Limited/MEGAsync/mega.socket");
#else
    sockPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    sockPath.append(QDir::separator()).append("data/Mega Limited/MEGAsync/mega.socket");
#endif
    sock.connectToServer(sockPath);
}

MEGASyncPlugin::~MEGASyncPlugin()
{
    qDebug("MEGASYNCPLUGIN : Closing");
    sock.close();
}

QList<QAction*> MEGASyncPlugin::actions(const KFileItemListProperties & fileItemInfos, QWidget * parentWidget)
{
    Q_UNUSED(parentWidget);
    QList<QAction*> actions;
    int state = RESPONSE_ERROR;

    // Check if the Desktop App is running
    selectedFilePath = QString::fromLatin1("~");
    state = getState();

    if (state == RESPONSE_ERROR)
    {
        qDebug("MEGASYNCPLUGIN : Desktop App not running");
        return actions;
    }

    int syncedFiles, syncedFolders, unsyncedFiles, unsyncedFolders;
    syncedFiles = syncedFolders = unsyncedFiles = unsyncedFolders = 0;

    // Make sure we only capture what the user has selected.
    selectedFilePath.clear();
    selectedFilePaths.clear();

    for( int i = 0; i < fileItemInfos.items().count(); i++)
    {
        KFileItem item = fileItemInfos.items().at(i);
        selectedFilePath = item.localPath();
        selectedFilePaths << selectedFilePath;

        // get the state of selected file
        state = getState();

        // count the number of synced / unsynced files and folders
        if (state == RESPONSE_SYNCED || state == RESPONSE_SYNCING || state == RESPONSE_PENDING)
        {
            if (item.isDir())
            {
                syncedFolders++;
            }
            else
            {
                syncedFiles++;
            }
        }
        else
        {
            if (item.isDir())
            {
                unsyncedFolders++;
            }
            else
            {
                unsyncedFiles++;
            }
        }
    }

    // populate Menu
    KActionMenu * menuAction = new KActionMenu(this);
    menuAction->setText("MEGA");
    actions << menuAction;

    // if there any unsynced files / folders selected
    if (unsyncedFiles || unsyncedFolders)
    {
        QAction* act = createChildAction(menuAction, STRING_UPLOAD, unsyncedFiles, unsyncedFolders);
        if (act)
        {
            connect(act, &QAction::triggered, this, &MEGASyncPlugin::uploadFiles);
        }
    }

    // if there any synced files / folders selected
    if (syncedFiles || syncedFolders)
    {
        QAction* act = createChildAction(menuAction, STRING_GETLINK, syncedFiles, syncedFolders);
        if (act)
        {
            // set menu icon //TODO: state refers to the last file. Does it make any sense??
            if (state == RESPONSE_SYNCED)
                act->setIcon(KIcon("mega-synced"));
            else if (state == RESPONSE_PENDING)
                act->setIcon(KIcon("mega-pending"));
            else if (state == RESPONSE_SYNCING)
                act->setIcon(KIcon("mega-syncing"));

            connect(act, SIGNAL(triggered()), this, SLOT(getLinks()));
        }
    }


    if (!unsyncedFiles && !unsyncedFolders && (syncedFiles + syncedFolders) == 1)
    {
        if (syncedFolders)
        {
            QAction* act = createChildAction(menuAction, STRING_VIEW_ON_MEGA);
            if (act)
            {
                connect(act, &QAction::triggered, this, &MEGASyncPlugin::viewOnMega);
            }
        }
        else
        {
            QAction* act = createChildAction(menuAction, STRING_VIEW_VERSIONS);
            if (act)
            {
                connect(act, &QAction::triggered, this, &MEGASyncPlugin::viewPreviousVersions);
            }
        }
    }

    qDebug("MEGASYNCPLUGIN : Created actions");

    return actions;
}

int MEGASyncPlugin::getState()
{
    QString res;
    QString cannonicalpath = QFileInfo(selectedFilePath).canonicalFilePath();
    cannonicalpath.append((char)0x1C);
    cannonicalpath.append('1');
    res = sendRequest(OP_PATH_STATE,cannonicalpath);

    return res.isEmpty() ? RESPONSE_ERROR : res.toInt();
}

void MEGASyncPlugin::getLink()
{
    if (sendRequest(OP_LINK, QFileInfo(selectedFilePath).canonicalFilePath()).size())
    {
        sendRequest(OP_END, " ");
    }
}

void MEGASyncPlugin::getLinks()
{
    for(int i = 0; i<selectedFilePaths.size(); i++)
    {
        QString path = selectedFilePaths.at(i);
        if (sendRequest(OP_LINK, QFileInfo(path).canonicalFilePath()).size())
        {
        }
    }
    sendRequest(OP_END, " ");
}

void MEGASyncPlugin::uploadFile()
{
    if (sendRequest(OP_UPLOAD, QFileInfo(selectedFilePath).canonicalFilePath()).size())
    {
        sendRequest(OP_END, " ");
    }
}

void MEGASyncPlugin::uploadFiles()
{
    for(int i = 0; i<selectedFilePaths.size(); i++)
    {
        QString path = selectedFilePaths.at(i);
        if (sendRequest(OP_UPLOAD, QFileInfo(path).canonicalFilePath()).size())
        {
        }
    }
    sendRequest(OP_END, " ");
}

void MEGASyncPlugin::viewOnMega()
{
    if (sendRequest(OP_VIEW, QFileInfo(selectedFilePath).canonicalFilePath()).size())
    {
        sendRequest(OP_END, " ");
    }
}

void MEGASyncPlugin::viewPreviousVersions()
{
    if (sendRequest(OP_PREVIOUS, QFileInfo(selectedFilePath).canonicalFilePath()).size())
    {
        sendRequest(OP_END, " ");
    }
}

QString MEGASyncPlugin::getString(int type, int numFiles,int numFolders)
{
    QString res;
    QString queryString = "";
    queryString.sprintf("%d:%d:%d", type, numFiles, numFolders);

    res = sendRequest(OP_STRING, queryString);
    int responseCode = res.isEmpty() ? RESPONSE_ERROR : res.toInt();

    if(responseCode == RESPONSE_ERROR || responseCode == RESPONSE_DEFAULT)
    {
        res.clear();
    }

    return res;
}

QAction *MEGASyncPlugin::createChildAction(KActionMenu *menu, int type, int numFiles, int numFolders)
{
    QString actionText = getString(type, numFiles, numFolders);
    if(!actionText.isEmpty())
    {
       QAction *act = new KAction(actionText, this);
       menu->addAction(act);
       return act;
    }
    return nullptr;
}

// send request and receive response from Extension server
// Return newly-allocated response string
QString MEGASyncPlugin::sendRequest(char type, QString command)
{
    int waitTime = -1; // This (instead of a timeout) makes dolphin hang until the location for an upload is selected (will be corrected in megasync>3.0.1).
                       // Otherwise megaync segafaults accesing client socket
    QString req;

    if(!sock.isOpen()) {
        sock.connectToServer(sockPath);
        if(!sock.waitForConnected(waitTime))
            return QString();
    }

    req.sprintf("%c:%s", type, command.toUtf8().constData());

    qDebug("MEGASYNCPLUGIN : Sending request \"%s\"", req.toUtf8().constData());

    sock.write(req.toUtf8());
    sock.flush();

    if(!sock.waitForReadyRead(waitTime)) {
        sock.close();
        return QString();
    }

    QString reply (sock.readAll().trimmed());

    return reply;
}

#include "megasync-plugin.moc"
