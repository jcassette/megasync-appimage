#include "Preferences.h"
#include "Version.h"
#include "Platform.h"
#include "FullName.h"
#include "StatsEventHandler.h"

#include <QDesktopServices>
#include <QDir>
#include <assert.h>

using namespace mega;

#ifdef WIN32
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif
const char Preferences::CLIENT_KEY[] = "FhMgXbqb";
const QString Preferences::USER_AGENT = QString::fromUtf8("%1/%2").arg(QString::fromUtf8(VER_FILEDESCRIPTION_STR),
                                                                       QString::fromUtf8(VER_PRODUCTVERSION_STR));
const int Preferences::VERSION_CODE = VER_FILEVERSION_CODE;
const int Preferences::BUILD_ID = VER_BUILD_ID;
// VER_PRODUCTVERSION_STR is "W.X.Y.Z". Drop the last number to keep "W.X.Y"
const QString Preferences::VERSION_STRING = QString::fromUtf8(VER_PRODUCTVERSION_STR).left(QString::fromUtf8(VER_PRODUCTVERSION_STR).lastIndexOf(QLatin1Char('.')));
QString Preferences::SDK_ID = QString::fromUtf8(VER_SDK_ID);
const QString Preferences::CHANGELOG = QString::fromUtf8(VER_CHANGES_NOTES);

const QString Preferences::TRANSLATION_FOLDER = QString::fromLatin1("://translations/");
const QString Preferences::TRANSLATION_PREFIX = QString::fromLatin1("MEGASyncStrings_");

int Preferences::STATE_REFRESH_INTERVAL_MS        = 10000;
int Preferences::NETWORK_REFRESH_INTERVAL_MS      = 30000;
int Preferences::FINISHED_TRANSFER_REFRESH_INTERVAL_MS        = 10000;

long long Preferences::OQ_DIALOG_INTERVAL_MS = 604800000; // 7 daysm
long long Preferences::OQ_NOTIFICATION_INTERVAL_MS = 129600000; // 36 hours
long long Preferences::ALMOST_OQ_UI_MESSAGE_INTERVAL_MS = 259200000; // 72 hours
long long Preferences::OQ_UI_MESSAGE_INTERVAL_MS = 129600000; // 36 hours
long long Preferences::PAYWALL_NOTIFICATION_INTERVAL_MS = 86400000; //24 hours
long long Preferences::USER_INACTIVITY_MS = 20000; // 20 secs

std::chrono::milliseconds Preferences::OVER_QUOTA_DIALOG_DISABLE_DURATION{std::chrono::hours(7*24)};
std::chrono::milliseconds Preferences::OVER_QUOTA_OS_NOTIFICATION_DISABLE_DURATION{std::chrono::hours(36)};
std::chrono::milliseconds Preferences::OVER_QUOTA_UI_ALERT_DISABLE_DURATION{std::chrono::hours(36)};
std::chrono::milliseconds Preferences::ALMOST_OVER_QUOTA_UI_ALERT_DISABLE_DURATION{std::chrono::hours(72)};
std::chrono::milliseconds Preferences::ALMOST_OVER_QUOTA_OS_NOTIFICATION_DISABLE_DURATION{std::chrono::hours(36)};
std::chrono::milliseconds Preferences::OVER_QUOTA_ACTION_DIALOGS_DISABLE_TIME{std::chrono::hours{12}};

long long Preferences::MIN_UPDATE_STATS_INTERVAL              = 300000;
long long Preferences::MIN_UPDATE_CLEANING_INTERVAL_MS        = 7200000;
long long Preferences::MIN_UPDATE_NOTIFICATION_INTERVAL_MS    = 172800000;
long long Preferences::MIN_REBOOT_INTERVAL_MS                 = 300000;
long long Preferences::MIN_EXTERNAL_NODES_WARNING_MS          = 60000;
long long Preferences::MIN_TRANSFER_NOTIFICATION_INTERVAL_MS  = 10000;

int Preferences::UPDATE_INITIAL_DELAY_SECS                    = 60;
int Preferences::UPDATE_RETRY_INTERVAL_SECS                   = 7200;
int Preferences::UPDATE_TIMEOUT_SECS                          = 600;
int Preferences::MAX_LOGIN_TIME_MS                            = 40000;
int Preferences::PROXY_TEST_TIMEOUT_MS                        = 10000;
unsigned int Preferences::MAX_IDLE_TIME_MS                    = 600000;
unsigned int Preferences::MAX_COMPLETED_ITEMS                 = 1000;

unsigned int Preferences::MUTEX_STEALER_MS                    = 0;
unsigned int Preferences::MUTEX_STEALER_PERIOD_MS             = 0;
unsigned int Preferences::MUTEX_STEALER_PERIOD_ONLY_ONCE      = 0;

const unsigned short Preferences::HTTP_PORT                   = 6341;

const QString Preferences::FINDER_EXT_BUNDLE_ID = QString::fromUtf8("mega.mac.MEGAShellExtFinder");
QString Preferences::BASE_URL = QString::fromLatin1("https://mega.nz");
QString Preferences::BASE_MEGA_IO_URL = QString::fromLatin1("https://mega.io");
QString Preferences::BASE_MEGA_HELP_URL = QString::fromLatin1("https://help.mega.io");
const QStringList Preferences::HTTPS_ALLOWED_ORIGINS = QStringList() << Preferences::BASE_URL
                                                                     << QLatin1String("https://mega.co.nz")
                                                                     << QLatin1String("chrome-extension://*")
                                                                     << QLatin1String("moz-extension://*")
                                                                     << QLatin1String("edge-extension://*");

bool Preferences::HTTPS_ORIGIN_CHECK_ENABLED = true;

#ifdef WIN32
    #ifdef _WIN64
        const QString Preferences::UPDATE_CHECK_URL             = QString::fromUtf8("http://g.static.mega.co.nz/upd/wsync64/v.txt");
    #else
        const QString Preferences::UPDATE_CHECK_URL             = QString::fromUtf8("http://g.static.mega.co.nz/upd/wsync/v.txt");
    #endif
#else
    #if defined(__arm64__)
        const QString Preferences::UPDATE_CHECK_URL                 = QString::fromUtf8("http://g.static.mega.co.nz/upd/msyncarm64/v.txt");
    #else
        const QString Preferences::UPDATE_CHECK_URL                 = QString::fromUtf8("http://g.static.mega.co.nz/upd/msyncv2/v.txt"); //Using msyncv2 to serve new updates and avoid keeping loader leftovers
    #endif
#endif

const char Preferences::UPDATE_PUBLIC_KEY[] = "EACTzXPE8fdMhm6LizLe1FxV2DncybVh2cXpW3momTb8tpzRNT833r1RfySz5uHe8gdoXN1W0eM5Bk8X-LefygYYDS9RyXrRZ8qXrr9ITJ4r8ATnFIEThO5vqaCpGWTVi5pOPI5FUTJuhghVKTyAels2SpYT5CmfSQIkMKv7YVldaV7A-kY060GfrNg4--ETyIzhvaSZ_jyw-gmzYl_dwfT9kSzrrWy1vQG8JPNjKVPC4MCTZJx9SNvp1fVi77hhgT-Mc5PLcDIfjustlJkDBHtmGEjyaDnaWQf49rGq94q23mLc56MSjKpjOR1TtpsCY31d1Oy2fEXFgghM0R-1UkKswVuWhEEd8nO2PimJOl4u9ZJ2PWtJL1Ro0Hlw9OemJ12klIAxtGV-61Z60XoErbqThwWT5Uu3D2gjK9e6rL9dufSoqjC7UA2C0h7KNtfUcUHw0UWzahlR8XBNFXaLWx9Z8fRtA_a4seZcr0AhIA7JdQG5i8tOZo966KcFnkU77pfQTSprnJhCfEmYbWm9EZA122LJBWq2UrSQQN3pKc9goNaaNxy5PYU1yXyiAfMVsBDmDonhRWQh2XhdV-FWJ3rOGMe25zOwV4z1XkNBuW4T1JF2FgqGR6_q74B2ccFC8vrNGvlTEcs3MSxTI_EKLXQvBYy7hxG8EPUkrMVCaWzzTQAFEQ";
const QString Preferences::CRASH_REPORT_URL                 = QString::fromUtf8("http://g.api.mega.co.nz/hb?crashdump");
const QString Preferences::UPDATE_FOLDER_NAME               = QString::fromLatin1("update");
const QString Preferences::UPDATE_BACKUP_FOLDER_NAME        = QString::fromLatin1("backup");
const QString Preferences::PROXY_TEST_URL                   = QString::fromUtf8("https://g.api.mega.co.nz/cs");
const QString Preferences::PROXY_TEST_SUBSTRING             = QString::fromUtf8("-2");
const QString Preferences::syncsGroupKey            = QString::fromLatin1("Syncs");
const QString Preferences::syncsGroupByTagKey       = QString::fromLatin1("SyncsByTag");
const QString Preferences::currentAccountKey        = QString::fromLatin1("currentAccount");
const QString Preferences::currentAccountStatusKey  = QString::fromLatin1("currentAccountStatus");
const QString Preferences::needsFetchNodesKey       = QString::fromLatin1("needsFetchNodes");
const QString Preferences::emailKey                 = QString::fromLatin1("email");
const QString Preferences::firstNameKey             = QString::fromLatin1("firstName");
const QString Preferences::lastNameKey              = QString::fromLatin1("lastName");
const QString Preferences::totalStorageKey          = QString::fromLatin1("totalStorage");
const QString Preferences::usedStorageKey           = QString::fromLatin1("usedStorage");
const QString Preferences::cloudDriveStorageKey     = QString::fromLatin1("cloudDriveStorage");
const QString Preferences::vaultStorageKey          = QString::fromLatin1("vaultStorage");
const QString Preferences::rubbishStorageKey        = QString::fromLatin1("rubbishStorage");
const QString Preferences::inShareStorageKey        = QString::fromLatin1("inShareStorage");
const QString Preferences::versionsStorageKey        = QString::fromLatin1("versionsStorage");
const QString Preferences::cloudDriveFilesKey       = QString::fromLatin1("cloudDriveFiles");
const QString Preferences::vaultFilesKey            = QString::fromLatin1("vaultFiles");
const QString Preferences::rubbishFilesKey          = QString::fromLatin1("rubbishFiles");
const QString Preferences::inShareFilesKey          = QString::fromLatin1("inShareFiles");
const QString Preferences::cloudDriveFoldersKey     = QString::fromLatin1("cloudDriveFolders");
const QString Preferences::vaultFoldersKey          = QString::fromLatin1("vaultFolders");
const QString Preferences::rubbishFoldersKey        = QString::fromLatin1("rubbishFolders");
const QString Preferences::inShareFoldersKey        = QString::fromLatin1("inShareFolders");
const QString Preferences::totalBandwidthKey        = QString::fromLatin1("totalBandwidth");
const QString Preferences::usedBandwidthIntervalKey        = QString::fromLatin1("usedBandwidthInterval");
const QString Preferences::usedBandwidthKey         = QString::fromLatin1("usedBandwidth");

const QString Preferences::overStorageDialogExecutionKey = QString::fromLatin1("overStorageDialogExecution");
const QString Preferences::overStorageNotificationExecutionKey = QString::fromLatin1("overStorageNotificationExecution");
const QString Preferences::almostOverStorageNotificationExecutionKey = QString::fromLatin1("almostOverStorageNotificationExecution");
const QString Preferences::payWallNotificationExecutionKey = QString::fromLatin1("payWallNotificationExecution");
const QString Preferences::almostOverStorageDismissExecutionKey = QString::fromLatin1("almostOverStorageDismissExecution");
const QString Preferences::overStorageDismissExecutionKey = QString::fromLatin1("overStorageDismissExecution");
const QString Preferences::storageStateQKey = QString::fromLatin1("storageStopLight");
const QString Preferences::businessStateQKey = QString::fromLatin1("businessState");
const QString Preferences::blockedStateQKey = QString::fromLatin1("blockedState");

