#include "GuestContent.h"

#include "GuestQmlDialog.h"
#include "MegaApplication.h"
#include "QmlDialogManager.h"

#include <QQmlEngine>

GuestContent::GuestContent(QObject *parent)
    : QMLComponent(parent)
{
    qmlRegisterModule("GuestContent", 1, 0);
    qmlRegisterType<GuestQmlDialog>("GuestQmlDialog", 1, 0, "GuestQmlDialog");
}

void GuestContent::onInitialPageButtonClicked()
{
#ifndef WIN32
    QmlDialogManager::instance()->openOnboardingDialog();
#endif
}

void GuestContent::onAboutMEGAClicked()
{
    MegaSyncApp->onAboutClicked();
}

void GuestContent::onPreferencesClicked()
{
    MegaSyncApp->openSettings();
}

void GuestContent::onExitClicked()
{
    MegaSyncApp->tryExitApplication();
}

void GuestContent::onVerifyEmailClicked()
{
    MegaSyncApp->getMegaApi()->resendVerificationEmail();
}

void GuestContent::onLogoutClicked()
{
    MegaSyncApp->unlink();
}

QUrl GuestContent::getQmlUrl()
{
    return QUrl(QString::fromUtf8("qrc:/guest/GuestDialog.qml"));
}

QString GuestContent::contextName()
{
    return QString::fromUtf8("guestContentAccess");
}
