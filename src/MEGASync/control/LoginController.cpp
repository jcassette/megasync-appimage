#include "LoginController.h"

#include "MegaApplication.h"
#include "ConnectivityChecker.h"
#include "Platform.h"
#include "QMegaMessageBox.h"

#include "TextDecorator.h"
#include "StatsEventHandler.h"

#include "Preferences.h"
#include "QmlDialogManager.h"

#include <QQmlContext>
#include "RequestListenerManager.h"

LoginController::LoginController(QObject* parent)
    : QObject{parent}
      , mMegaApi(MegaSyncApp->getMegaApi())
      , mPreferences(Preferences::instance())
      , mGlobalListener(std::make_unique<mega::QTMegaGlobalListener>(MegaSyncApp->getMegaApi(), this))
      , mEmailError(false)
      , mPasswordError(false)
      , mProgress(0)
      , mState(LOGGED_OUT)
      , mNewAccount(false)
{
    ListenerCallbacks lcInfo{
        this,
        std::bind(&LoginController::onRequestStart, this, std::placeholders::_1),
        std::bind(&LoginController::onRequestFinish, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&LoginController::onRequestUpdate, this, std::placeholders::_1)
    };

    mDelegateListener = RequestListenerManager::instance().registerAndGetListener(lcInfo);

    mMegaApi->addRequestListener(mDelegateListener.get());
    mMegaApi->addGlobalListener(mGlobalListener.get());
    mConnectivityTimer = new QTimer(this);
    mConnectivityTimer->setSingleShot(true);
    mConnectivityTimer->setInterval(static_cast<int>(Preferences::MAX_LOGIN_TIME_MS));
    connect(mConnectivityTimer, &QTimer::timeout, this, &LoginController::runConnectivityCheck);

    EphemeralCredentials credentials{mPreferences->getEphemeralCredentials()};
    if(!credentials.sessionId.isEmpty())
    {
        mMegaApi->resumeCreateAccount(credentials.sessionId.toUtf8().constData());
    }
}

void LoginController::login(const QString& email, const QString& password)
{
    mMegaApi->login(email.toUtf8().constData(), password.toUtf8().constData());
}

void LoginController::createAccount(const QString& email, const QString& password,
                                const QString& name, const QString& lastName)
{
    mMegaApi->createAccount(email.toUtf8().constData(), password.toUtf8().constData(),
                             name.toUtf8().constData(), lastName.toUtf8().constData());
}

void LoginController::changeRegistrationEmail(const QString& email)
{
    QString fullName = QLatin1String("%1 %2").arg(mName, mLastName);
    mMegaApi->resendSignupLink(email.toUtf8().constData(), fullName.toUtf8().constData());
}

void LoginController::login2FA(const QString& pin)
{
    mMegaApi->multiFactorAuthLogin(mEmail.toUtf8().constData(), mPassword.toUtf8().constData(), pin.toUtf8().constData());
}

const QString& LoginController::getEmail() const
{
    return mEmail;
}

void LoginController::cancelLogin() const
{
    mMegaApi->localLogout();
}

void LoginController::cancelCreateAccount() const
{
    mMegaApi->cancelCreateAccount();
}

double LoginController::getProgress() const
{
    return mProgress;
}

LoginController::State LoginController::getState() const
{
    return mState;
}

void LoginController::setState(State state)
{
    if(mState != state)
    {
        mState = state;
        emit stateChanged();
    }
}

bool LoginController::getEmailError() const
{
    return mEmailError;
}

const QString& LoginController::getEmailErrorMsg() const
{
    return mEmailErrorMsg;
}

void LoginController::setEmailError(bool error)
{
    if(error != mEmailError)
    {
        mEmailError = error;
        emit emailErrorChanged();
    }
}

void LoginController::setEmailErrorMsg(const QString& msg)
{
    if(msg != mEmailErrorMsg)
    {
        mEmailErrorMsg = msg;
        emit emailErrorMsgChanged();
    }
}

bool LoginController::getPasswordError() const
{
    return mPasswordError;
}

const QString& LoginController::getPasswordErrorMsg() const
{
    return mPasswordErrorMsg;
}

void LoginController::setPasswordError(bool error)
{
    if(error != mPasswordError)
    {
        mPasswordError = error;
        emit passwordErrorChanged();
    }
}