const QString Preferences::transferOverQuotaDialogLastExecutionKey = QString::fromLatin1("transferOverQuotaDialogLastExecution");
const QString Preferences::transferOverQuotaOsNotificationLastExecutionKey = QString::fromLatin1("transferOverQuotaOsNotificationLastExecution");
const QString Preferences::transferAlmostOverQuotaOsNotificationLastExecutionKey = QString::fromLatin1("transferAlmostOverQuotaOsNotificationLastExecution");
const QString Preferences::transferAlmostOverQuotaUiAlertLastExecutionKey = QString::fromLatin1("transferAlmostOverQuotaUiAlertLastExecution");
const QString Preferences::transferOverQuotaUiAlertLastExecutionKey = QString::fromLatin1("transferOverQuotaUiAlertDisableUntil");

const QString Preferences::transferOverQuotaSyncDialogLastExecutionKey = QString::fromLatin1("transferOverQuotaSyncDialogLastExecution");
const QString Preferences::transferOverQuotaDownloadsDialogLastExecutionKey = QString::fromLatin1("transferOverQuotaDownloadsDialogLastExecution");
const QString Preferences::transferOverQuotaImportLinksDialogLastExecutionKey = QString::fromLatin1("transferOverQuotaImportLinksDialogLastExecution");
const QString Preferences::transferOverQuotaStreamDialogLastExecutionKey = QString::fromLatin1("transferOverQuotaStreamDialogLastExecution");
const QString Preferences::storageOverQuotaUploadsDialogLastExecutionKey = QString::fromLatin1("storageOverQuotaUploadsDialogLastExecution");
const QString Preferences::storageOverQuotaSyncsDialogLastExecutionKey = QString::fromLatin1("storageOverQuotaSyncsDialogLastExecution");

const bool Preferences::defaultShowNotifications = true;

const bool Preferences::defaultDeprecatedNotifications      = true;
const QString Preferences::showDeprecatedNotificationsKey   = QString::fromLatin1("showNotifications");

//Stalled Issues
const Preferences::StalledIssuesModeType Preferences::defaultStalledIssuesMode = Preferences::StalledIssuesModeType::Smart;
const QString Preferences::stalledIssuesModeKey   = QString::fromLatin1("stalledIssuesSmartMode");

const QString Preferences::stalledIssuesEventDateKey = QString::fromLatin1("stalledIssuesEventDate");
//End of Stalled Issues

const QString Preferences::accountTypeKey           = QString::fromLatin1("accountType");
const QString Preferences::proExpirityTimeKey       = QString::fromLatin1("proExpirityTime");
const QString Preferences::startOnStartupKey        = QString::fromLatin1("startOnStartup");
const QString Preferences::languageKey              = QString::fromLatin1("language");
const QString Preferences::updateAutomaticallyKey   = QString::fromLatin1("updateAutomatically");
const QString Preferences::uploadLimitKBKey         = QString::fromLatin1("uploadLimitKB");
const QString Preferences::downloadLimitKBKey       = QString::fromLatin1("downloadLimitKB");
const QString Preferences::parallelUploadConnectionsKey       = QString::fromLatin1("parallelUploadConnections");
const QString Preferences::parallelDownloadConnectionsKey     = QString::fromLatin1("parallelDownloadConnections");

const QString Preferences::upperSizeLimitKey        = QString::fromLatin1("upperSizeLimit");
const QString Preferences::lowerSizeLimitKey        = QString::fromLatin1("lowerSizeLimit");

const QString Preferences::lastCustomStreamingAppKey    = QString::fromLatin1("lastCustomStreamingApp");

const QString Preferences::upperSizeLimitValueKey       = QString::fromLatin1("upperSizeLimitValue");
const QString Preferences::lowerSizeLimitValueKey       = QString::fromLatin1("lowerSizeLimitValue");
const QString Preferences::upperSizeLimitUnitKey        = QString::fromLatin1("upperSizeLimitUnit");
const QString Preferences::lowerSizeLimitUnitKey        = QString::fromLatin1("lowerSizeLimitUnit");


const QString Preferences::cleanerDaysLimitKey       = QString::fromLatin1("cleanerDaysLimit");
const QString Preferences::cleanerDaysLimitValueKey  = QString::fromLatin1("cleanerDaysLimitValue");

const QString Preferences::folderPermissionsKey         = QString::fromLatin1("folderPermissions");
const QString Preferences::filePermissionsKey           = QString::fromLatin1("filePermissions");

const QString Preferences::proxyTypeKey             = QString::fromLatin1("proxyType");
const QString Preferences::proxyProtocolKey         = QString::fromLatin1("proxyProtocol");
const QString Preferences::proxyServerKey           = QString::fromLatin1("proxyServer");
const QString Preferences::proxyPortKey             = QString::fromLatin1("proxyPort");
const QString Preferences::proxyRequiresAuthKey     = QString::fromLatin1("proxyRequiresAuth");
const QString Preferences::proxyUsernameKey         = QString::fromLatin1("proxyUsername");
const QString Preferences::proxyPasswordKey         = QString::fromLatin1("proxyPassword");
const QString Preferences::configuredSyncsKey       = QString::fromLatin1("configuredSyncs");
const QString Preferences::syncNameKey              = QString::fromLatin1("syncName");
const QString Preferences::syncIdKey                = QString::fromLatin1("syncId");
const QString Preferences::localFolderKey           = QString::fromLatin1("localFolder");
const QString Preferences::megaFolderKey            = QString::fromLatin1("megaFolder");
const QString Preferences::megaFolderHandleKey      = QString::fromLatin1("megaFolderHandle");
const QString Preferences::folderActiveKey          = QString::fromLatin1("folderActive");
const QString Preferences::temporaryInactiveKey     = QString::fromLatin1("temporaryInactive");
const QString Preferences::downloadFolderKey        = QString::fromLatin1("downloadFolder");
const QString Preferences::uploadFolderKey          = QString::fromLatin1("uploadFolder");
const QString Preferences::importFolderKey          = QString::fromLatin1("importFolder");
const QString Preferences::hasDefaultUploadFolderKey    = QString::fromLatin1("hasDefaultUploadFolder");
const QString Preferences::hasDefaultDownloadFolderKey  = QString::fromLatin1("hasDefaultDownloadFolder");
const QString Preferences::localFingerprintKey      = QString::fromLatin1("localFingerprint");
const QString Preferences::isCrashedKey             = QString::fromLatin1("isCrashed");
const QString Preferences::wasPausedKey             = QString::fromLatin1("wasPaused");
const QString Preferences::wasUploadsPausedKey      = QString::fromLatin1("wasUploadsPaused");
const QString Preferences::wasDownloadsPausedKey    = QString::fromLatin1("wasDownloadsPaused");
const QString Preferences::lastExecutionTimeKey     = QString::fromLatin1("lastExecutionTime");
const QString Preferences::excludedSyncNamesKey     = QString::fromLatin1("excludedSyncNames");
const QString Preferences::excludedSyncPathsKey     = QString::fromLatin1("excludedSyncPaths");
const QString Preferences::lastVersionKey           = QString::fromLatin1("lastVersion");
const QString Preferences::lastStatsRequestKey      = QString::fromLatin1("lastStatsRequest");
const QString Preferences::lastUpdateTimeKey        = QString::fromLatin1("lastUpdateTime");
const QString Preferences::lastUpdateVersionKey     = QString::fromLatin1("lastUpdateVersion");
const QString Preferences::previousCrashesKey       = QString::fromLatin1("previousCrashes");
const QString Preferences::lastRebootKey            = QString::fromLatin1("lastReboot");
const QString Preferences::lastExitKey              = QString::fromLatin1("lastExit");
const QString Preferences::disableOverlayIconsKey   = QString::fromLatin1("disableOverlayIcons");
const QString Preferences::disableFileVersioningKey = QString::fromLatin1("disableFileVersioning");
const QString Preferences::disableLeftPaneIconsKey  = QString::fromLatin1("disableLeftPaneIcons");
const QString Preferences::sessionKey               = QString::fromLatin1("session");
const QString Preferences::ephemeralSessionKey      = QString::fromLatin1("ephemeralSession");
const QString Preferences::firstStartDoneKey        = QString::fromLatin1("firstStartDone");
const QString Preferences::firstSyncDoneKey         = QString::fromLatin1("firstSyncDone");
const QString Preferences::firstBackupDoneKey       = QString::fromLatin1("firstBackupDone");
const QString Preferences::firstFileSyncedKey       = QString::fromLatin1("firstFileSynced");
const QString Preferences::firstFileBackedUpKey     = QString::fromLatin1("firstFileBackedUp");
const QString Preferences::firstWebDownloadKey      = QString::fromLatin1("firstWebclientDownload");
const QString Preferences::fatWarningShownKey       = QString::fromLatin1("fatWarningShown");
const QString Preferences::installationTimeKey      = QString::fromLatin1("installationTime");
const QString Preferences::accountCreationTimeKey   = QString::fromLatin1("accountCreationTime");
const QString Preferences::hasLoggedInKey           = QString::fromLatin1("hasLoggedIn");
const QString Preferences::useHttpsOnlyKey          = QString::fromLatin1("useHttpsOnly");
const QString Preferences::SSLcertificateExceptionKey  = QString::fromLatin1("SSLcertificateException");
const QString Preferences::maxMemoryUsageKey        = QString::fromLatin1("maxMemoryUsage");
const QString Preferences::maxMemoryReportTimeKey   = QString::fromLatin1("maxMemoryReportTime");
const QString Preferences::oneTimeActionDoneKey     = QString::fromLatin1("oneTimeActionDone");
const QString Preferences::oneTimeActionUserDoneKey     = QString::fromLatin1("oneTimeActionUserDone");
const QString Preferences::transferIdentifierKey    = QString::fromLatin1("transferIdentifier");
const QString Preferences::lastPublicHandleKey      = QString::fromLatin1("lastPublicHandle");
const QString Preferences::lastPublicHandleTimestampKey = QString::fromLatin1("lastPublicHandleTimestamp");
const QString Preferences::lastPublicHandleTypeKey = QString::fromLatin1("lastPublicHandleType");
const QString Preferences::disabledSyncsKey = QString::fromLatin1("disabledSyncs");
const QString Preferences::neverCreateLinkKey       = QString::fromUtf8("neverCreateLink");
const QString Preferences::notifyDisabledSyncsKey = QString::fromLatin1("notifyDisabledSyncs");
const QString Preferences::importMegaLinksEnabledKey = QString::fromLatin1("importMegaLinksEnabled");
const QString Preferences::downloadMegaLinksEnabledKey = QString::fromLatin1("downloadMegaLinksEnabled");
const QString Preferences::systemTrayPromptSuppressed = QString::fromLatin1("systemTrayPromptSuppressed");
const QString Preferences::systemTrayLastPromptTimestamp = QString::fromLatin1("systemTrayLastPromptTimestamp");
const QString Preferences::lastDailyStatTimeKey = QString::fromLatin1("lastDailyStatTimeKey");
const QString Preferences::askOnExclusionRemove = QString::fromLatin1("askOnExclusionRemove");
const QString Preferences::themeKey = QString::fromLatin1("themeType");

//Sleep settings
const QString Preferences::awakeIfActiveKey = QString::fromLatin1("sleepIfInactiveEnabledKey");
const bool Preferences::defaultAwakeIfActive = false;

const bool Preferences::defaultStartOnStartup       = true;
const bool Preferences::defaultUpdateAutomatically  = true;
const bool Preferences::defaultUpperSizeLimit       = false;
const bool Preferences::defaultLowerSizeLimit       = false;

const bool Preferences::defaultCleanerDaysLimit     = true;

const bool Preferences::defaultUseHttpsOnly         = true;
const bool Preferences::defaultSSLcertificateException = false;
const int  Preferences::defaultUploadLimitKB        = -1;
const int  Preferences::defaultDownloadLimitKB      = 0;
const long long Preferences::defaultTimeStamp       = 0;

