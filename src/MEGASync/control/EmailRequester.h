#ifndef EMAILREQUESTER_H
#define EMAILREQUESTER_H

#include <mega/bindings/qt/QTMegaGlobalListener.h>

#include <QMap>
#include <QObject>
#include <QRecursiveMutex>
#include <memory>

class RequestInfo: public QObject
{
    Q_OBJECT

public :
    explicit RequestInfo(QObject* parent = nullptr)
        : QObject(parent),
        requestFinished(true)
    {
    }

    bool requestFinished;
    void setEmail(const QString& email);
    QString getEmail() const;

signals:
    void emailChanged(QString email);

private:
    QString mEmail;
};


class EmailRequester : public QObject, public mega::MegaGlobalListener
{
    Q_OBJECT

public:
    static EmailRequester* instance();

    void reset();
    QString getEmail(mega::MegaHandle userHandle);
    static RequestInfo* getRequest(mega::MegaHandle userHandle, const QString& email = QString());
    RequestInfo* addUser(mega::MegaHandle userHandle, const QString& email = QString());
    void onUsersUpdate(mega::MegaApi* api, mega::MegaUserList *users) override;

private:
    explicit EmailRequester();
    void requestEmail(mega::MegaHandle userHandle);

    mega::MegaApi* mMegaApi;
    QRecursiveMutex mRequestsDataLock;
    QMap<mega::MegaHandle, RequestInfo*> mRequestsData;
    std::unique_ptr<mega::QTMegaGlobalListener> mGlobalListener;
    static std::unique_ptr<EmailRequester> mInstance;
};

#endif