void LoginController::setPasswordErrorMsg(const QString& msg)
{
    if(msg != mPasswordErrorMsg)
    {
        mPasswordErrorMsg = msg;
        emit passwordErrorMsgChanged();
    }
}

const QString& LoginController::getCreateAccountErrorMsg() const
{
    return mCreateAccountErrorMsg;
}

void LoginController::setCreateAccountErrorMsg(const QString& msg)
{
    if(msg != mCreateAccountErrorMsg)
    {
        mCreateAccountErrorMsg = msg;
        emit createAccountErrorMsgChanged();
    }
}

void LoginController::processOnboardingClosed()
{
    if(getState() == LoginController::State::FETCH_NODES_FINISHED_ONBOARDING)
    {
        if (!Preferences::instance()->isFirstSyncDone() &&
            !Preferences::instance()->isFirstBackupDone())
        {
            MegaSyncApp->getStatsEventHandler()->sendEvent(
                AppStatsEvents::EventType::ONBOARDING_CLOSED_WITHOUT_SETTING_SYNCS);
        }

        setState(LoginController::State::FETCH_NODES_FINISHED);
        onboardingFinished();
    }
}

bool LoginController::isFetchNodesFinished() const
{
    return getState() >= LoginController::State::FETCH_NODES_FINISHED_ONBOARDING;
}

void LoginController::onRequestFinish(mega::MegaRequest* request, mega::MegaError* e)
{
    switch(request->getType())
    {
        case mega::MegaRequest::TYPE_LOGIN:
        {
            mConnectivityTimer->stop();
            MegaSyncApp->initLocalServer();
            if(e->getErrorCode() == mega::MegaError::API_OK)
            {
                mPreferences->setAccountStateInGeneral(Preferences::STATE_LOGGED_OK);
            }
            else if(e->getErrorCode() != mega::MegaError::API_EMFAREQUIRED)
            {
                if(request->getText())
                {
                    setState(LOGGING_IN_2FA_FAILED);
                }
                else
                {
                    setState(LOGGED_OUT);
                }
            }
            onLogin(request, e);
            break;
        }
        case mega::MegaRequest::TYPE_LOGOUT:
        {
            onLogout(request, e);
            break;
        }
        case mega::MegaRequest::TYPE_CREATE_ACCOUNT:
        {
            if(request->getParamType() == mega::MegaApi::RESUME_ACCOUNT)
            {
                onAccountCreationResume(request, e);
            }
            else if(request->getParamType() == mega::MegaApi::CANCEL_ACCOUNT)
            {
                onAccountCreationCancel(request, e);
            }
            else
            {
                onAccountCreation(request, e);
            }
            break;
        }
        case mega::MegaRequest::TYPE_SEND_SIGNUP_LINK:
        {
            onEmailChanged(request, e);
            break;
        }
        case mega::MegaRequest::TYPE_FETCH_NODES:
        {
            onFetchNodes(request, e);
            break;
        }
        case mega::MegaRequest::TYPE_WHY_AM_I_BLOCKED:
        {
            onWhyAmIBlocked(request, e);
            break;
        }
    }
}

void LoginController::onRequestUpdate(mega::MegaRequest* request)
{
    if (request->getType() == mega::MegaRequest::TYPE_FETCH_NODES)
    {
        if (request->getTotalBytes() > 0)
        {
            double total = static_cast<double>(request->getTotalBytes());
            double part = static_cast<double>(request->getTransferredBytes());
            double progress = part/total;

            mProgress = progress;
            emit progressChanged();
        }
    }
}

void LoginController::onRequestStart(mega::MegaRequest* request)
{
    switch(request->getType())
    {
    case mega::MegaRequest::TYPE_LOGIN:
    {
        mConnectivityTimer->start();
        if(request->getText())
        {
            setState(LOGGING_IN_2FA_VALIDATING);
        }
        else
        {
            setState(LOGGING_IN);
        }
        break;
    }
    case mega::MegaRequest::TYPE_CREATE_ACCOUNT:
    {
        if(request->getParamType() == mega::MegaApi::CREATE_ACCOUNT)
        {
            setState(CREATING_ACCOUNT);
        }
        break;
    }
    case mega::MegaRequest::TYPE_FETCH_NODES:
    {
        if(mState == LOGGING_IN_2FA_VALIDATING)
        {
            setState(FETCHING_NODES_2FA);
        }
        else
        {
            setState(FETCHING_NODES);
        }
        break;
    }
    }
}