//The default appDataId starts from 1, as 0 will be used for invalid appDataId
const unsigned long long  Preferences::defaultTransferIdentifier            = 1;
const int  Preferences::defaultParallelUploadConnections                    = 3;
const int  Preferences::defaultParallelDownloadConnections                  = 4;
const unsigned long long  Preferences::defaultUpperSizeLimitValue           = 1; //Input UI range 1-9999. Use 1 as default value
const unsigned long long  Preferences::defaultLowerSizeLimitValue           = 1; //Input UI range 1-9999. Use 1 as default value
const int  Preferences::defaultCleanerDaysLimitValue                        = 30;
const int Preferences::defaultLowerSizeLimitUnit =  Preferences::MEGA_BYTE_UNIT;
const int Preferences::defaultUpperSizeLimitUnit =  Preferences::MEGA_BYTE_UNIT;
const int Preferences::defaultFolderPermissions = 0;
const int Preferences::defaultFilePermissions   = 0;
#ifdef WIN32
const int  Preferences::defaultProxyType            = Preferences::PROXY_TYPE_AUTO;
#else
const int  Preferences::defaultProxyType            = Preferences::PROXY_TYPE_NONE;
#endif
const int  Preferences::defaultProxyProtocol        = Preferences::PROXY_PROTOCOL_HTTP;
const QString  Preferences::defaultProxyServer      = QString::fromLatin1("127.0.0.1");
const unsigned short Preferences::defaultProxyPort  = 8080;
const bool Preferences::defaultProxyRequiresAuth    = false;
const QString Preferences::defaultProxyUsername     = QString::fromLatin1("");
const QString Preferences::defaultProxyPassword     = QString::fromLatin1("");

const int  Preferences::defaultAccountStatus      = STATE_NOT_INITIATED;
const bool  Preferences::defaultNeedsFetchNodes   = false;

const bool  Preferences::defaultNeverCreateLink   = false;

const bool  Preferences::defaultImportMegaLinksEnabled = true;
const bool  Preferences::defaultDownloadMegaLinksEnabled = true;
const bool Preferences::defaultSystemTrayPromptSuppressed = false;
const Preferences::ThemeType Preferences::defaultTheme = Preferences::ThemeType::LIGHT_THEME;
const bool Preferences::defaultAskOnExclusionRemove = true;

const int Preferences::minSyncStateChangeProcessingIntervalMs = 200;

std::shared_ptr<Preferences> Preferences::instance()
{
    static std::shared_ptr<Preferences> preferences (new Preferences());
    return preferences;
}

void Preferences::initialize(QString dataPath)
{
    mDataPath = dataPath;

    QString lockSettingsFile = QDir::toNativeSeparators(dataPath + QString::fromLatin1("/MEGAsync.cfg.lock"));
    QFile::remove(lockSettingsFile);

    QDir dir(dataPath);
    dir.mkpath(QString::fromLatin1("."));
    QString settingsFile = QDir::toNativeSeparators(dataPath + QString::fromLatin1("/MEGAsync.cfg"));
    QString bakSettingsFile = QDir::toNativeSeparators(dataPath + QString::fromLatin1("/MEGAsync.cfg.bak"));
    bool retryFlag = false;

    errorFlag = false;
    mSettings.reset(new EncryptedSettings(settingsFile));

    QString currentAccount = mSettings->value(currentAccountKey).toString();
    if (currentAccount.size())
    {
        if (hasEmail(currentAccount))
        {
            login(currentAccount);
        }
        else
        {
            MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Settings does not contain current account group. Will try to use backup settings")
                         .toUtf8().constData());
            errorFlag = true;
            retryFlag = true;
        }
    }
    else
    {
        retryFlag = true;
    }

    if (QFile::exists(bakSettingsFile) && retryFlag)
    {
        if (QFile::exists(settingsFile))
        {
            QFile::remove(settingsFile);
        }

        if (QFile::rename(bakSettingsFile, settingsFile))
        {
            mSettings.reset(new EncryptedSettings(settingsFile));

            //Retry with backup file
            currentAccount = mSettings->value(currentAccountKey).toString();
            if (currentAccount.size())
            {
                if (hasEmail(currentAccount))
                {
                    login(currentAccount);
                    errorFlag = false;
                }
                else
                {
                    MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Settings does not contain current account group in backup setting either.")
                                 .toUtf8().constData());
                    errorFlag = true;
                }
            }
        }
    }

    if (errorFlag)
    {
        MegaApi::log(MegaApi::LOG_LEVEL_DEBUG, QString::fromUtf8("Cleaning settings after error encountered.").toUtf8().constData());
        clearAll();
    }
    else
    {
        recoverDeprecatedNotificationsSettings();
    }
}

Preferences::Preferences() :
    QObject(),
    mutex(QMutex::Recursive),
    mSettings(nullptr),
    diffTimeWithSDK(0),
    lastTransferNotification(0)
{
    clearTemporalBandwidth();
}

QString Preferences::email()
{
    assert(logged());
    return getValueConcurrent<QString>(emailKey);
}

void Preferences::setEmail(QString email)
{
    mutex.lock();
    login(email);
    mSettings->setValue(emailKey, email);
    mutex.unlock();
    emit stateChanged();
}

QString Preferences::firstName()
{
    assert(logged());
    return getValueConcurrent<QString>(firstNameKey, QString());
}

void Preferences::setFirstName(QString firstName)
{
    setValueConcurrently(firstNameKey, firstName);
}

QString Preferences::lastName()
{
    assert(logged());
    return getValueConcurrent<QString>(lastNameKey, QString());
}

void Preferences::setLastName(QString lastName)
{
    setValueConcurrently(lastNameKey, lastName);
}

QString Preferences::fileHash(const QString& filePath)
{
    assert(logged());
    return getValueConcurrent<QString>(filePath, QString());
}

void Preferences::setFileHash(const QString& filePath, const QString& fileHash)
{
    setValueConcurrently(filePath, fileHash, true);
}

void Preferences::setSession(QString session)
{
    mutex.lock();
    storeSessionInGeneral(session);
    mutex.unlock();
}

void Preferences::setSessionInUserGroup(QString session)
{
    assert(logged());
    setValueConcurrently(sessionKey, session);
}

void Preferences::storeSessionInGeneral(QString session)
{
    mutex.lock();

    QString currentAccount;
    if (logged())
    {
        mSettings->setValue(sessionKey, session); //store in user group too (for backwards compatibility)
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    mSettings->setValue(sessionKey, session);
    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
}

QString Preferences::getSessionInGeneral()
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    QString value = getValue<QString>(sessionKey);
    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
    return value;
}


QString Preferences::getSession()
{
    mutex.lock();
    QString value;
    if (logged())
    {
        value = mSettings->value(sessionKey).toString(); // for MEGAsync prior unfinished fetchnodes resumable sessions (<=4.3.1)
    }

    if (value.isEmpty() && needsFetchNodesInGeneral())
    {
        value = getSessionInGeneral(); // for MEGAsync with unfinished fetchnodes resumable sessions (>4.3.1)
    }

    mutex.unlock();
    return value;
}

void Preferences::removeEphemeralCredentials()
{
    QMutexLocker lock(&mutex);
    mSettings->remove(ephemeralSessionKey);
}

void Preferences::setEphemeralCredentials(const EphemeralCredentials& cred)
{
    QMutexLocker lock(&mutex);
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream << cred;
    mSettings->setValue(ephemeralSessionKey, array.toBase64());
}

EphemeralCredentials Preferences::getEphemeralCredentials()
{
    QMutexLocker lock(&mutex);
    QByteArray array = getValue<QByteArray>(ephemeralSessionKey);
    QByteArray base64 = QByteArray::fromBase64(array);
    QDataStream stream(&base64, QIODevice::ReadOnly);
    EphemeralCredentials cred;
    stream >> cred;
    return cred;
}

unsigned long long Preferences::transferIdentifier()
{
    mutex.lock();
    assert(logged());
    auto value = getValue<unsigned long long>(transferIdentifierKey, defaultTransferIdentifier);
    value++;
    mSettings->setValue(transferIdentifierKey, value);
    mutex.unlock();
    return value;
}

long long Preferences::lastTransferNotificationTimestamp()
{
    return lastTransferNotification;
}

void Preferences::setLastTransferNotificationTimestamp()
{
    lastTransferNotification = QDateTime::currentMSecsSinceEpoch();
}

long long Preferences::totalStorage()
{
    assert(logged());
    return getValueConcurrent<long long>(totalStorageKey);
}

void Preferences::setTotalStorage(long long value)
{
    assert(logged());
    setValueConcurrently(totalStorageKey, value);
}

long long Preferences::usedStorage()
{
    assert(logged());
    return getValueConcurrent<long long>(usedStorageKey);
}

void Preferences::setUsedStorage(long long value)
{
    assert(logged());
    value = std::max(value, static_cast<long long>(0));
    setValueConcurrently(usedStorageKey, value);
}

long long Preferences::availableStorage()
{
    mutex.lock();
    assert(logged());
    long long total = getValue<long long>(totalStorageKey);
    long long used = getValue<long long>(usedStorageKey);
    mutex.unlock();
    long long available = total - used;
    return available >= 0 ? available : 0;
}

long long Preferences::cloudDriveStorage()
{
    assert(logged());
    return getValueConcurrent<long long>(cloudDriveStorageKey);
}

void Preferences::setCloudDriveStorage(long long value)
{
    assert(logged());
    setValueConcurrently(cloudDriveStorageKey, value);
}

long long Preferences::vaultStorage()
{
    assert(logged());
    return getValueConcurrent<long long>(vaultStorageKey);
}

void Preferences::setVaultStorage(long long value)
{
    assert(logged());
    setValueConcurrently(vaultStorageKey, value);
}

long long Preferences::rubbishStorage()
{
    assert(logged());
    return getValueConcurrent<long long>(rubbishStorageKey);
}

void Preferences::setRubbishStorage(long long value)
{
    assert(logged());
    setValueConcurrently(rubbishStorageKey, value);
}

long long Preferences::inShareStorage()
{
    assert(logged());
    return getValueConcurrent<long long>(inShareStorageKey);
}

void Preferences::setInShareStorage(long long value)
{
    assert(logged());
    setValueConcurrently(inShareStorageKey, value);
}

long long Preferences::versionsStorage()
{
    assert(logged());
    return getValueConcurrent<long long>(versionsStorageKey);
}

void Preferences::setVersionsStorage(long long value)
{
    assert(logged());
    setValueConcurrently(versionsStorageKey, value);
}

long long Preferences::cloudDriveFiles()
{
    assert(logged());
    return getValueConcurrent<long long>(cloudDriveFilesKey);
}

void Preferences::setCloudDriveFiles(long long value)
{
    assert(logged());
    setValueConcurrently(cloudDriveFilesKey, value);
}

long long Preferences::vaultFiles()
{
    assert(logged());
    return getValueConcurrent<long long>(vaultFilesKey);
}

void Preferences::setVaultFiles(long long value)
{
    assert(logged());
    setValueConcurrently(vaultFilesKey, value);
}

long long Preferences::rubbishFiles()
{
    assert(logged());
    return getValueConcurrent<long long>(rubbishFilesKey);
}

void Preferences::setRubbishFiles(long long value)
{
    assert(logged());
    setValueConcurrently(rubbishFilesKey, value);
}

long long Preferences::inShareFiles()
{
    assert(logged());
    return getValueConcurrent<long long>(inShareFilesKey);
}

void Preferences::setInShareFiles(long long value)
{
    assert(logged());
    setValueConcurrently(inShareFilesKey, value);
}

long long Preferences::cloudDriveFolders()
{
    assert(logged());
    return getValueConcurrent<long long>(cloudDriveFoldersKey);
}

void Preferences::setCloudDriveFolders(long long value)
{
    assert(logged());
    setValueConcurrently(cloudDriveFoldersKey, value);
}

long long Preferences::vaultFolders()
{
    assert(logged());
    return getValueConcurrent<long long>(vaultFoldersKey);
}

void Preferences::setVaultFolders(long long value)
{
    assert(logged());
    setValueConcurrently(vaultFoldersKey, value);
}

long long Preferences::rubbishFolders()
{
    assert(logged());
    return getValueConcurrent<long long>(rubbishFoldersKey);
}

void Preferences::setRubbishFolders(long long value)
{
    assert(logged());
    setValueConcurrently(rubbishFoldersKey, value);
}

long long Preferences::inShareFolders()
{
    assert(logged());
    return getValueConcurrent<long long>(inShareFoldersKey);
}

