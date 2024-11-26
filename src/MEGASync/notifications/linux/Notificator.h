// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef NOTIFICATOR_H
#define NOTIFICATOR_H

#include "NotificatorBase.h"

#ifdef USE_DBUS
#include <QDBusMessage>
class QDBusInterface;
#endif

class DesktopAppNotification : public DesktopAppNotificationBase
{
    Q_OBJECT

public:
    DesktopAppNotification();
    ~DesktopAppNotification() = default;

    QIcon getImage() const;
    void setImagePath(const QString &value) override;

#ifdef USE_DBUS
public slots:
    void dBusNotificationSentCallback(QDBusMessage dbusMssage);
    void dbusNotificationSentErrorCallback(QDBusError error);
    void dBusNotificationCallback(QDBusMessage dbusMssage);
#endif

protected:
    QIcon image;
};


/** Cross-platform desktop notification client. */
class Notificator: public NotificatorBase
{
    Q_OBJECT

public:
    /** Create a new notificator.
       @note Ownership of trayIcon is not transferred to this object.
    */
    Notificator(const QString &programName, QSystemTrayIcon *trayIcon, QObject *parent);
    ~Notificator();

    void notify(Class cls, const QString &title, const QString &text, int millisTimeout = 10000);
    void notify(DesktopAppNotification *notification);

#ifdef USE_DBUS
private:
    QDBusInterface *interface;
    bool dbussSupportsActions;

    void notifyDBus(Class cls, const QString &title, const QString &text, const QIcon &icon, int millisTimeout, const QStringList &actions = QStringList(), DesktopAppNotification *notification = nullptr);
#endif
};

#endif // NOTIFICATOR_H