void LoginController::onEvent(mega::MegaApi*, mega::MegaEvent* event)
{
    if(event->getType() == mega::MegaEvent::EVENT_CONFIRM_USER_EMAIL)
    {
        mNewAccount = true;
        setEmail(QString::fromLatin1(event->getText()));
        mPreferences->removeEphemeralCredentials();
        setState(EMAIL_CONFIRMED);
        emit emailConfirmed();
    }
    else if (event->getType() == mega::MegaEvent::EVENT_STORAGE)
    {
        if (!isFetchNodesFinished())//event arrived too soon, we will apply it later
        {
            eventPendingStorage.reset(event->copy());
        }
        else
        {
            eventPendingStorage.reset();
        }
    }
}

void LoginController::onLogin(mega::MegaRequest* request, mega::MegaError* e)
{
    if(e->getErrorCode() == mega::MegaError::API_OK)
    {
        mPreferences->setEmailAndGeneralSettings(QString::fromUtf8(request->getEmail()));

        dumpSession();

        fetchNodes(mEmail);
        if (!mPreferences->hasLoggedIn())
        {
            mPreferences->setHasLoggedIn(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
        }
        MegaSyncApp->onLoginFinished();
    }
    else
    {
        mPreferences->setAccountStateInGeneral(Preferences::STATE_LOGGED_FAILED);
        switch(e->getErrorCode())
        {
            case mega::MegaError::API_EINCOMPLETE:
            {
                setPasswordErrorMsg(tr("Please check your e-mail and click the link to confirm your account."));
                break;
            }
            case mega::MegaError::API_ETOOMANY:
            {
                setPasswordErrorMsg(tr("You have attempted to log in too many times.[BR]Please wait until %1 and try again.")
                                    .replace(QString::fromUtf8("[BR]"), QString::fromUtf8("\n"))
                                    .arg(QTime::currentTime().addSecs(3600).toString(QString::fromUtf8("hh:mm"))));
                break;
            }
            case mega::MegaError::API_EMFAREQUIRED:
            {
                mPassword = QString::fromUtf8(request->getPassword());
                setEmail(QString::fromLatin1(request->getEmail()));
                setState(LOGGING_IN_2FA_REQUIRED);
                break;
            }
            case mega::MegaError::API_ENOENT:
            {
                setPasswordErrorMsg(tr("Invalid email or password. Please try again."));
                break;
            }
            case mega::MegaError::API_EACCESS:
            {
                break;
            }
            default:
            {
                setPasswordErrorMsg(QCoreApplication::translate("MegaError", e->getErrorString()));
                break;
            }
        }
    }

    MegaSyncApp->onGlobalSyncStateChanged(mMegaApi);

    if(e->getErrorCode() != mega::MegaError::API_EMFAREQUIRED)
    {
        setEmailError(e->getErrorCode() != mega::MegaError::API_OK);
        setPasswordError(e->getErrorCode() != mega::MegaError::API_OK);
    }
}

void LoginController::onboardingFinished()
{
    SyncInfo::instance()->rewriteSyncSettings(); //write sync settings into user's preferences

    MegaSyncApp->onboardingFinished(false);
}

void LoginController::onAccountCreation(mega::MegaRequest* request, mega::MegaError* e)
{
    if(e->getErrorCode() == mega::MegaError::API_OK)
    {
        setEmail(QString::fromLatin1(request->getEmail()));
        mName = QString::fromUtf8(request->getName());
        mLastName = QString::fromUtf8(request->getText());
        EphemeralCredentials credentials;
        credentials.email = mEmail;
        credentials.sessionId = QString::fromUtf8(request->getSessionKey());
        mPreferences->setEphemeralCredentials(credentials);
        MegaSyncApp->getStatsEventHandler()->sendEvent(AppStatsEvents::EventType::ACC_CREATION_START);
        if (!mPreferences->accountCreationTime())
        {
                mPreferences->setAccountCreationTime(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);
        }
        setState(WAITING_EMAIL_CONFIRMATION);
    }
    else
    {
        setState(CREATING_ACCOUNT_FAILED);
        setCreateAccountErrorMsg(getRepeatedEmailMsg());
    }
}

void LoginController::onAccountCreationResume(mega::MegaRequest* request, mega::MegaError* e)
{
    Q_UNUSED(request)
    if(e->getErrorCode() == mega::MegaError::API_OK)
    {
        EphemeralCredentials credentials = mPreferences->getEphemeralCredentials();
        setEmail(credentials.email);
        setState(WAITING_EMAIL_CONFIRMATION);
    }
    else
    {
        mPreferences->removeEphemeralCredentials();
        setState(LOGGED_OUT);
    }
}

void LoginController::onEmailChanged(mega::MegaRequest* request, mega::MegaError* e)
{
    if(e->getErrorCode() != mega::MegaError::API_OK)
    {
        emit changeRegistrationEmailFinished(false, getRepeatedEmailMsg());
    }
    else
    {
        setEmail(QString::fromLatin1(request->getEmail()));
        EphemeralCredentials credentials = mPreferences->getEphemeralCredentials();
        credentials.email = mEmail;
        mPreferences->setEphemeralCredentials(credentials);
        emit changeRegistrationEmailFinished(true);
    }
}

void LoginController::onFetchNodes(mega::MegaRequest* request, mega::MegaError* e)
{
    Q_UNUSED(request)
    if (e->getErrorCode() == mega::MegaError::API_OK)
    {
        //Update/set root node
        MegaSyncApp->getRootNode(true); //TODO: move this to thread pool, notice that mRootNode is used below
        MegaSyncApp->getVaultNode(true);
        MegaSyncApp->getRubbishNode(true);

        mPreferences->setAccountStateInGeneral(Preferences::STATE_FETCHNODES_OK);
        mPreferences->setNeedsFetchNodesInGeneral(false);

        mProgress = 0; //sets guestdialog progressbar as indeterminate
        emit progressChanged();
        MegaSyncApp->onFetchNodesFinished();
    }
    else
    {
        setState(LOGGED_OUT);
        mPreferences->setAccountStateInGeneral(Preferences::STATE_FETCHNODES_FAILED);
        mPreferences->setNeedsFetchNodesInGeneral(true);
        mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Error fetching nodes: %1")
                                                                .arg(QString::fromUtf8(e->getErrorString())).toUtf8().constData());
    }

    if(e->getErrorCode() == mega::MegaError::API_OK)
    {
        if(!mPreferences->isOneTimeActionUserDone(Preferences::ONE_TIME_ACTION_ONBOARDING_SHOWN)
            && !(mPreferences->isFirstBackupDone() || mPreferences->isFirstSyncDone())) //Onboarding don´t has to be shown to users that
                                                                                        //doesn´t have one_time_action_onboarding_shown
        {                                                                               //and they have first backup or first sync done
            QmlDialogManager::instance()->openOnboardingDialog();
            setState(FETCH_NODES_FINISHED_ONBOARDING);
            mPreferences->setOneTimeActionUserDone(Preferences::ONE_TIME_ACTION_ONBOARDING_SHOWN, true);
        }
        else
        {
            setState(FETCH_NODES_FINISHED);
            onboardingFinished();
        }

        if(eventPendingStorage)
        {
            MegaSyncApp->onEvent(mMegaApi, eventPendingStorage.get());
            eventPendingStorage.reset();
        }
    }
}