void Preferences::setInShareFolders(long long value)
{
    assert(logged());
    setValueConcurrently(inShareFoldersKey, value);
}

long long Preferences::totalBandwidth()
{
    assert(logged());
    return getValueConcurrent<long long>(totalBandwidthKey);
}

void Preferences::setTotalBandwidth(long long value)
{
    assert(logged());
    setValueConcurrently(totalBandwidthKey, value);
}

int Preferences::bandwidthInterval()
{
    assert(logged());
    return getValueConcurrent<int>(usedBandwidthIntervalKey);
}

void Preferences::setBandwidthInterval(int value)
{
    assert(logged());
    setValueConcurrently(usedBandwidthIntervalKey, value);
}

bool Preferences::isTemporalBandwidthValid()
{
    return isTempBandwidthValid;
}

long long Preferences::getMsDiffTimeWithSDK()
{
    return diffTimeWithSDK;
}

void Preferences::setDsDiffTimeWithSDK(long long diffTime)
{
    this->diffTimeWithSDK = diffTime;
}

long long Preferences::getOverStorageDialogExecution()
{
    assert(logged());
    return getValueConcurrent<long long>(overStorageDialogExecutionKey, defaultTimeStamp);
}

void Preferences::setOverStorageDialogExecution(long long timestamp)
{
    assert(logged());
    setValueConcurrently(overStorageDialogExecutionKey, timestamp);
}

long long Preferences::getOverStorageNotificationExecution()
{
    assert(logged());
    return getValueConcurrent<long long>(overStorageNotificationExecutionKey, defaultTimeStamp);
}

void Preferences::setOverStorageNotificationExecution(long long timestamp)
{
    assert(logged());
    setValueConcurrently(overStorageNotificationExecutionKey, timestamp);
}

long long Preferences::getAlmostOverStorageNotificationExecution()
{
    assert(logged());
    return getValueConcurrent<long long>(almostOverStorageNotificationExecutionKey, defaultTimeStamp);
}

void Preferences::setAlmostOverStorageNotificationExecution(long long timestamp)
{
    assert(logged());
    setValueConcurrently(almostOverStorageNotificationExecutionKey, timestamp);
}

long long Preferences::getPayWallNotificationExecution()
{
    assert(logged());
    return getValueConcurrent<long long>(payWallNotificationExecutionKey, defaultTimeStamp);
}

void Preferences::setPayWallNotificationExecution(long long timestamp)
{
    assert(logged());
    setValueConcurrently(payWallNotificationExecutionKey, timestamp);
}

long long Preferences::getAlmostOverStorageDismissExecution()
{
    assert(logged());
    return getValueConcurrent<long long>(almostOverStorageDismissExecutionKey, defaultTimeStamp);
}

void Preferences::setAlmostOverStorageDismissExecution(long long timestamp)
{
    assert(logged());
    setValueConcurrently(almostOverStorageDismissExecutionKey, timestamp);
}

long long Preferences::getOverStorageDismissExecution()
{
    assert(logged());
    return getValueConcurrent<long long>(overStorageDismissExecutionKey, defaultTimeStamp);
}

void Preferences::setOverStorageDismissExecution(long long timestamp)
{
    assert(logged());
    setValueConcurrently(overStorageDismissExecutionKey, timestamp);
}

std::chrono::system_clock::time_point Preferences::getTimePoint(const QString& key)
{
    QMutexLocker locker(&mutex);
    assert(logged());
    const long long value{getValue<long long>(key, defaultTimeStamp)};
    std::chrono::milliseconds durationMillis(value);
    return std::chrono::system_clock::time_point{durationMillis};
}

void Preferences::setTimePoint(const QString& key, const std::chrono::system_clock::time_point& timepoint)
{
    QMutexLocker locker(&mutex);
    assert(logged());
    auto timePointMillis = std::chrono::time_point_cast<std::chrono::milliseconds>(timepoint).time_since_epoch().count();
    mSettings->setValue(key, static_cast<long long>(timePointMillis));
}

template<typename T>
T Preferences::getValue(const QString &key)
{
    return mSettings->value(key).value<T>();
}

template<typename T>
T Preferences::getValue(const QString &key, const T &defaultValue)
{
    return mSettings->value(key, defaultValue).template value<T>();
}

template<typename T>
T Preferences::getValueConcurrent(const QString &key)
{
    QMutexLocker locker(&mutex);
    return getValue<T>(key);
}

template<typename T>
T Preferences::getValueConcurrent(const QString &key, const T &defaultValue)
{
    QMutexLocker locker(&mutex);
    return getValue<T>(key, defaultValue);
}

void Preferences::setAndCachedValue(const QString &key, const QVariant &value)
{
    mSettings->setValue(key, value);
}

void Preferences::setValueConcurrently(const QString &key, const QVariant &value, bool notifyChange)
{
    QMutexLocker locker(&mutex);
    setAndCachedValue(key, value);
    if(notifyChange)
    {
        emit valueChanged(key);
    }
}

std::chrono::system_clock::time_point Preferences::getTransferOverQuotaDialogLastExecution()
{
    return getTimePoint(transferOverQuotaDialogLastExecutionKey);
}

void Preferences::setTransferOverQuotaDialogLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferOverQuotaDialogLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getTransferOverQuotaOsNotificationLastExecution()
{
    return getTimePoint(transferOverQuotaOsNotificationLastExecutionKey);
}

void Preferences::setTransferOverQuotaOsNotificationLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferOverQuotaOsNotificationLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getTransferAlmostOverQuotaOsNotificationLastExecution()
{
    return getTimePoint(transferAlmostOverQuotaOsNotificationLastExecutionKey);
}

void Preferences::setTransferAlmostOverQuotaOsNotificationLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferAlmostOverQuotaOsNotificationLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getTransferAlmostOverQuotaUiAlertLastExecution()
{
    return getTimePoint(transferAlmostOverQuotaUiAlertLastExecutionKey);
}

void Preferences::setTransferAlmostOverQuotaUiAlertLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferAlmostOverQuotaUiAlertLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getTransferOverQuotaUiAlertLastExecution()
{
    return getTimePoint(transferOverQuotaUiAlertLastExecutionKey);
}

void Preferences::setTransferOverQuotaUiAlertLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferOverQuotaUiAlertLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getTransferOverQuotaSyncDialogLastExecution()
{
    return getTimePoint(transferOverQuotaSyncDialogLastExecutionKey);
}

void Preferences::setTransferOverQuotaSyncDialogLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferOverQuotaSyncDialogLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getTransferOverQuotaDownloadsDialogLastExecution()
{
    return getTimePoint(transferOverQuotaDownloadsDialogLastExecutionKey);
}

void Preferences::setTransferOverQuotaDownloadsDialogLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferOverQuotaDownloadsDialogLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getTransferOverQuotaImportLinksDialogLastExecution()
{
    return getTimePoint(transferOverQuotaImportLinksDialogLastExecutionKey);
}

void Preferences::setTransferOverQuotaImportLinksDialogLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferOverQuotaImportLinksDialogLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getTransferOverQuotaStreamDialogLastExecution()
{
    return getTimePoint(transferOverQuotaStreamDialogLastExecutionKey);
}

void Preferences::setTransferOverQuotaStreamDialogLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(transferOverQuotaStreamDialogLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getStorageOverQuotaUploadsDialogLastExecution()
{
    return getTimePoint(storageOverQuotaUploadsDialogLastExecutionKey);
}

void Preferences::setStorageOverQuotaUploadsDialogLastExecution(std::chrono::system_clock::time_point timepoint)
{
    setTimePoint(storageOverQuotaUploadsDialogLastExecutionKey, timepoint);
}

std::chrono::system_clock::time_point Preferences::getStorageOverQuotaSyncsDialogLastExecution()
{
    return getTimePoint(storageOverQuotaSyncsDialogLastExecutionKey);
}

void Preferences::setStorageOverQuotaSyncsDialogLastExecution(std::chrono::system_clock::time_point timepoint)
{

    setTimePoint(storageOverQuotaSyncsDialogLastExecutionKey, timepoint);
}

int Preferences::getStorageState()
{
    assert(logged());
    return getValueConcurrent<int>(storageStateQKey, MegaApi::STORAGE_STATE_UNKNOWN);
}

void Preferences::setStorageState(int value)
{
    assert(logged());
    setValueConcurrently(storageStateQKey, value);
}

int Preferences::getBusinessState()
{
    assert(logged());
    return getValueConcurrent<int>(businessStateQKey, -2);
}

void Preferences::setBusinessState(int value)
{
    assert(logged());
    setValueConcurrently(businessStateQKey, value);
}

int Preferences::getBlockedState()
{
    assert(logged());
    return getValueConcurrent<int>(blockedStateQKey, -2);
}

void Preferences::setBlockedState(int value)
{
    assert(logged());
    setValueConcurrently(blockedStateQKey, value);
}

void Preferences::setTemporalBandwidthValid(bool value)
{
    this->isTempBandwidthValid = value;
}

long long Preferences::temporalBandwidth()
{
    return tempBandwidth > 0 ? tempBandwidth : 0;
}

void Preferences::setTemporalBandwidth(long long value)
{
    this->tempBandwidth = value;
}

int Preferences::temporalBandwidthInterval()
{
    return tempBandwidthInterval > 0 ? tempBandwidthInterval : 6;
}

void Preferences::setTemporalBandwidthInterval(int value)
{
    this->tempBandwidthInterval = value;
}

long long Preferences::usedBandwidth()
{
    assert(logged());
    return getValueConcurrent<long long>(usedBandwidthKey);
}

void Preferences::setUsedBandwidth(long long value)
{
    assert(logged());
    setValueConcurrently(usedBandwidthKey, value);
}

int Preferences::accountType()
{
    assert(logged());
    return getValueConcurrent<int>(accountTypeKey);
}

void Preferences::setAccountType(int value)
{
    assert(logged());
    setValueConcurrently(accountTypeKey, value);
}

long long Preferences::proExpirityTime()
{
    assert(logged());
    return getValueConcurrent<long long>(proExpirityTimeKey);
}

void Preferences::setProExpirityTime(long long value)
{
    assert(logged());
    setValueConcurrently(proExpirityTimeKey, value);
}
/************ NOTIFICATIONS GETTERS/SETTERS ************/

bool Preferences::isNotificationEnabled(NotificationsTypes type, bool includingGeneralSwitch)
{
    bool value(false);

    if(!includingGeneralSwitch || isGeneralSwitchNotificationsOn())
    {
        auto key = notificationsTypeToString(type);

        if(!key.isEmpty())
        {
            value = getValueConcurrent<bool>(key, defaultShowNotifications);
        }
    }

    return value;
}

bool Preferences::isAnyNotificationEnabled(bool includingGeneralSwitch)
{
    bool result(false);

    if(!includingGeneralSwitch || isGeneralSwitchNotificationsOn())
    {
        for(int index = notificationsTypeUT(NotificationsTypes::GENERAL_SWITCH_NOTIFICATIONS) + 1;
            index < notificationsTypeUT(NotificationsTypes::LAST); ++index)
        {
           if(isNotificationEnabled((NotificationsTypes)index,includingGeneralSwitch))
           {
               result = true;
               break;
           }
        }
    }

    return result;
}

bool Preferences::isGeneralSwitchNotificationsOn()
{
    bool generalSwitchNotificationsValue(false);

    auto generalSwitchNotificationsKey = notificationsTypeToString(NotificationsTypes::GENERAL_SWITCH_NOTIFICATIONS);

    if(!generalSwitchNotificationsKey.isEmpty())
    {
        generalSwitchNotificationsValue = getValueConcurrent<bool>(generalSwitchNotificationsKey, defaultShowNotifications);
    }

    return generalSwitchNotificationsValue;
}

void Preferences::enableNotifications(NotificationsTypes type, bool value)
{
    assert(logged());

    auto key = notificationsTypeToString(type);

    if(!key.isEmpty())
    {
        setValueConcurrently(key, value);
    }
}

void Preferences::recoverDeprecatedNotificationsSettings()
{
    QVariant deprecatedGlobalNotifications = getValueConcurrent<QVariant>(showDeprecatedNotificationsKey);
    if(!deprecatedGlobalNotifications.isNull())
    {
        assert(logged());
        for(int index = notificationsTypeUT(NotificationsTypes::GENERAL_SWITCH_NOTIFICATIONS) + 1;
            index < notificationsTypeUT(NotificationsTypes::LAST); ++index)
        {
            auto key = notificationsTypeToString((NotificationsTypes)index);

            if(!key.isEmpty())
            {
               setValueConcurrently(key,deprecatedGlobalNotifications);
            }
        }

        QMutexLocker locker(&mutex);
        mSettings->remove(showDeprecatedNotificationsKey);
    }
}

QString Preferences::notificationsTypeToString(NotificationsTypes type)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<NotificationsTypes>();
    return QString::fromUtf8(metaEnum.valueToKey(notificationsTypeUT(type)));
}

