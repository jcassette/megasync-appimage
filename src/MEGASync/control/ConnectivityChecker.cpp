#include "ConnectivityChecker.h"
#include "megaapi.h"

using namespace mega;

ConnectivityChecker::ConnectivityChecker(QString testURL, QObject *parent) :
    QObject(parent) , testRequest(testURL)
{
    networkAccess = new QNetworkAccessManager(this);
    timer = new QTimer(this);
    timer->setSingleShot(true);

    connect(timer, SIGNAL(timeout()), this, SLOT(onTestTimeout()));
    connect(networkAccess, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(onTestFinished(QNetworkReply*)));
    connect(networkAccess, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
            this, SLOT(onProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

    this->method = METHOD_GET;
    this->testURL = testURL;
    this->reply = NULL;
    timeoutms = 1000000;
}

void ConnectivityChecker::setProxy(QNetworkProxy proxy)
{
    this->proxy = proxy;
}

void ConnectivityChecker::setTimeout(int ms)
{
    this->timeoutms = ms;
}

void ConnectivityChecker::setTestString(QString testString)
{
    this->testString = testString;
}

void ConnectivityChecker::setMethod(int method)
{
    this->method = method;
}

void ConnectivityChecker::setHeader(QByteArray header, QByteArray value)
{
    testRequest.setRawHeader(header, value);
}

void ConnectivityChecker::setPostData(QByteArray postData)
{
    this->postData = postData;
}

void ConnectivityChecker::startCheck()
{    
    timer->stop();

    testRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    networkAccess->setProxy(proxy);

    timer->start(timeoutms);
    if (method == METHOD_POST)
    {
        testRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                              QByteArray("application/x-www-form-urlencoded"));
        reply = networkAccess->post(testRequest, postData);
    }
    else
    {
        reply = networkAccess->get(testRequest);
    }
}

void ConnectivityChecker::onTestFinished(QNetworkReply *reply)
{
    timer->stop();
    reply->deleteLater();
    this->reply = NULL;

    QVariant statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    if (!statusCode.isValid() || (statusCode.toInt() != 200) || (reply->error() != QNetworkReply::NoError))
    {
        QString e = QString::fromUtf8("Error testing proxy: %1:%2 %3-%4 (%5 - %6)").arg(proxy.hostName()).arg(proxy.port())
                .arg(proxy.user()).arg(proxy.password().size()).arg(statusCode.toInt()).arg(reply->errorString());
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, e.toUtf8().constData());

        emit testFinished(false);
        return;
    }

    QString data = QString::fromUtf8(reply->readAll());
    if (!testString.isEmpty() && !data.contains(testString))
    {
        QString e = QString::fromUtf8("Invalid response testing proxy: %1:%2 %3-%4 %5").arg(proxy.hostName()).arg(proxy.port())
                .arg(proxy.user()).arg(proxy.password().size()).arg(data);
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, e.toUtf8().constData());

        emit testFinished(false);
        return;
    }

    emit testFinished(true);
}

void ConnectivityChecker::onTestTimeout()
{
    if (reply)
    {
        reply->abort();
    }

    QString e = QString::fromUtf8("Timeout testing proxy: %1:%2 %3-%4").arg(proxy.hostName()).arg(proxy.port()).arg(proxy.user()).arg(proxy.password().size());
    MegaApi::log(MegaApi::LOG_LEVEL_ERROR, e.toUtf8().constData());
}

void ConnectivityChecker::onProxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *auth)
{
    if (!proxy.user().isEmpty())
    {
        auth->setUser(proxy.user());
        auth->setPassword(proxy.password());
    }
}