void LoginController::onWhyAmIBlocked(mega::MegaRequest* request, mega::MegaError* e)
{
    if (e->getErrorCode() == mega::MegaError::API_OK
        && request->getNumber() == mega::MegaApi::ACCOUNT_NOT_BLOCKED)
    {
        // if we received a block before nodes were fetch,
        // we want to try again now that we are no longer blocked
        if (mState != FETCHING_NODES && !MegaSyncApp->getRootNode())
        {
            fetchNodes();
            //show fetchnodes page in new guestwidget
        }
    }
}

void LoginController::onAccountCreationCancel(mega::MegaRequest* request, mega::MegaError* e)
{
    Q_UNUSED(request)
    Q_UNUSED(e)
    mMegaApi->logout(false, nullptr); //megaapi->cancelCreateAccount doesn´t invalidate the ephemeral session.
    mPreferences->removeEphemeralCredentials();
    mEmail.clear();
    mPassword.clear();
    mName.clear();
    mLastName.clear();
    emit emailChanged();
    emit accountCreationCancelled();
}

void LoginController::onLogout(mega::MegaRequest* request, mega::MegaError* e)
{
    Q_UNUSED(e)
    Q_UNUSED(request)
    int errorCode = e->getErrorCode();
    int paramType =  request->getParamType();
    if (errorCode == mega::MegaError::API_EINCOMPLETE && paramType == mega::MegaError::API_ESSL)
    {
        return;
    }

    setState(LOGGED_OUT);
}