/************ STALLED ISSUES **********************************/

Preferences::StalledIssuesModeType Preferences::stalledIssuesMode()
{
    auto value = getValueConcurrent<int>(stalledIssuesModeKey, static_cast<int>(defaultStalledIssuesMode));
    return static_cast<StalledIssuesModeType>(value);
}

void Preferences::setStalledIssuesMode(StalledIssuesModeType value)
{
    auto currentValue(stalledIssuesMode());
    if(value != currentValue)
    {
        AppStatsEvents::EventType type = AppStatsEvents::EventType::NONE;

        if(value == StalledIssuesModeType::Smart)
        {
            type = (currentValue == StalledIssuesModeType::None)
                    ? AppStatsEvents::EventType::SI_SMART_MODE_FIRST_SELECTED
                    : AppStatsEvents::EventType::SI_CHANGE_TO_SMART_MODE;
        }
        else
        {
            type = (currentValue == StalledIssuesModeType::None)
                    ? AppStatsEvents::EventType::SI_ADVANCED_MODE_FIRST_SELECTED
                    : AppStatsEvents::EventType::SI_CHANGE_TO_ADVANCED_MODE;
        }

        if(type != AppStatsEvents::EventType::NONE)
        {
            MegaSyncApp->getStatsEventHandler()->sendEvent(type);
        }

        setValueConcurrently(stalledIssuesModeKey, static_cast<int>(value), true);
    }
}

bool Preferences::isStalledIssueSmartModeActivated()
{
    return stalledIssuesMode() == Preferences::StalledIssuesModeType::Smart;
}

QDate Preferences::stalledIssuesEventLastDate()
{
    return getValueConcurrent<QDate>(stalledIssuesEventDateKey, QDate());
}

void Preferences::updateStalledIssuesEventLastDate()
{
    setValueConcurrently(stalledIssuesEventDateKey, QDate::currentDate());
}

/************ END OF NOTIFICATIONS GETTERS/SETTERS ************/

bool Preferences::startOnStartup()
{
    return getValueConcurrent<bool>(startOnStartupKey, defaultStartOnStartup);
}

void Preferences::setStartOnStartup(bool value)
{
    setValueConcurrently(startOnStartupKey, value);
}

bool Preferences::usingHttpsOnly()
{
    return getValueConcurrent<bool>(useHttpsOnlyKey, defaultUseHttpsOnly);
}

void Preferences::setUseHttpsOnly(bool value)
{
    setValueConcurrently(useHttpsOnlyKey, value);
}

bool Preferences::SSLcertificateException()
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }
    bool value = getValue<bool>(SSLcertificateExceptionKey, defaultSSLcertificateException);
    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
    return value;
}

void Preferences::setSSLcertificateException(bool value)
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }
    mSettings->setValue(SSLcertificateExceptionKey, value);
    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
}

QString Preferences::language()
{
    return getValueConcurrent<QString>(languageKey, QLocale::system().name());
}

void Preferences::setLanguage(QString &value)
{
    setValueConcurrently(languageKey, value);
}

bool Preferences::updateAutomatically()
{
    return getValueConcurrent<bool>(updateAutomaticallyKey, defaultUpdateAutomatically);
}

void Preferences::setUpdateAutomatically(bool value)
{
    setValueConcurrently(updateAutomaticallyKey, value);
}

bool Preferences::hasDefaultUploadFolder()
{
    return getValueConcurrent<bool>(hasDefaultUploadFolderKey, uploadFolder() != 0);
}

bool Preferences::hasDefaultDownloadFolder()
{
    return getValueConcurrent<bool>(hasDefaultDownloadFolderKey, !downloadFolder().isEmpty());
}

void Preferences::setHasDefaultUploadFolder(bool value)
{
    setValueConcurrently(hasDefaultUploadFolderKey, value);
}

void Preferences::setHasDefaultDownloadFolder(bool value)
{
    setValueConcurrently(hasDefaultDownloadFolderKey, value);
}

bool Preferences::canUpdate(QString filePath)
{
    mutex.lock();

    bool value = true;

#ifdef WIN32
    qt_ntfs_permission_lookup++; // turn checking on
#endif
    if (!QFileInfo(filePath).isWritable())
    {
        value = false;
    }
#ifdef WIN32
    qt_ntfs_permission_lookup--; // turn it off again
#endif

    mutex.unlock();

    return value;
}

int Preferences::accountStateInGeneral()
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    int value = getValue<int>(currentAccountStatusKey, defaultAccountStatus);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
    return value;
}

void Preferences::setAccountStateInGeneral(int value)
{
    mutex.lock();

    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    mSettings->setValue(currentAccountStatusKey, value);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
}


bool Preferences::needsFetchNodesInGeneral()
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    bool value = getValue<bool>(needsFetchNodesKey, defaultNeedsFetchNodes);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
    return value;
}

void Preferences::setNeedsFetchNodesInGeneral(bool value)
{
    mutex.lock();

    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    mSettings->setValue(needsFetchNodesKey, value);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
}


int Preferences::uploadLimitKB()
{
    assert(logged());
    return getValueConcurrent<int>(uploadLimitKBKey, defaultUploadLimitKB);
}

void Preferences::setUploadLimitKB(int value)
{
    assert(logged());
    setValueConcurrently(uploadLimitKBKey, value);
}

int Preferences::downloadLimitKB()
{
    assert(logged());
    return getValueConcurrent<int>(downloadLimitKBKey, defaultDownloadLimitKB);
}

int Preferences::parallelUploadConnections()
{
    return getValueConcurrent<int>(parallelUploadConnectionsKey, defaultParallelUploadConnections);
}

int Preferences::parallelDownloadConnections()
{
    return getValueConcurrent<int>(parallelDownloadConnectionsKey, defaultParallelDownloadConnections);
}

void Preferences::setParallelUploadConnections(int value)
{
    assert(logged());
    if (value < 1 || value > 6)
    {
       value = 3;
    }
    setValueConcurrently(parallelUploadConnectionsKey, value);
}

void Preferences::setParallelDownloadConnections(int value)
{
    assert(logged());
    if (value < 1 || value > 6)
    {
       value = 4;
    }
    setValueConcurrently(parallelDownloadConnectionsKey, value);
}

void Preferences::setDownloadLimitKB(int value)
{
    assert(logged());
    setValueConcurrently(downloadLimitKBKey, value);
}

bool Preferences::upperSizeLimit()
{
    return getValueConcurrent<bool>(upperSizeLimitKey, defaultUpperSizeLimit);
}

void Preferences::setUpperSizeLimit(bool value)
{
    setValueConcurrently(upperSizeLimitKey, value);
}

unsigned long long Preferences::upperSizeLimitValue()
{
    assert(logged());
    return getValueConcurrent<unsigned long long>(upperSizeLimitValueKey, defaultUpperSizeLimitValue);
}

void Preferences::setUpperSizeLimitValue(unsigned long long value)
{
    assert(logged());
    setValueConcurrently(upperSizeLimitValueKey, value);
}

bool Preferences::cleanerDaysLimit()
{
    return getValueConcurrent<bool>(cleanerDaysLimitKey, defaultCleanerDaysLimit);
}

void Preferences::setCleanerDaysLimit(bool value)
{
    setValueConcurrently(cleanerDaysLimitKey, value);
}

int Preferences::cleanerDaysLimitValue()
{
    assert(logged());
    return getValueConcurrent<int>(cleanerDaysLimitValueKey, defaultCleanerDaysLimitValue);
}

void Preferences::setCleanerDaysLimitValue(int value)
{
    assert(logged());
    setValueConcurrently(cleanerDaysLimitValueKey, value);
}

int Preferences::upperSizeLimitUnit()
{
    assert(logged());
    return getValueConcurrent<int>(upperSizeLimitUnitKey, defaultUpperSizeLimitUnit);
}
void Preferences::setUpperSizeLimitUnit(int value)
{
    assert(logged());
    setValueConcurrently(upperSizeLimitUnitKey, value);
}

bool Preferences::lowerSizeLimit()
{
    return getValueConcurrent<bool>(lowerSizeLimitKey, defaultLowerSizeLimit);
}

void Preferences::setLowerSizeLimit(bool value)
{
    setValueConcurrently(lowerSizeLimitKey, value);
}

unsigned long long Preferences::lowerSizeLimitValue()
{
    assert(logged());
    return getValueConcurrent<unsigned long long>(lowerSizeLimitValueKey, defaultLowerSizeLimitValue);
}

void Preferences::setLowerSizeLimitValue(unsigned long long value)
{
    assert(logged());
    setValueConcurrently(lowerSizeLimitValueKey, value);
}

int Preferences::lowerSizeLimitUnit()
{
    assert(logged());
    return getValueConcurrent<int>(lowerSizeLimitUnitKey, defaultLowerSizeLimitUnit);
}

void Preferences::setLowerSizeLimitUnit(int value)
{
    assert(logged());
    setValueConcurrently(lowerSizeLimitUnitKey, value);
}

int Preferences::folderPermissionsValue()
{
    mutex.lock();
    int permissions = getValue<int>(folderPermissionsKey, defaultFolderPermissions);
    mutex.unlock();
    return permissions;
}

void Preferences::setFolderPermissionsValue(int permissions)
{
    setValueConcurrently(folderPermissionsKey, permissions);
}

int Preferences::filePermissionsValue()
{
    mutex.lock();
    int permissions = getValue<int>(filePermissionsKey, defaultFilePermissions);
    mutex.unlock();
    return permissions;
}

void Preferences::setFilePermissionsValue(int permissions)
{
    setValueConcurrently(filePermissionsKey, permissions);
}

int Preferences::proxyType()
{
    return getValueConcurrent<int>(proxyTypeKey, defaultProxyType);
}

void Preferences::setProxyType(int value)
{
    setValueConcurrently(proxyTypeKey, value);
}

int Preferences::proxyProtocol()
{
    return getValueConcurrent<int>(proxyProtocolKey, defaultProxyProtocol);
}

void Preferences::setProxyProtocol(int value)
{
    setValueConcurrently(proxyProtocolKey, value);
}

QString Preferences::proxyServer()
{
    return getValueConcurrent<QString>(proxyServerKey, defaultProxyServer);
}

void Preferences::setProxyServer(const QString &value)
{
    setValueConcurrently(proxyServerKey, value);
}

unsigned short Preferences::proxyPort()
{
    return getValueConcurrent<unsigned short>(proxyPortKey, defaultProxyPort);
}

void Preferences::setProxyPort(unsigned short value)
{
    setValueConcurrently(proxyPortKey, value);
}

bool Preferences::proxyRequiresAuth()
{
    return getValueConcurrent<bool>(proxyRequiresAuthKey, defaultProxyRequiresAuth);
}

void Preferences::setProxyRequiresAuth(bool value)
{
    setValueConcurrently(proxyRequiresAuthKey, value);
}

QString Preferences::getProxyUsername()
{
    return getValueConcurrent<QString>(proxyUsernameKey, defaultProxyUsername);
}

void Preferences::setProxyUsername(const QString &value)
{
    setValueConcurrently(proxyUsernameKey, value);
}

QString Preferences::getProxyPassword()
{
    return getValueConcurrent<QString>(proxyPasswordKey, defaultProxyPassword);
}

void Preferences::setProxyPassword(const QString &value)
{
    setValueConcurrently(proxyPasswordKey, value);
}

