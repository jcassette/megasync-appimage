#ifndef GUESTCONTENT_H
#define GUESTCONTENT_H

#include "QmlDialogWrapper.h"

class GuestContent : public QMLComponent
{
    Q_OBJECT

public:
    explicit GuestContent(QObject *parent = 0);

    QUrl getQmlUrl() override;

    QString contextName() override;

    Q_INVOKABLE void onAboutMEGAClicked();
    Q_INVOKABLE void onPreferencesClicked();
    Q_INVOKABLE void onExitClicked();
    Q_INVOKABLE void onVerifyEmailClicked();
    Q_INVOKABLE void onLogoutClicked();
    Q_INVOKABLE void onInitialPageButtonClicked();
};

#endif // GUESTCONTENT_H