void LoginController::fetchNodes(const QString& email)
{
    assert(mState != FETCHING_NODES);
           // We need to load exclusions and migrate sync configurations from MEGAsync held cache, to SDK's
           // prior fetching nodes (when the SDK will resume syncing)

           // If we are loging into a new session of an account previously used in MEGAsync,
           // we will use the previous configurations stored in that user mPreferences
           // However, there is a case in which we are not able to do so at this point:
           // we don't know the user email.
           // That should only happen when trying to resume a session (using the session id stored in general mPreferences)
           // that didn't complete a fetch nodes (i.e. does not have mPreferences logged).
           // that can happen for blocked accounts.
           // Fortunately, the SDK can help us get the email of the session
    bool needFindingOutEmail = !mPreferences->logged() && email.isEmpty();

    auto loadMigrateAndFetchNodes = [this](const QString& email)
    {
        if (!mPreferences->logged() && email.isEmpty()) // I still couldn't get the the email: won't be able to access user settings
        {
            mMegaApi->fetchNodes();
        }
        else
        {
            loadSyncExclusionRules(email);
            migrateSyncConfToSdk(email); // this will produce the fetch nodes once done
        }
    };

    if (!needFindingOutEmail)
    {
        loadMigrateAndFetchNodes(email);
    }
    else // we will ask the SDK the email
    {
        auto listener = RequestListenerManager::instance().registerAndGetCustomFinishListener(
            this,
            [loadMigrateAndFetchNodes](mega::MegaRequest* request, mega::MegaError* e) {
                    QString email;

                    if (e->getErrorCode() == mega::MegaError::API_OK)
                    {
                        auto emailFromRequest = request->getEmail();
                        if (emailFromRequest)
                        {
                            email = QString::fromUtf8(emailFromRequest);
                        }
                    }

                    // in any case, proceed:
                    loadMigrateAndFetchNodes(email);
                });

        mMegaApi->getUserEmail(mMegaApi->getMyUserHandleBinary(), listener.get());
    }
}

void LoginController::migrateSyncConfToSdk(const QString& email)
{
    bool needsMigratingFromOldSession = !mPreferences->logged();
    assert(mPreferences->logged() || !email.isEmpty());


    int cachedBusinessState = 999;
    int cachedBlockedState = 999;
    int cachedStorageState = 999;

    auto oldCachedSyncs = mPreferences->readOldCachedSyncs(&cachedBusinessState, &cachedBlockedState, &cachedStorageState, email);
    auto oldCacheSyncsCount = oldCachedSyncs.size();
    if (oldCacheSyncsCount > 0)
    {
        if (cachedBusinessState == -2)
        {
            cachedBusinessState = 999;
        }
        if (cachedBlockedState == -2)
        {
            cachedBlockedState = 999;
        }
        if (cachedStorageState == mega::MegaApi::STORAGE_STATE_UNKNOWN)
        {
            cachedStorageState = 999;
        }

        mMegaApi->copyCachedStatus(cachedStorageState, cachedBlockedState, cachedBusinessState);
    }

    foreach(SyncData osd, oldCachedSyncs)
    {
        QString msg1 = QString::fromUtf8("Copying sync data to SDK cache: ") + osd.mLocalFolder
                           + QString::fromUtf8(". Name: ") + osd.mName;
        mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_DEBUG, msg1.toUtf8().constData());

        auto listener = RequestListenerManager::instance().registerAndGetCustomFinishListener(
            this,
            [this, osd, &oldCacheSyncsCount, needsMigratingFromOldSession, email](mega::MegaRequest* request, mega::MegaError* e) {
                if (e->getErrorCode() == mega::MegaError::API_OK)
                {
                    //preload the model with the restored configuration: that includes info that the SDK does not handle (e.g: syncID)
                    SyncInfo::instance()->pickInfoFromOldSync(osd, request->getParentHandle(), needsMigratingFromOldSession);
                    mPreferences->removeOldCachedSync(osd.mPos, email);
                }
                else
                {
                    QString msg2 = QString::fromUtf8("Failed to copy sync ") + osd.mLocalFolder
                                       + QString::fromUtf8(": ") + QString::fromUtf8(e->getErrorString());
                    mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_ERROR, msg2.toUtf8().constData());
                }

                --oldCacheSyncsCount;
                if (oldCacheSyncsCount == 0)//All syncs copied to sdk, proceed with fetchnodes
                {
                    mMegaApi->fetchNodes();
                }
            });

        mMegaApi->copySyncDataToCache(osd.mLocalFolder.toUtf8().constData(), osd.mName.toUtf8().constData(),
                                      osd.mMegaHandle, osd.mMegaFolder.toUtf8().constData(),
                                      osd.mLocalfp, osd.mEnabled, osd.mTemporarilyDisabled, listener.get());
    }

    if (oldCacheSyncsCount == 0)//No syncs to be copied to sdk, proceed with fetchnodes
    {
        mMegaApi->fetchNodes();
    }
}

