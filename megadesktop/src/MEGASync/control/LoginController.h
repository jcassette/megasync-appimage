#ifndef LOGINCONTROLLER_H
#define LOGINCONTROLLER_H

#include "megaapi.h"
#include "mega/bindings/qt/QTMegaGlobalListener.h"

#include <QObject>
#include <QTimer>

#include <memory>

namespace mega
{
    class QTMegaRequestListener;
}

class Preferences;
class LoginController : public QObject, public mega::MegaGlobalListener
{
    Q_OBJECT
    Q_PROPERTY(bool newAccount MEMBER mNewAccount CONSTANT)
    Q_PROPERTY(QString email MEMBER mEmail READ getEmail NOTIFY emailChanged)
    Q_PROPERTY(double progress MEMBER mProgress READ getProgress NOTIFY progressChanged)
    Q_PROPERTY(State state MEMBER mState READ getState  WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(bool emailError MEMBER mEmailError READ getEmailError WRITE setEmailError NOTIFY emailErrorChanged)
    Q_PROPERTY(QString emailErrorMsg MEMBER mEmailErrorMsg READ getEmailErrorMsg WRITE setEmailErrorMsg NOTIFY emailErrorMsgChanged)
    Q_PROPERTY(bool passwordError MEMBER mPasswordError READ getPasswordError WRITE setPasswordError NOTIFY passwordErrorChanged)
    Q_PROPERTY(QString passwordErrorMsg MEMBER mPasswordErrorMsg READ getPasswordErrorMsg WRITE setPasswordErrorMsg NOTIFY passwordErrorMsgChanged)
    Q_PROPERTY(QString createAccountErrorMsg MEMBER mCreateAccountErrorMsg
                   READ getCreateAccountErrorMsg WRITE setCreateAccountErrorMsg NOTIFY createAccountErrorMsgChanged)

public:
    enum State {
        LOGGED_OUT = 0,
        SIGN_UP,
        CHANGING_REGISTER_EMAIL,
        LOGGING_IN,
        LOGGING_IN_2FA_REQUIRED,
        LOGGING_IN_2FA_VALIDATING,
        LOGGING_IN_2FA_FAILED,
        CREATING_ACCOUNT,
        CREATING_ACCOUNT_FAILED,
        WAITING_EMAIL_CONFIRMATION,
        EMAIL_CONFIRMED,
        FETCHING_NODES,
        FETCHING_NODES_2FA,
        FETCH_NODES_FINISHED_ONBOARDING,
        FETCH_NODES_FINISHED
    };
    Q_ENUM(State)

    explicit LoginController(QObject *parent = nullptr);
    virtual ~LoginController() = default;
    Q_INVOKABLE void login(const QString& email, const QString& password);
    Q_INVOKABLE void createAccount(const QString& email, const QString& password, const QString& name, const QString& lastName);
    Q_INVOKABLE void changeRegistrationEmail(const QString& email);
    Q_INVOKABLE void login2FA(const QString& pin);
    Q_INVOKABLE const QString& getEmail() const;
    Q_INVOKABLE void cancelLogin() const;
    Q_INVOKABLE void cancelCreateAccount() const;
    Q_INVOKABLE double getProgress() const;
    Q_INVOKABLE State getState() const;
    Q_INVOKABLE void setState(State state);
    Q_INVOKABLE bool getEmailError() const;
    Q_INVOKABLE const QString& getEmailErrorMsg() const;
    Q_INVOKABLE void setEmailError(bool error);
    Q_INVOKABLE void setEmailErrorMsg(const QString& msg);
    Q_INVOKABLE bool getPasswordError() const;
    Q_INVOKABLE const QString& getPasswordErrorMsg() const;
    Q_INVOKABLE void setPasswordError(bool error);
    Q_INVOKABLE void setPasswordErrorMsg(const QString& msg);
    Q_INVOKABLE const QString& getCreateAccountErrorMsg() const;
    Q_INVOKABLE void setCreateAccountErrorMsg(const QString& msg);

    void processOnboardingClosed();

    bool isFetchNodesFinished() const;

    void onRequestFinish(mega::MegaRequest* request, mega::MegaError* e);
    void onRequestUpdate(mega::MegaRequest* request);
    void onRequestStart(mega::MegaRequest *request);

    void onEvent(mega::MegaApi*, mega::MegaEvent* event) override;

signals:

    void emailChanged();
    void changeRegistrationEmailFinished(bool success, const QString& errorMsg = QString());
    void emailConfirmed();
    void progressChanged();
    void stateChanged();
    void emailErrorMsgChanged();
    void emailErrorChanged();
    void passwordErrorMsgChanged();
    void passwordErrorChanged();
    void createAccountErrorMsgChanged();
    void accountCreationCancelled();

protected:
    virtual void onLogin(mega::MegaRequest* request, mega::MegaError* e);
    virtual void onboardingFinished();
    void onAccountCreation(mega::MegaRequest* request, mega::MegaError* e);
    void onAccountCreationResume(mega::MegaRequest* request, mega::MegaError* e);
    void onEmailChanged(mega::MegaRequest* request, mega::MegaError* e);
    void onFetchNodes(mega::MegaRequest* request, mega::MegaError* e);
    void onWhyAmIBlocked(mega::MegaRequest* request, mega::MegaError* e);
    void onAccountCreationCancel(mega::MegaRequest* request, mega::MegaError* e);

    void onLogout(mega::MegaRequest* request, mega::MegaError* e);
    void fetchNodes(const QString& email = QString());

    mega::MegaApi * mMegaApi;
    std::shared_ptr<Preferences> mPreferences;

private slots:
    void runConnectivityCheck();
    void onConnectivityCheckFinished(bool success);

private:
    unsigned long long computeExclusionSizeLimit(const unsigned long long& sizeLimitValue, const int unit);
    void migrateSyncConfToSdk(const QString& email);
    void loadSyncExclusionRules(const QString& email);
    void dumpSession();
    QString getRepeatedEmailMsg();
    void setEmail(const QString& email);

    std::shared_ptr<mega::QTMegaRequestListener> mDelegateListener;
    std::unique_ptr<mega::QTMegaGlobalListener> mGlobalListener;
    std::unique_ptr<mega::MegaEvent> eventPendingStorage;

    QTimer *mConnectivityTimer;
    bool mEmailError;
    QString mEmailErrorMsg;
    bool mPasswordError;
    QString mPasswordErrorMsg;
    QString mCreateAccountErrorMsg;
    double mProgress;
    State mState;
    QString mEmail;
    QString mName;
    QString mLastName;
    QString mPassword;
    bool mNewAccount;
};

class FastLoginController : public LoginController
{
    Q_OBJECT

public:
    explicit FastLoginController(QObject* parent =  nullptr);
    bool fastLogin();

protected:
    void onLogin(mega::MegaRequest* request, mega::MegaError* e) override;
    void onboardingFinished() override;

};

class LogoutController : public QObject
{
    Q_OBJECT

public:
    explicit LogoutController(QObject* parent =  nullptr);
    virtual ~LogoutController();
    void onRequestFinish(mega::MegaRequest* request, mega::MegaError* e);
    void onRequestStart(mega::MegaRequest *request);

signals:
    void logout(bool isLocalLogout);

private:
    mega::MegaApi * mMegaApi;
    std::shared_ptr<mega::QTMegaRequestListener> mDelegateListener;
    bool mLoginInWithoutSession;
};

#endif // LOGINCONTROLLER_H
