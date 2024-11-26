// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "Notificator.h"

#include <QApplication>
#include <QByteArray>
#include <QIcon>
#include <QImageWriter>
#include "QMegaMessageBox.h"
#include "Utilities.h"
#include <QMetaType>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTemporaryFile>
#include <QVariant>
#include <memory>

#include "MegaApplication.h"

#include "platform/win/wintoastlib.h"
using namespace WinToastLib;

using namespace mega;

const QString& DesktopAppNotificationBase::defaultImage = QString();

Notificator::Notificator(const QString &programName, QSystemTrayIcon *trayicon, QObject *parent) :
    NotificatorBase(programName, trayicon, parent)
{
    if (trayicon && trayicon->supportsMessages())
    {
        mMode = QSystemTray;
    }

    mCurrentNotification = NULL;

    WinToast::instance()->setAppName(L"MEGAsync");
    WinToast::instance()->setAppUserModelId(L"MegaLimited.MEGAsync");
    WinToast::instance()->initialize();
    connect(mTrayIcon, SIGNAL(messageClicked()), this, SLOT(onMessageClicked()));
}

void Notificator::notifySystray(Class cls, const QString &title, const QString &text, int millisTimeout, bool forceQt)
{

    if (!forceQt && WinToast::instance()->isCompatible())
    {
        DesktopAppNotification *n = new DesktopAppNotification();
        if (title != MegaSyncApp->getMEGAString())
        {
            n->setTitle(title);
        }
        n->setText(text);
        n->setExpirationTime(millisTimeout);
        n->setType(cls);
        n->setStyle(WinToastTemplate::ImageAndText01);
        notifySystray(n);
    }
    else
    {
        NotificatorBase::notifySystray(cls, title, text, millisTimeout, forceQt);
    }
}

void Notificator::notifySystray(DesktopAppNotificationBase *notification)
{
    if (!notification)
    {
        return;
    }

    if (!WinToast::instance()->isCompatible())
    {
        if (mCurrentNotification)
        {
            mCurrentNotification->deleteLater();
        }
        mCurrentNotification = notification;
        notifySystray((Notificator::Class)notification->getType(), notification->getTitle(),
                      notification->getText(), notification->getExpirationTime(),
                      true);
        return;
    }

    connect(notification, &DesktopAppNotificationBase::failed, this, &Notificator::onModernNotificationFailed);

    WinToastTemplate templ(WinToastTemplate::ImageAndText02);
    templ.setTextField((LPCWSTR)notification->getTitle().utf16(), WinToastTemplate::FirstLine);
    templ.setTextField((LPCWSTR)notification->getText().utf16(), WinToastTemplate::SecondLine);
    templ.setAttributionText((LPCWSTR)notification->getSource().utf16());
    templ.setExpiration(notification->getExpirationTime());
    if (!notification->getImagePath().isEmpty())
    {
        templ.setImagePath((LPCWSTR)notification->getImagePath().utf16());
    }

    QStringList userActions = notification->getActions();
    for (int i = 0; i < userActions.size(); i++)
    {
        templ.addAction((LPCWSTR)userActions.at(i).utf16());
    }

    notification->setId(WinToast::instance()->showToast(templ, std::shared_ptr<IWinToastHandler>(new WinToastNotification(notification))));
}

void Notificator::onModernNotificationFailed()
{
    DesktopAppNotification *notification = qobject_cast<DesktopAppNotification *>(QObject::sender());
    if (mCurrentNotification)
    {
        mCurrentNotification->deleteLater();
    }
    mCurrentNotification = notification;
    notifySystray((Notificator::Class)notification->getType(), notification->getTitle(),
                  notification->getText(),
                  notification->getExpirationTime(), true);
}

DesktopAppNotification::DesktopAppNotification()
    : DesktopAppNotificationBase()
{
    QFile icon(QString::fromUtf8("://images/app_ico.ico"));
    mImagePath = MegaApplication::applicationDataPath() + QString::fromUtf8("\\MEGAsync.ico");
    if (!QFile(mImagePath).exists())
    {
        icon.copy(mImagePath);
    }
}

DesktopAppNotification::~DesktopAppNotification()
{
    if (mId != -1)
    {
        WinToast::instance()->hideToast(mId);
    }
}

QMutex WinToastNotification::mMutex;

WinToastNotification::WinToastNotification(QPointer<DesktopAppNotificationBase> megaNotification)
{
    this->notification = megaNotification;
}

WinToastNotification::~WinToastNotification()
{
}

void WinToastNotification::toastActivated()
{
    mMutex.lock();
    if (notification)
    {
        emit notification->activated(DesktopAppNotification::Action::content);
        notification = NULL;
    }
    mMutex.unlock();
}

void WinToastNotification::toastActivated(int actionIndex)
{
    mMutex.lock();
    if (notification)
    {
        emit notification->activated(static_cast<DesktopAppNotification::Action>(actionIndex));
        notification = NULL;
    }
    mMutex.unlock();
}

void WinToastNotification::toastDismissed(WinToastDismissalReason state)
{
    mMutex.lock();
    if (notification)
    {
        auto reason = DesktopAppNotification::CloseReason::Unknown;
        switch (state)
        {
        case WinToastDismissalReason::UserCanceled:
            reason = DesktopAppNotification::CloseReason::UserAction;
            break;
        case WinToastDismissalReason::ApplicationHidden:
            reason = DesktopAppNotification::CloseReason::AppHidden;
            break;
        case WinToastDismissalReason::TimedOut:
            reason = DesktopAppNotification::CloseReason::TimedOut;
            break;
        }

        emit notification->closed(reason);
        notification = NULL;
    }
    mMutex.unlock();
}

void WinToastNotification::toastFailed()
{
    mMutex.lock();
    if (notification)
    {
        emit notification->failed();
        notification = NULL;
    }
    mMutex.unlock();
}