void LoginController::loadSyncExclusionRules(const QString& email)
{
    assert(mPreferences->logged() || !email.isEmpty());

    // if not logged in & email provided, read old syncs from that user and load new-cache sync from prev session
    bool temporarilyLoggedPrefs = false;
    if (!mPreferences->logged() && !email.isEmpty())
    {
        temporarilyLoggedPrefs = mPreferences->enterUser(email);
        if (!temporarilyLoggedPrefs) // nothing to load
        {
            return;
        }
        mPreferences->loadExcludedSyncNames(); //to attend the corner case:
            // comming from old versions that didn't include some defaults

    }
    assert(mPreferences->logged()); //At this point mPreferences should be logged, just because you enterUser() or it was already logged

    if (!mPreferences->logged())
    {
        return;
    }
    const QStringList exclusions = mPreferences->getExcludedSyncNames();
    if(!exclusions.isEmpty())
    {
        std::vector<std::string> vExclusions;
        for (const QString& exclusion : exclusions)
        {
            vExclusions.push_back(exclusion.toUtf8().constData());
        }
        mMegaApi->setLegacyExcludedNames(&vExclusions);
    }
    const QStringList exclusionPaths = mPreferences->getExcludedSyncPaths();
    if(!exclusionPaths.isEmpty())
    {
        std::vector<std::string> vExclusionPaths;
        for (const QString& exclusionPath : exclusionPaths)
        {
            vExclusionPaths.push_back(exclusionPath.toUtf8().constData());
        }
        mMegaApi->setLegacyExcludedPaths(&vExclusionPaths);
    }
    if (mPreferences->lowerSizeLimit())
    {
        mMegaApi->setLegacyExclusionLowerSizeLimit(computeExclusionSizeLimit(mPreferences->lowerSizeLimitValue(), mPreferences->lowerSizeLimitUnit()));
    }

    if (mPreferences->upperSizeLimit())
    {
        mMegaApi->setLegacyExclusionUpperSizeLimit(computeExclusionSizeLimit(mPreferences->upperSizeLimitValue(), mPreferences->upperSizeLimitUnit()));
    }

    if (temporarilyLoggedPrefs)
    {
        mPreferences->leaveUser();
    }
}

void LoginController::dumpSession()
{
    std::unique_ptr<char []> session(mMegaApi->dumpSession());
    if (session)
    {
        mPreferences->setSession(QString::fromUtf8(session.get()));
    }
}

QString LoginController::getRepeatedEmailMsg()
{
    return tr("Email address already in use.");
}

void LoginController::setEmail(const QString& email)
{
    if(mEmail != email)
    {
        mEmail = email;
        emit emailChanged();
    }
}

unsigned long long LoginController::computeExclusionSizeLimit(const unsigned long long& sizeLimitValue, const int unit)
{
    const double bytesPerKb = 1024;
    const double sizeLimitPower = pow(bytesPerKb, static_cast<double>(unit));
    return sizeLimitValue * static_cast<unsigned long long>(trunc(sizeLimitPower));
}

