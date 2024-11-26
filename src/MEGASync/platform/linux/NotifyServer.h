#ifndef NOTIFYSERVER_H
#define NOTIFYSERVER_H

#include "MegaApplication.h"
#include "megaapi.h"

class NotifyServer: public QObject
{
    Q_OBJECT

 public:
    NotifyServer();
    virtual ~NotifyServer();
    void notifyItemChange(std::string *localPath);
    void notifySyncAdd(QString path);
    void notifySyncDel(QString path);

 protected:
    QLocalServer *m_localServer;

 public Q_SLOTS:
    void acceptConnection();
    void onClientDisconnected();
    void doSendToAll(const char *type, QByteArray str);

 private:
    MegaApplication *app;
    QString sockPath;
    QList<QLocalSocket *> m_clients;

signals:
    void sendToAll(const char *type, QByteArray str);

};

#endif