QString Preferences::proxyHostAndPort()
{
    mutex.lock();
    QString proxy;
    QString hostname = proxyServer();

    QHostAddress ipAddress(hostname);
    if (ipAddress.protocol() == QAbstractSocket::IPv6Protocol)
    {
        proxy = QString::fromUtf8("[") + hostname + QString::fromLatin1("]:") + QString::number(proxyPort());
    }
    else
    {
        proxy = hostname + QString::fromLatin1(":") + QString::number(proxyPort());
    }

    mutex.unlock();
    return proxy;
}

long long Preferences::lastExecutionTime()
{
    return getValueConcurrent<long long>(lastExecutionTimeKey, 0);
}

long long Preferences::installationTime()
{
    return getValueConcurrent<long long>(installationTimeKey, 0);
}

void Preferences::setInstallationTime(long long time)
{
    setValueConcurrently(installationTimeKey, time);
}

long long Preferences::accountCreationTime()
{
    return getValueConcurrent<long long>(accountCreationTimeKey, 0);
}

void Preferences::setAccountCreationTime(long long time)
{
    setValueConcurrently(accountCreationTimeKey, time);

}

long long Preferences::hasLoggedIn()
{
    return getValueConcurrent<long long>(hasLoggedInKey, 0);
}

void Preferences::setHasLoggedIn(long long time)
{
    setValueConcurrently(hasLoggedInKey, time);
}

bool Preferences::isFirstStartDone()
{
    return getValueConcurrent<bool>(firstStartDoneKey, false);
}

void Preferences::setFirstStartDone(bool value)
{
    setValueConcurrently(firstStartDoneKey, value);
}

bool Preferences::isFirstSyncDone()
{
    return getValueConcurrent<bool>(firstSyncDoneKey, false);
}

void Preferences::setFirstSyncDone(bool value)
{
    setValueConcurrently(firstSyncDoneKey, value);
}

bool Preferences::isFirstBackupDone()
{
    return getValueConcurrent<bool>(firstBackupDoneKey, false);
}

void Preferences::setFirstBackupDone(bool value)
{
    setValueConcurrently(firstBackupDoneKey, value);
}

bool Preferences::isFirstFileSynced()
{
    return getValueConcurrent<bool>(firstFileSyncedKey, false);
}

void Preferences::setFirstFileSynced(bool value)
{
    setValueConcurrently(firstFileSyncedKey, value);
}

bool Preferences::isFirstFileBackedUp()
{
    return getValueConcurrent<bool>(firstFileBackedUpKey, false);
}

void Preferences::setFirstFileBackedUp(bool value)
{
    setValueConcurrently(firstFileBackedUpKey, value);
}

bool Preferences::isFirstWebDownloadDone()
{
    return getValueConcurrent<bool>(firstWebDownloadKey, false);
}

void Preferences::setFirstWebDownloadDone(bool value)
{
    setValueConcurrently(firstWebDownloadKey, value);
}

bool Preferences::isFatWarningShown()
{
    return getValueConcurrent<bool>(fatWarningShownKey, false);
}

void Preferences::setFatWarningShown(bool value)
{
    setValueConcurrently(fatWarningShownKey, value);
}

QString Preferences::lastCustomStreamingApp()
{
    return getValueConcurrent<QString>(lastCustomStreamingAppKey);
}

void Preferences::setLastCustomStreamingApp(const QString &value)
{
    setValueConcurrently(lastCustomStreamingAppKey, value);
}

unsigned long long Preferences::getMaxMemoryUsage()
{
    return getValueConcurrent<unsigned long long>(maxMemoryUsageKey, 0);
}

void Preferences::setMaxMemoryUsage(unsigned long long value)
{
    setValueConcurrently(maxMemoryUsageKey, value);
}

long long Preferences::getMaxMemoryReportTime()
{
    return getValueConcurrent<long long>(maxMemoryReportTimeKey, 0);
}

void Preferences::setMaxMemoryReportTime(long long timestamp)
{
    setValueConcurrently(maxMemoryReportTimeKey, timestamp);
}

long long Preferences::lastDailyStatTime()
{
    return getValueConcurrent<long long>(lastDailyStatTimeKey, 0);
}

void Preferences::setLastDailyStatTime(long long time)
{
    setValueConcurrently(lastDailyStatTimeKey, time);
}

void Preferences::setLastExecutionTime(long long time)
{
    setValueConcurrently(lastExecutionTimeKey, time);
}

long long Preferences::lastUpdateTime()
{
    return getValueConcurrent<long long>(lastUpdateTimeKey, 0);
}

void Preferences::setLastUpdateTime(long long time)
{
    assert(logged());
    setValueConcurrently(lastUpdateTimeKey, time);
}

int Preferences::lastUpdateVersion()
{
    assert(logged());
    return getValueConcurrent<int>(lastUpdateVersionKey, 0);
}

void Preferences::setLastUpdateVersion(int version)
{
    assert(logged());
    setValueConcurrently(lastUpdateVersionKey, version);
}

QString Preferences::downloadFolder()
{
    mutex.lock();
    QString value = QDir::toNativeSeparators(getValue<QString>(downloadFolderKey));
    mutex.unlock();
    return value;
}

void Preferences::setDownloadFolder(QString value)
{
    setValueConcurrently(downloadFolderKey, QDir::toNativeSeparators(value));
}

MegaHandle Preferences::uploadFolder()
{
    assert(logged());
    return getValueConcurrent<mega::MegaHandle>(uploadFolderKey);
}

void Preferences::setUploadFolder(mega::MegaHandle value)
{
    assert(logged());
    setValueConcurrently(uploadFolderKey, QVariant::fromValue<mega::MegaHandle>(value));
}

mega::MegaHandle Preferences::importFolder()
{
    assert(logged());
    return getValueConcurrent<mega::MegaHandle>(importFolderKey);
}

void Preferences::setImportFolder(MegaHandle value)
{
    assert(logged());
    setValueConcurrently(importFolderKey, QVariant::fromValue<mega::MegaHandle>(value));
}

bool Preferences::getImportMegaLinksEnabled()
{
    assert(logged());
    return getValueConcurrent<bool>(importMegaLinksEnabledKey, defaultImportMegaLinksEnabled);
}

void Preferences::setImportMegaLinksEnabled(const bool value)
{
    assert(logged());
    setValueConcurrently(importMegaLinksEnabledKey, value);
}

bool Preferences::getDownloadMegaLinksEnabled()
{
    assert(logged());
    return getValueConcurrent<bool>(downloadMegaLinksEnabledKey, defaultDownloadMegaLinksEnabled);
}

void Preferences::setDownloadMegaLinksEnabled(const bool value)
{
    assert(logged());
    setValueConcurrently(downloadMegaLinksEnabledKey, value);
}

bool Preferences::neverCreateLink()
{
    return getValueConcurrent<bool>(neverCreateLinkKey, defaultNeverCreateLink);
}

void Preferences::setNeverCreateLink(bool value)
{
    setValueConcurrently(neverCreateLinkKey, value);
}


/////////   Sync related stuff /////////////////////

void Preferences::removeAllFolders()
{
    QMutexLocker qm(&mutex);
    assert(logged());

    //remove all configured syncs
    mSettings->beginGroup(syncsGroupByTagKey);
    mSettings->remove(QLatin1String("")); //remove group and all its settings
    mSettings->endGroup();
}
QStringList Preferences::getExcludedSyncNames()
{
    assert(logged());
    QStringList value = excludedSyncNames;
    return value;
}

QStringList Preferences::getExcludedSyncPaths()
{
    assert(logged());
    QStringList value = excludedSyncPaths;
    return value;
}

bool Preferences::isOneTimeActionDone(int action)
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    bool value = getValue<bool>(oneTimeActionDoneKey + QString::number(action), false);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
    return value;
}

void Preferences::setOneTimeActionDone(int action, bool done)
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    mSettings->setValue(oneTimeActionDoneKey + QString::number(action), done);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
}

void Preferences::setSystemTrayPromptSuppressed(bool value)
{
    setValueConcurrently(systemTrayPromptSuppressed, value);
}

bool Preferences::isSystemTrayPromptSuppressed()
{
    return getValueConcurrent<bool>(systemTrayPromptSuppressed, defaultSystemTrayPromptSuppressed);
}

void Preferences::setAskOnExclusionRemove(bool value)
{
    setValueConcurrently(askOnExclusionRemove, value);
}

bool Preferences::isAskOnExclusionRemove()
{
    return getValueConcurrent<bool>(askOnExclusionRemove, defaultAskOnExclusionRemove);
}

void Preferences::setSystemTrayLastPromptTimestamp(long long timestamp)
{
    setValueConcurrently(systemTrayLastPromptTimestamp, timestamp);
}

long long Preferences::getSystemTrayLastPromptTimestamp()
{
    return getValueConcurrent<long long>(systemTrayLastPromptTimestamp, defaultTimeStamp);
}


bool Preferences::isOneTimeActionUserDone(int action)
{
    QMutexLocker qm(&mutex);
    assert(logged());

    return getValue<bool>(oneTimeActionUserDoneKey + QString::number(action), false);
}

void Preferences::setOneTimeActionUserDone(int action, bool done)
{
    QMutexLocker qm(&mutex);
    assert(logged());

    mSettings->setValue(oneTimeActionUserDoneKey + QString::number(action), done);
}


QStringList Preferences::getPreviousCrashes()
{
    mutex.lock();
    QStringList previousCrashes;
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }
    previousCrashes = mSettings->value(previousCrashesKey).toString().split(QString::fromLatin1("\n", Qt::SkipEmptyParts));
    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }

    mutex.unlock();
    return previousCrashes;
}

void Preferences::setPreviousCrashes(QStringList crashes)
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    if (!crashes.size())
    {
        mSettings->remove(previousCrashesKey);
    }
    else
    {
        mSettings->setValue(previousCrashesKey, crashes.join(QString::fromLatin1("\n")));
    }

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
}

long long Preferences::getLastReboot()
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    long long value = getValue<long long>(lastRebootKey);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }

    mutex.unlock();
    return value;
}

void Preferences::setLastReboot(long long value)
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    mSettings->setValue(lastRebootKey, value);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
}

long long Preferences::getLastExit()
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    long long value = getValue<long long>(lastExitKey);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }

    mutex.unlock();
    return value;
}

void Preferences::setLastExit(long long value)
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    mSettings->setValue(lastExitKey, value);

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();
}

QSet<MegaHandle> Preferences::getDisabledSyncTags()
{
    QMutexLocker qm(&mutex);
    assert(logged());

    QStringList stringTagList = getValueConcurrent<QString>(disabledSyncsKey).split(QString::fromUtf8("0x1E"), Qt::SkipEmptyParts);
    if (!stringTagList.isEmpty())
    {
        QList<mega::MegaHandle> tagList;
        for (auto &tag : stringTagList)
        {
            tagList.append(tag.toULongLong());
        }

        return QSet<mega::MegaHandle>(tagList.begin(), tagList.end());
    }

    return QSet<mega::MegaHandle>();
}

void Preferences::setDisabledSyncTags(QSet<mega::MegaHandle> disabledSyncs)
{
    QMutexLocker qm(&mutex);
    assert(logged());

    QList<mega::MegaHandle> disabledTags = disabledSyncs.values();
    QStringList tags;

    for(auto &tag : disabledTags)
    {
        tags.append(QString::number(tag));
    }

    setValueConcurrently(disabledSyncsKey, tags.join(QString::fromUtf8("0x1E")));
}

bool Preferences::getNotifyDisabledSyncsOnLogin()
{
    return getValueConcurrent<bool>(notifyDisabledSyncsKey, false);
}

void Preferences::setNotifyDisabledSyncsOnLogin(bool notify)
{
    setValueConcurrently(notifyDisabledSyncsKey, notify);
}

void Preferences::getLastHandleInfo(MegaHandle &lastHandle, int &type, long long &timestamp)
{
    mutex.lock();
    assert(logged());
    timestamp = getValue<long long>(lastPublicHandleTimestampKey, 0);
    lastHandle = getValue<unsigned long long>(lastPublicHandleKey, mega::INVALID_HANDLE);
    type = getValue<int>(lastPublicHandleTypeKey, MegaApi::AFFILIATE_TYPE_INVALID);
    mutex.unlock();
}