void LoginController::runConnectivityCheck()
{
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::NoProxy);
    if (mPreferences->proxyType() == Preferences::PROXY_TYPE_CUSTOM)
    {
        int proxyProtocol = mPreferences->proxyProtocol();
        switch (proxyProtocol)
        {
            case Preferences::PROXY_PROTOCOL_SOCKS5H:
                proxy.setType(QNetworkProxy::Socks5Proxy);
                break;
            default:
                proxy.setType(QNetworkProxy::HttpProxy);
                break;
        }

        proxy.setHostName(mPreferences->proxyServer());
        proxy.setPort(quint16(mPreferences->proxyPort()));
        if (mPreferences->proxyRequiresAuth())
        {
            proxy.setUser(mPreferences->getProxyUsername());
            proxy.setPassword(mPreferences->getProxyPassword());
        }
    }
    else if (mPreferences->proxyType() == mega::MegaProxy::PROXY_AUTO)
    {
        std::unique_ptr<mega::MegaProxy> autoProxy(mMegaApi->getAutoProxySettings());
        if (autoProxy && autoProxy->getProxyType() == mega::MegaProxy::PROXY_CUSTOM)
        {
            QString proxyURL = QString::fromUtf8(autoProxy->getProxyURL());

            QStringList parts = proxyURL.split(QString::fromUtf8("://"));
            if (parts.size() == 2 && parts[0].startsWith(QString::fromUtf8("socks")))
            {
                proxy.setType(QNetworkProxy::Socks5Proxy);
            }
            else
            {
                proxy.setType(QNetworkProxy::HttpProxy);
            }

            QStringList arguments = parts[parts.size()-1].split(QString::fromUtf8(":"));
            if (arguments.size() == 2)
            {
                proxy.setHostName(arguments[0]);
                proxy.setPort(quint16(arguments[1].toInt()));
            }
        }
    }

    ConnectivityChecker *connectivityChecker = new ConnectivityChecker(Preferences::PROXY_TEST_URL);
    connectivityChecker->setProxy(proxy);
    connectivityChecker->setTestString(Preferences::PROXY_TEST_SUBSTRING);
    connectivityChecker->setTimeout(Preferences::PROXY_TEST_TIMEOUT_MS);

    connect(connectivityChecker, &ConnectivityChecker::testFinished, this,
             &LoginController::onConnectivityCheckFinished, Qt::UniqueConnection);

    connectivityChecker->startCheck();
    mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_INFO, "Running connectivity test...");
}


void LoginController::onConnectivityCheckFinished(bool success)
{
    sender()->deleteLater();
    if(success)
    {
        mega::MegaApi::log(mega::MegaApi::LOG_LEVEL_INFO, "Connectivity test finished OK");
    }
    else
    {
        MegaSyncApp->showErrorMessage(tr("MEGAsync is unable to connect. Please check your "
                                           "Internet connectivity and local firewall configuration. "
                                           "Note that most antivirus software includes a firewall."));
    }
}

FastLoginController::FastLoginController(QObject *parent)
    : LoginController(parent)
{

}

bool FastLoginController::fastLogin()
{
    QString session = mPreferences->getSession();
    if(session.size())
    {
        mMegaApi->fastLogin(session.toUtf8().constData());
        return true;
    }
    return false;
}

void FastLoginController::onLogin(mega::MegaRequest* request, mega::MegaError* e)
{
    Q_UNUSED(request)
    //This prevents to handle logins in the initial setup wizard
    if (mPreferences->logged())
    {
        Platform::getInstance()->prepareForSync();
        int errorCode = e->getErrorCode();
        if (errorCode == mega::MegaError::API_OK)
        {
            MegaSyncApp->onLoginFinished();
            if (!mPreferences->getSession().isEmpty())
            {
                //Successful login, fetch nodes
                fetchNodes();
                return;
            }
        }
        else if (errorCode != mega::MegaError::API_ESID && errorCode != mega::MegaError::API_ESSL)
        //Invalid session or public key, already managed in TYPE_LOGOUT
        {
            QMegaMessageBox::MessageBoxInfo msgInfo;
            msgInfo.title = MegaSyncApp->getMEGAString();
            msgInfo.text = tr("Login error: %1").arg(QCoreApplication::translate("MegaError", e->getErrorString()));

            QMegaMessageBox::warning(msgInfo);
        }

        //Wrong login -> logout
        MegaSyncApp->unlink(true);
    }
    MegaSyncApp->onGlobalSyncStateChanged(mMegaApi);
}

void FastLoginController::onboardingFinished()
{
    MegaSyncApp->onboardingFinished(true);
}

