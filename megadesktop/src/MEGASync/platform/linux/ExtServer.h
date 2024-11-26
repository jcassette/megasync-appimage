#ifndef EXTSERVER_H
#define EXTSERVER_H

#include "MegaApplication.h"
#include "megaapi.h"

typedef enum {
   STRING_UPLOAD = 0,
   STRING_GETLINK = 1,
   STRING_SHARE = 2,
   STRING_SEND = 3,
   STRING_VIEW_ON_MEGA = 5,
   STRING_VIEW_VERSIONS = 6
} StringID;

class ExtServer: public QObject
{
    Q_OBJECT

 public:
    ExtServer(MegaApplication *app);
    virtual ~ExtServer();

 protected:
    QLocalServer *m_localServer;
    QQueue<QString> uploadQueue;
    QQueue<QString> exportQueue;

 public Q_SLOTS:
    void acceptConnection();
    void onClientData();
    void onClientDisconnected();
 private:
    QString sockPath;
    QList<QLocalSocket *> m_clients;
    std::string mLastPath;

    const char *GetAnswerToRequest(const char *buf);
    QString getActionName(const int actionId);

    void addToQueue(QQueue<QString>& queue, const char* content);
    void clearQueues();
    void viewOnMega(const char* content);

 signals:
    void newUploadQueue(QQueue<QString> uploadQueue);
    void newExportQueue(QQueue<QString> exportQueue);
    void viewOnMega(QByteArray path, bool versions);
};

#endif