void Preferences::setLastPublicHandle(MegaHandle handle, int type)
{
    mutex.lock();
    assert(logged());
    mSettings->setValue(lastPublicHandleKey, (unsigned long long) handle);
    mSettings->setValue(lastPublicHandleTimestampKey, QDateTime::currentMSecsSinceEpoch());
    mSettings->setValue(lastPublicHandleTypeKey, type);
    mutex.unlock();
}

int Preferences::getNumUsers()
{
    mutex.lock();
    assert(!logged());
    int value = mSettings->numChildGroups();
    mutex.unlock();
    return value;
}


bool Preferences::enterUser(QString account)
{
    QMutexLocker locker(&mutex);
    assert(!logged());
    if (account.size() && mSettings->containsGroup(account))
    {
        mSettings->beginGroup(account);
        readFolders();
        return true;
    }
    return false;
}

void Preferences::enterUser(int i)
{
    mutex.lock();
    assert(!logged());
    assert(i < mSettings->numChildGroups());
    if (i < mSettings->numChildGroups())
    {
        mSettings->beginGroup(i);
    }

    readFolders();
    mutex.unlock();
}

void Preferences::leaveUser()
{
    mutex.lock();
    assert(logged());
    mSettings->endGroup();

    mutex.unlock();
}

void Preferences::unlink()
{
    mutex.lock();
    assert(logged());
    mSettings->remove(sessionKey); // Remove session from specific account settings
    mSettings->endGroup();
    mutex.unlock();

    resetGlobalSettings();
}

void Preferences::resetGlobalSettings()
{
    mutex.lock();
    QString currentAccount;
    if (logged())
    {
        mSettings->endGroup();
        currentAccount = mSettings->value(currentAccountKey).toString();
    }

    mSettings->remove(currentAccountKey);
    mSettings->remove(needsFetchNodesKey);
    mSettings->remove(currentAccountStatusKey);
    mSettings->remove(sessionKey); // Remove session from global settings
    clearTemporalBandwidth();

    if (!currentAccount.isEmpty())
    {
        mSettings->beginGroup(currentAccount);
    }
    mutex.unlock();

    emit stateChanged();
}

bool Preferences::isCrashed()
{
    mutex.lock();
    bool value = getValue<bool>(isCrashedKey, false);
    mutex.unlock();
    return value;
}

void Preferences::setCrashed(bool value)
{
    setValueConcurrently(isCrashedKey, value);
    sync();
}

bool Preferences::getGlobalPaused()
{
    return getValueConcurrent<bool>(wasPausedKey, false);
}

void Preferences::setGlobalPaused(bool value)
{
    setValueConcurrently(wasPausedKey, value, true);
}

bool Preferences::getUploadsPaused()
{
    return getValueConcurrent<bool>(wasUploadsPausedKey, false);
}

void Preferences::setUploadsPaused(bool value)
{
    setValueConcurrently(wasUploadsPausedKey, value);
}

bool Preferences::getDownloadsPaused()
{
    return getValueConcurrent<bool>(wasDownloadsPausedKey, false);
}

void Preferences::setDownloadsPaused(bool value)
{
    setValueConcurrently(wasDownloadsPausedKey, value);
}

long long Preferences::lastStatsRequest()
{
    return getValueConcurrent<long long>(lastStatsRequestKey, 0);
}

void Preferences::setLastStatsRequest(long long value)
{
    setValueConcurrently(lastStatsRequestKey, value);
}

bool Preferences::awakeIfActiveEnabled()
{
    mutex.lock();
    assert(logged());
    bool result = getValue(awakeIfActiveKey, defaultAwakeIfActive);
    mutex.unlock();
    return result;
}

void Preferences::setAwakeIfActive(bool value)
{
    setValueConcurrently(awakeIfActiveKey, value);
}

bool Preferences::fileVersioningDisabled()
{
    mutex.lock();
    assert(logged());
    bool result = getValue(disableFileVersioningKey, false);
    mutex.unlock();
    return result;
}

void Preferences::disableFileVersioning(bool value)
{
    setValueConcurrently(disableFileVersioningKey, value);
}

bool Preferences::overlayIconsDisabled()
{
    mutex.lock();
    bool result = getValue(disableOverlayIconsKey, false);
    mutex.unlock();
    return result;
}

void Preferences::disableOverlayIcons(bool value)
{
    setValueConcurrently(disableOverlayIconsKey, value);
}

bool Preferences::leftPaneIconsDisabled()
{
    mutex.lock();
    bool result = getValue(disableLeftPaneIconsKey, false);
    mutex.unlock();
    return result;
}

void Preferences::disableLeftPaneIcons(bool value)
{
    setValueConcurrently(disableLeftPaneIconsKey, value);
}

bool Preferences::error()
{
    return errorFlag;
}

QString Preferences::getDataPath()
{
    mutex.lock();
    QString ret = mDataPath;
    mutex.unlock();
    return ret;
}

QString Preferences::getTempTransfersPath()
{
    return getDataPath() + QDir::separator() + QLatin1String("TempTransfers") + QDir::separator();
}

void Preferences::clearTempTransfersPath()
{
    QDir dir(getTempTransfersPath());
    dir.setFilter(QDir::Hidden | QDir::Files);
    foreach(QString dirItem, dir.entryList())
    {
        dir.remove(dirItem);
    }
}

void Preferences::clearTemporalBandwidth()
{
    isTempBandwidthValid = false;
    tempBandwidth = 0;
    tempBandwidthInterval = 6;
}

void Preferences::clearAll()
{
    mutex.lock();
    if (logged())
    {
        unlink();
    }

    mSettings->clear();
    mutex.unlock();
}

void Preferences::sync()
{
    QMutexLocker locker(&mutex);
    mSettings->sync();
}

void Preferences::setThemeType(ThemeType theme)
{
    auto currentValue(getThemeType());

    if (theme != currentValue)
    {
        setValueConcurrently(themeKey, static_cast<int>(theme));
    }
}

Preferences::ThemeType Preferences::getThemeType()
{
    auto value = getValueConcurrent<int>(themeKey, static_cast<int>(defaultTheme));
    return static_cast<ThemeType>(value);
}

void Preferences::setEmailAndGeneralSettings(const QString &email)
{
    int proxyType = this->proxyType();
    QString proxyServer = this->proxyServer();
    auto proxyPort = this->proxyPort();
    int proxyProtocol = this->proxyProtocol();
    bool proxyAuth = this->proxyRequiresAuth();
    QString proxyUsername = this->getProxyUsername();
    QString proxyPassword = this->getProxyPassword();

    QString session = this->getSessionInGeneral();

    this->setEmail(email);

    this->setSessionInUserGroup(session); //this is required to provide backwards compatibility
    this->setProxyType(proxyType);
    this->setProxyServer(proxyServer);
    this->setProxyPort(proxyPort);
    this->setProxyProtocol(proxyProtocol);
    this->setProxyRequiresAuth(proxyAuth);
    this->setProxyUsername(proxyUsername);
    this->setProxyPassword(proxyPassword);
}

void Preferences::monitorUserAttributes()
{
    assert(logged());
    // Setup FIRST_NAME and LAST_NAME monitoring
    updateFullName();
}

void Preferences::login(QString account)
{
    mutex.lock();
    logout();
    mSettings->setValue(currentAccountKey, account);
    mSettings->beginGroup(account);
    readFolders();
    loadExcludedSyncNames();
    int lastVersion = mSettings->value(lastVersionKey).toInt();
    if (lastVersion != Preferences::VERSION_CODE)
    {
        if ((lastVersion != 0) && (lastVersion < Preferences::VERSION_CODE))
        {
            emit updated(lastVersion);
        }
        mSettings->setValue(lastVersionKey, Preferences::VERSION_CODE);
    }
    mutex.unlock();
}

bool Preferences::logged()
{
    mutex.lock();
    bool value = !mSettings->isGroupEmpty();
    mutex.unlock();
    return value;
}

bool Preferences::hasEmail(QString email)
{
    mutex.lock();
    assert(!logged());
    bool value = mSettings->containsGroup(email);
    if (value)
    {
        mSettings->beginGroup(email);
        QString storedEmail = mSettings->value(emailKey).toString();
        value = !storedEmail.compare(email);
        if (!value)
        {
            MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Email key differs from requested email: %1. Removing the old entry: %2")
                         .arg(email).arg(storedEmail).toUtf8().constData());
            mSettings->remove(QString::fromLatin1(""));
        }
        mSettings->endGroup();
    }
    mutex.unlock();
    return value;
}

void Preferences::logout()
{
    mutex.lock();
    if (logged())
    {
        mSettings->endGroup();
    }
    clearTemporalBandwidth();
    mutex.unlock();
}

static bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    return s1.toLower() < s2.toLower();
}
void Preferences::loadExcludedSyncNames()
{
    excludedSyncNames = getValue<QString>(excludedSyncNamesKey).split(QString::fromLatin1("\n", Qt::SkipEmptyParts));
    if (excludedSyncNames.size()==1 && excludedSyncNames.at(0).isEmpty())
    {
        excludedSyncNames.clear();
    }

    excludedSyncPaths = getValue<QString>(excludedSyncPathsKey).split(QString::fromLatin1("\n", Qt::SkipEmptyParts));
    if (excludedSyncPaths.size()==1 && excludedSyncPaths.at(0).isEmpty())
    {
        excludedSyncPaths.clear();
    }
    excludedSyncNames.removeDuplicates();
    std::sort(excludedSyncNames.begin(), excludedSyncNames.end(), caseInsensitiveLessThan);

    excludedSyncPaths.removeDuplicates();
    std::sort(excludedSyncPaths.begin(), excludedSyncPaths.end(), caseInsensitiveLessThan);
}

QMap<mega::MegaHandle, std::shared_ptr<SyncSettings> > Preferences::getLoadedSyncsMap() const
{
    return loadedSyncsMap;
}

void Preferences::readFolders()
{
    mutex.lock();
    assert(logged());

    loadedSyncsMap.clear();

    mSettings->beginGroup(syncsGroupByTagKey);
    int numSyncs = mSettings->numChildGroups();
    for (int i = 0; i < numSyncs; i++)
    {
        mSettings->beginGroup(i);

        auto sc = std::make_shared<SyncSettings>(mSettings->value(configuredSyncsKey).value<QString>());
        if (sc->backupId())
        {
            loadedSyncsMap[sc->backupId()] = sc;
        }
        else
        {
            MegaApi::log(MegaApi::LOG_LEVEL_WARNING, QString::fromLatin1("Reading invalid Sync Setting!").toUtf8().constData());
        }

        mSettings->endGroup();
    }
    mSettings->endGroup();
    mutex.unlock();
}

SyncData::SyncData(QString name,
                   QString localFolder,
                   mega::MegaHandle megaHandle,
                   QString megaFolder,
                   long long localfp,
                   bool enabled,
                   bool tempDisabled,
                   int pos,
                   QString syncID):
    mName(name),
    mLocalFolder(localFolder),
    mMegaHandle(megaHandle),
    mMegaFolder(megaFolder),
    mLocalfp(localfp),
    mEnabled(enabled),
    mTemporarilyDisabled(tempDisabled),
    mPos(pos),
    mSyncID(syncID)
{}

void Preferences::removeOldCachedSync(int position, QString email)
{
    QMutexLocker qm(&mutex);
    assert(logged() || !email.isEmpty());

    // if not logged, use email to get into that user group and remove just some specific sync group
    if (!logged() && email.size() && mSettings->containsGroup(email))
    {
        mSettings->beginGroup(email);
        mSettings->beginGroup(syncsGroupKey);
        mSettings->beginGroup(QString::number(position));
        mSettings->remove(QString::fromLatin1("")); //Remove all previous values
        mSettings->endGroup();//sync
        mSettings->endGroup();//old syncs
        mSettings->endGroup();//user
        return;
    }

    // otherwise remove oldSync and rewrite all
    auto it = oldSyncs.begin();
    while (it != oldSyncs.end())
    {
        if (it->mPos == position)
        {
            it = oldSyncs.erase(it);
        }
        else
        {
            ++it;
        }
    }
    saveOldCachedSyncs();
}