LogoutController::LogoutController(QObject* parent)
    : QObject(parent)
      , mMegaApi(MegaSyncApp->getMegaApi())
      , mLoginInWithoutSession(false)
{   
    ListenerCallbacks lcInfo{
        this,
        std::bind(&LogoutController::onRequestStart, this, std::placeholders::_1),
        std::bind(&LogoutController::onRequestFinish, this, std::placeholders::_1, std::placeholders::_2)
    };

    mDelegateListener = RequestListenerManager::instance().registerAndGetListener(lcInfo);
    mMegaApi->addRequestListener(mDelegateListener.get());
}

LogoutController::~LogoutController()
{
}

void LogoutController::onRequestFinish(mega::MegaRequest* request, mega::MegaError* e)
{
    if(request->getType() == mega::MegaRequest::TYPE_LOGIN)
    {
        mLoginInWithoutSession = false;
    }

    if(request->getType() != mega::MegaRequest::TYPE_LOGOUT)
    {
        return;
    }
    int errorCode = e->getErrorCode();
    int paramType =  request->getParamType();
    if (errorCode || paramType)
    {
        if (errorCode == mega::MegaError::API_EINCOMPLETE && paramType == mega::MegaError::API_ESSL && !mLoginInWithoutSession)
        {
            //Typical case: Connecting from a public wifi when the wifi sends you to a landing page
            //SDK cannot connect through SSL securely and asks MEGA Desktop to log out

                   //In previous versions, the user was asked to continue with a warning about a MITM risk.
                   //One of the options was disabling the public key pinning to continue working as usual
                   //This option was to risky and the solution taken was silently retry reconnection

                   // Retry while enforcing key pinning silently
            mMegaApi->retryPendingConnections();
            return;
        }

        if (paramType == mega::MegaError::API_ESID)
        {
            QMegaMessageBox::MessageBoxInfo msgInfo;
            msgInfo.title = MegaSyncApp->getMEGAString();
            msgInfo.text = tr("You have been logged out on this computer from another location");
            msgInfo.ignoreCloseAll = true;

            QMegaMessageBox::information(msgInfo);
        }
        else if (paramType == mega::MegaError::API_ESSL)
        {
            QMegaMessageBox::MessageBoxInfo msgInfo;
            msgInfo.title = MegaSyncApp->getMEGAString();
            msgInfo.text = tr("Our SSL key can't be verified. You could be affected by a man-in-the-middle attack or your antivirus software "
                               "could be intercepting your communications and causing this problem. Please disable it and try again.")
                           + QString::fromUtf8(" (Issuer: %1)").arg(QString::fromUtf8(request->getText() ? request->getText() : "Unknown"));
            msgInfo.ignoreCloseAll = true;

            QMegaMessageBox::critical(msgInfo);
            mMegaApi->localLogout();
        }
        else if (paramType != mega::MegaError::API_EACCESS && paramType != mega::MegaError::API_EBLOCKED)
        {
            QMegaMessageBox::MessageBoxInfo msgInfo;
            msgInfo.title = MegaSyncApp->getMEGAString();
            if(errorCode != mega::MegaError::API_OK)
            {
                msgInfo.text =tr("You have been logged out because of this error: %1").arg(QCoreApplication::translate("MegaError", e->getErrorString()));
            }
            else
            {
                Text::Link link(QString::fromLatin1("mailto:support@mega.nz"));
                QString text = tr("You have been logged out. Please contact [A]support@mega.nz[/A] if this issue persists.");
                link.process(text);
                msgInfo.text = text;
                msgInfo.textFormat = Qt::RichText;
            }
            msgInfo.ignoreCloseAll = true;

            QMegaMessageBox::information(msgInfo);
        }
        MegaSyncApp->unlink();
    }

           //Check for any sync disabled by logout to warn user on next login with user&password
    const auto syncSettings (SyncInfo::instance()->getAllSyncSettings());
    auto isErrorLoggedOut = [](const std::shared_ptr<SyncSettings> s) {return s->getError() == mega::MegaSync::LOGGED_OUT;};
    if (std::any_of(syncSettings.cbegin(), syncSettings.cend(), isErrorLoggedOut))
    {
        Preferences::instance()->setNotifyDisabledSyncsOnLogin(true);
    }

    emit logout(!request->getFlag());
    mLoginInWithoutSession = false;
}

void LogoutController::onRequestStart(mega::MegaRequest *request)
{
    if(request->getType() == mega::MegaRequest::TYPE_LOGIN && !request->getSessionKey())
    {
        mLoginInWithoutSession = true;
    }
}
