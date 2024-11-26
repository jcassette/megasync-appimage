
#ifndef WITH_KF5
//TODO: actually it makes no sense to have overlayplugin without KF5, since it seems to be available only after >= 5.16
#include <koverlayiconplugin.h>
#include <KPluginFactory>
#include <KIOCore/kfileitem.h>
#else
#include <KF5/KIOWidgets/KOverlayIconPlugin>
#include <KPluginFactory>
#include <KF5/KIOCore/kfileitem.h>
//#include <QLocalServer>
#endif

#include <QtNetwork/QLocalSocket>
#include <QDir>
#include <QMetaEnum>
#include <QtNetwork/QAbstractSocket>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif

typedef enum {
    RESPONSE_SYNCED  = 0,
    RESPONSE_PENDING = 1,
    RESPONSE_SYNCING = 2,
    RESPONSE_IGNORED = 3,
    RESPONSE_PAUSED  = 4,
    RESPONSE_DEFAULT = 9,
    RESPONSE_ERROR   = 10,
} FileState;

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

class MegasyncDolphinOverlayPlugin : public KOverlayIconPlugin
{
    Q_PLUGIN_METADATA(IID "com.megasync.ovarlayiconplugin" FILE "megasync-plugin-overlay.json")
    Q_OBJECT

    typedef QHash<QByteArray, QByteArray> StatusMap;
    StatusMap m_status;
    QLocalSocket sockNotifyServer;
    QString sockPathNofityServer;

    QLocalSocket sockExtServer;
    QString sockPathExtServer;

private slots:

    void sockNotifyServer_connected()
    {
        qDebug("MEGASYNCOVERLAYPLUGIN: connected to Notify Server");
    }

    void sockNotifyServer_disconnected()
    {
        qDebug("MEGASYNCOVERLAYPLUGIN: disconnected from Notify Server");
    }

    void sockNotifyServer_error(QLocalSocket::LocalSocketError err)
    {
        QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
        qCritical("MEGASYNCOVERLAYPLUGIN: error in connection to notify server: %s", metaEnum.valueToKey(err));
    }

    void sockExtServer_connected()
    {
        qDebug("MEGASYNCOVERLAYPLUGIN: connected to Ext Server");
    }

    void sockExtServer_disconnected()
    {
        qDebug("MEGASYNCOVERLAYPLUGIN: disconnected from Ext Server");
    }

    void sockExtServer_error(QLocalSocket::LocalSocketError err)
    {
        QMetaEnum metaEnum = QMetaEnum::fromType<QAbstractSocket::SocketError>();
        qCritical("MEGASYNCOVERLAYPLUGIN: error in connection to ext server: %s", metaEnum.valueToKey(err));
    }

    void notifiedfromServer()
    {
        qDebug("MEGASYNCOVERLAYPLUGIN: notifiedfromServer");

        while(sockNotifyServer.canReadLine())
        {

            char type[1];
            sockNotifyServer.read(type, 1); //TODO: control errors

            QString action="unknown";

            switch(*type) {
            case 'P': // item state changed
                action="item state changed";
                break;
            case 'A': // sync folder added
                action="sync folder added";
                break;
            case 'D': // sync folder deleted
                action="sync folder deleted";
                break;
            default:
                qCritical("MEGASYNCOVERLAYPLUGIN: unexpected read from notifyServer. type=%s", type);
                break;
            }

            QString url = sockNotifyServer.readLine();
            while(url.endsWith('\n')) url.chop(1);

            qDebug("MEGASYNCOVERLAYPLUGIN: Server notified <%s>: %s",action.toUtf8().constData(), url.toUtf8().constData());

            emit overlaysChanged(QUrl::fromLocalFile(url), getOverlays(QUrl::fromLocalFile(url)));
        }
    }

public:

    MegasyncDolphinOverlayPlugin()
    {
        qDebug("MEGASYNCOVERLAYPLUGIN: Loading plugin ... ");

        connect(&sockNotifyServer, SIGNAL(connected()), this, SLOT(sockNotifyServer_connected()));
        connect(&sockNotifyServer, SIGNAL(disconnected()), this, SLOT(sockNotifyServer_disconnected()));

        connect(&sockNotifyServer, SIGNAL(readyRead()), this, SLOT(notifiedfromServer()));
        connect(&sockNotifyServer, SIGNAL(error(QLocalSocket::LocalSocketError)),
                this, SLOT(sockNotifyServer_error(QLocalSocket::LocalSocketError)));

        connect(&sockExtServer, SIGNAL(connected()), this, SLOT(sockExtServer_connected()));
        connect(&sockExtServer, SIGNAL(disconnected()), this, SLOT(sockExtServer_disconnected()));
        connect(&sockExtServer, SIGNAL(error(QLocalSocket::LocalSocketError)),
                this, SLOT(sockExtServer_error(QLocalSocket::LocalSocketError)));

#if QT_VERSION < 0x050000
        sockPathNofityServer = QDir::home().path();
        sockPathNofityServer.append(QDir::separator()).append(".local/share/data/Mega Limited/MEGAsync/notify.socket");
#else
        sockPathNofityServer = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        sockPathNofityServer.append(QDir::separator()).append("data/Mega Limited/MEGAsync/notify.socket");
#endif
        sockNotifyServer.connectToServer(sockPathNofityServer);

#if QT_VERSION < 0x050000
        sockPathExtServer = QDir::home().path();
        sockPathExtServer.append(QDir::separator()).append(".local/share/data/Mega Limited/MEGAsync/mega.socket");
#else
        sockPathExtServer = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
        sockPathExtServer.append(QDir::separator()).append("data/Mega Limited/MEGAsync/mega.socket");
#endif
        sockExtServer.connectToServer(sockPathExtServer);
    }

    ~MegasyncDolphinOverlayPlugin()
    {
        sockNotifyServer.close();
    }

    QStringList getOverlays(const QUrl& url) override
    {
        if (!url.isLocalFile())
        {
            return QStringList();
        }

        QStringList r;

        int state = getState(url.toLocalFile());

        switch (state)
        {
            case RESPONSE_SYNCED:
                r << "mega-dolphin-synced";
                qDebug("MEGASYNCOVERLAYPLUGIN: getOverlays <%s>: mega-dolphin-synced",url.toLocalFile().toUtf8().constData());
                break;
            case RESPONSE_PENDING:
                r << "mega-dolphin-pending";
                qDebug("MEGASYNCOVERLAYPLUGIN: getOverlays <%s>: mega-dolphin-pending",url.toLocalFile().toUtf8().constData());
                break;
            case RESPONSE_SYNCING:
                r << "mega-dolphin-syncing";
                qDebug("MEGASYNCOVERLAYPLUGIN: getOverlays <%s>: mega-dolphin-syncing",url.toLocalFile().toUtf8().constData());
                break;
            default:
                qDebug("MEGASYNCOVERLAYPLUGIN: getOverlays <%s>: %d",url.toLocalFile().toUtf8().constData(),state);
                break;
        }

        return r;

        return QStringList();
    }

private:

    int getState(QString path)
    {
        QString res;
        res = sendRequest(OP_PATH_STATE, QFileInfo(path).canonicalFilePath());

        return res.isEmpty() ? RESPONSE_ERROR : res.toInt();
    }

    // send request and receive response from Extension server
    // Return newly-allocated response string
    QString sendRequest(char type, QString command)
    {
        int waitTime = -1; // This (instead of a timeout) makes dolphin hang until the location for an upload is selected (will be corrected in megasync>3.0.1).
                           // Otherwise megaync segafaults accesing client socket
        QString req;

        if(!sockExtServer.isOpen()) {
            sockExtServer.connectToServer(sockPathExtServer);
            if(!sockExtServer.waitForConnected(waitTime))
                return QString();
        }

        req.sprintf("%c:%s", type, command.toUtf8().constData());

        sockExtServer.write(req.toUtf8());
        sockExtServer.flush();

        if(!sockExtServer.waitForReadyRead(waitTime)) {
            sockExtServer.close();
            return QString();
        }

        QString reply;
        reply.append(sockExtServer.readAll());

        return reply;
    }
};

#include "megasync-plugin-overlay.moc"