QList<SyncData> Preferences::readOldCachedSyncs(int *cachedBusinessState, int *cachedBlockedState, int *cachedStorageState, QString email)
{
    QMutexLocker qm(&mutex);
    oldSyncs.clear();

    // if not logged in & email provided, read old syncs from that user and load new-cache sync from prev session
    bool temporarilyLoggedPrefs = false;
    if (!instance()->logged() && !email.isEmpty())
    {
        loadedSyncsMap.clear(); //ensure loaded are empty even when there is no email
        temporarilyLoggedPrefs = instance()->enterUser(email);
        if (temporarilyLoggedPrefs)
        {
            MegaApi::log(MegaApi::LOG_LEVEL_DEBUG, QString::fromUtf8("Migrating syncs data to SDK cache from previous session")
                         .toUtf8().constData());
        }
        else
        {
            return oldSyncs;
        }
    }

    assert(logged());
    //restore cached status
    if (cachedBusinessState) *cachedBusinessState = getValue<int>(businessStateQKey, -2);
    if (cachedBlockedState) *cachedBlockedState = getValue<int>(blockedStateQKey, -2);
    if (cachedStorageState) *cachedStorageState = getValue<int>(storageStateQKey, MegaApi::STORAGE_STATE_UNKNOWN);

    mSettings->beginGroup(syncsGroupKey);
    int numSyncs = mSettings->numChildGroups();
    for (int i = 0; i < numSyncs; i++)
    {
        mSettings->beginGroup(i);

        bool enabled = mSettings->value(folderActiveKey, true).toBool();

        MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromLatin1("Reading old cache sync setting ... ").toUtf8().constData());

        if (temporarilyLoggedPrefs) //coming from old session
        {
            MegaApi::log(MegaApi::LOG_LEVEL_WARNING, QString::fromLatin1(" ... sync configuration rescued from old session. Set as disabled.")
                         .toUtf8().constData());

            enabled = false; // syncs coming from old sessions are now considered unsafe to continue automatically
            // Note: in this particular case, we are not showing any error in the sync (since that information is not carried out
            // to the SDK)
        }

        oldSyncs.push_back(SyncData(
            mSettings->value(syncNameKey).toString(),
            mSettings->value(localFolderKey).toString(),
            mSettings->value(megaFolderHandleKey, QVariant::fromValue<mega::MegaHandle>(INVALID_HANDLE)).value<mega::MegaHandle>(),
            mSettings->value(megaFolderKey).toString(),
            mSettings->value(localFingerprintKey, 0).toLongLong(),
            enabled,
            mSettings->value(temporaryInactiveKey, false).toBool(),
            i,
            mSettings->value(syncIdKey, true).toString()));

        mSettings->endGroup();
    }
    mSettings->endGroup();

    if (temporarilyLoggedPrefs)
    {
        instance()->leaveUser();
    }

    return oldSyncs;
}

void Preferences::saveOldCachedSyncs()
{
    QMutexLocker qm(&mutex);
    assert(logged());

    if (!logged())
    {
        return;
    }

    mSettings->beginGroup(syncsGroupKey);

    mSettings->remove(QString::fromLatin1("")); //Remove all previous values

    int i = 0 ;
    foreach(SyncData osd, oldSyncs) //normally if no errors happened it'll be empty
    {
        mSettings->beginGroup(QString::number(i));

        mSettings->setValue(syncNameKey, osd.mName);
        mSettings->setValue(localFolderKey, osd.mLocalFolder);
        mSettings->setValue(localFingerprintKey, osd.mLocalfp);
        mSettings->setValue(megaFolderHandleKey, QVariant::fromValue<mega::MegaHandle>(osd.mMegaHandle));
        mSettings->setValue(megaFolderKey, osd.mMegaFolder);
        mSettings->setValue(folderActiveKey, osd.mEnabled);
        mSettings->setValue(syncIdKey, osd.mSyncID);

        mSettings->endGroup();
    }

    mSettings->endGroup();
}


void Preferences::removeAllSyncSettings()
{
    QMutexLocker qm(&mutex);
    assert(logged());

    mSettings->beginGroup(syncsGroupByTagKey);

    mSettings->remove(QString::fromLatin1("")); //removes group and all its settings

    mSettings->endGroup();
}


void Preferences::removeSyncSetting(std::shared_ptr<SyncSettings> syncSettings)
{
    QMutexLocker qm(&mutex);
    assert(logged() && syncSettings);
    if (!syncSettings)
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromLatin1("Removing invalid Sync Setting!").toUtf8().constData());
        return;
    }

    mSettings->beginGroup(syncsGroupByTagKey);

    mSettings->beginGroup(QString::number(syncSettings->backupId()));

    mSettings->remove(QString::fromLatin1("")); //removes group and all its settings

    mSettings->endGroup();

    mSettings->endGroup();
}

void Preferences::writeSyncSetting(std::shared_ptr<SyncSettings> syncSettings)
{
    if (logged())
    {
        QMutexLocker qm(&mutex);

        mSettings->beginGroup(syncsGroupByTagKey);

        mSettings->beginGroup(QString::number(syncSettings->backupId()));

        mSettings->setValue(configuredSyncsKey, syncSettings->toString());

        mSettings->endGroup();

        mSettings->endGroup();
    }
    else
    {
        MegaApi::log(MegaApi::LOG_LEVEL_WARNING, QString::fromLatin1("Writting sync settings before logged in").toUtf8().constData());
    }
}

void Preferences::setBaseUrl(const QString &value)
{
    BASE_URL = value;
}

template<typename T>
void Preferences::overridePreference(const QSettings &settings, QString &&name, T &value)
{
    T previous = value;
    QVariant variant = settings.value(name, previous);
    value = variant.value<T>();
    if (previous != value)
    {
        qDebug() << "Preference " << name << " overridden: " << value;
    }
}

template<>
void Preferences::overridePreference(const QSettings &settings, QString &&name, std::chrono::milliseconds &value)
{
    const std::chrono::milliseconds previous{value};
    const long long previousMillis{static_cast<long long>(value.count())};
    const QVariant variant{settings.value(name, previousMillis)};
    value = std::chrono::milliseconds(variant.value<long long>());
    if (previous != value)
    {
        qDebug() << "Preference " << name << " overridden: " << value.count();
    }
}

void Preferences::overridePreferences(const QSettings &settings)
{
    overridePreference(settings, QString::fromUtf8("OQ_DIALOG_INTERVAL_MS"), Preferences::OQ_DIALOG_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("OQ_NOTIFICATION_INTERVAL_MS"), Preferences::OQ_NOTIFICATION_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("ALMOST_OS_INTERVAL_MS"), Preferences::ALMOST_OQ_UI_MESSAGE_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("OS_INTERVAL_MS"), Preferences::OQ_UI_MESSAGE_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("PAYWALL_NOTIFICATION_INTERVAL_MS"), Preferences::PAYWALL_NOTIFICATION_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("USER_INACTIVITY_MS"), Preferences::USER_INACTIVITY_MS);
    overridePreference(settings, QString::fromUtf8("STATE_REFRESH_INTERVAL_MS"), Preferences::STATE_REFRESH_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("NETWORK_REFRESH_INTERVAL_MS"), Preferences::NETWORK_REFRESH_INTERVAL_MS);

    overridePreference(settings, QString::fromUtf8("TRANSFER_OVER_QUOTA_DIALOG_DISABLE_DURATION_MS"), Preferences::OVER_QUOTA_DIALOG_DISABLE_DURATION);
    overridePreference(settings, QString::fromUtf8("TRANSFER_OVER_QUOTA_OS_NOTIFICATION_DISABLE_DURATION_MS"), Preferences::OVER_QUOTA_OS_NOTIFICATION_DISABLE_DURATION);
    overridePreference(settings, QString::fromUtf8("TRANSFER_OVER_QUOTA_UI_ALERT_DISABLE_DURATION_MS"), Preferences::OVER_QUOTA_UI_ALERT_DISABLE_DURATION);
    overridePreference(settings, QString::fromUtf8("TRANSFER_ALMOST_OVER_QUOTA_UI_ALERT_DISABLE_DURATION_MS"), Preferences::ALMOST_OVER_QUOTA_UI_ALERT_DISABLE_DURATION);
    overridePreference(settings, QString::fromUtf8("TRANSFER_ALMOST_OVER_QUOTA_OS_NOTIFICATION_DISABLE_DURATION_MS"), Preferences::ALMOST_OVER_QUOTA_OS_NOTIFICATION_DISABLE_DURATION);
    overridePreference(settings, QString::fromUtf8("OVER_QUOTA_ACTION_DIALOGS_DISABLE_TIME_MS"), Preferences::OVER_QUOTA_ACTION_DIALOGS_DISABLE_TIME);

    overridePreference(settings, QString::fromUtf8("MIN_UPDATE_STATS_INTERVAL"), Preferences::MIN_UPDATE_STATS_INTERVAL);
    overridePreference(settings, QString::fromUtf8("MIN_UPDATE_CLEANING_INTERVAL_MS"), Preferences::MIN_UPDATE_CLEANING_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("MIN_UPDATE_NOTIFICATION_INTERVAL_MS"), Preferences::MIN_UPDATE_NOTIFICATION_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("MIN_REBOOT_INTERVAL_MS"), Preferences::MIN_REBOOT_INTERVAL_MS);
    overridePreference(settings, QString::fromUtf8("MIN_EXTERNAL_NODES_WARNING_MS"), Preferences::MIN_EXTERNAL_NODES_WARNING_MS);
    overridePreference(settings, QString::fromUtf8("MIN_TRANSFER_NOTIFICATION_INTERVAL_MS"), Preferences::MIN_TRANSFER_NOTIFICATION_INTERVAL_MS);

    overridePreference(settings, QString::fromUtf8("UPDATE_INITIAL_DELAY_SECS"), Preferences::UPDATE_INITIAL_DELAY_SECS);
    overridePreference(settings, QString::fromUtf8("UPDATE_RETRY_INTERVAL_SECS"), Preferences::UPDATE_RETRY_INTERVAL_SECS);
    overridePreference(settings, QString::fromUtf8("UPDATE_TIMEOUT_SECS"), Preferences::UPDATE_TIMEOUT_SECS);
    overridePreference(settings, QString::fromUtf8("MAX_LOGIN_TIME_MS"), Preferences::MAX_LOGIN_TIME_MS);
    overridePreference(settings, QString::fromUtf8("PROXY_TEST_TIMEOUT_MS"), Preferences::PROXY_TEST_TIMEOUT_MS);
    overridePreference(settings, QString::fromUtf8("MAX_IDLE_TIME_MS"), Preferences::MAX_IDLE_TIME_MS);
    overridePreference(settings, QString::fromUtf8("MAX_COMPLETED_ITEMS"), Preferences::MAX_COMPLETED_ITEMS);

    overridePreference(settings, QString::fromUtf8("MUTEX_STEALER_MS"), Preferences::MUTEX_STEALER_MS);
    overridePreference(settings, QString::fromUtf8("MUTEX_STEALER_PERIOD_MS"), Preferences::MUTEX_STEALER_PERIOD_MS);
    overridePreference(settings, QString::fromUtf8("MUTEX_STEALER_PERIOD_ONLY_ONCE"), Preferences::MUTEX_STEALER_PERIOD_ONLY_ONCE);
}

void Preferences::updateFullName()
{
    auto fullNameRequest (UserAttributes::UserAttributesManager::instance()
                      .requestAttribute<UserAttributes::FullName>(email().toUtf8().constData()));
    connect(fullNameRequest.get(), &UserAttributes::FullName::separateNamesReady,
            this, &Preferences::setFullName, Qt::UniqueConnection);

    if(fullNameRequest->isAttributeReady())
    {
        setFullName(fullNameRequest->getFirstName(), fullNameRequest->getLastName());
    }
}

void Preferences::setFullName(const QString& newFirstName, const QString& newLastName)
{
    if (newFirstName != firstName())
    {
        setFirstName(newFirstName);
    }
    if (newLastName != lastName())
    {
        setLastName(newLastName);
    }
}
