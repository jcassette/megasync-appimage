#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <megaapi.h>
#include "EncryptedSettings.h"
#include "SyncInfo.h"
#include "EphemeralCredentials.h"

#include <QLocale>
#include <QStringList>
#include <QMutex>
#include <QDataStream>

#include <chrono>
#include <type_traits>

Q_DECLARE_METATYPE(QList<long long>)

class SyncSettings;
struct SyncData;
class Preferences : public QObject
{
    Q_OBJECT

signals:
    void stateChanged();
    void updated(int lastVersion);
    void valueChanged(QString key);

private:
    Preferences();

public:
    //NOT thread-safe. Must be called before creating threads.
    static std::shared_ptr<Preferences> instance();
    Preferences(const Preferences&) = delete;
    Preferences& operator=(const Preferences&) = delete;
    void initialize(QString dataPath);
    void setEmailAndGeneralSettings(const QString &email);
    void monitorUserAttributes();

    //Thread safe functions
    virtual bool logged(); //true if a full login+fetchnodes has completed (now or in previous executions)
    bool hasEmail(QString email);
    QString email();
    void setEmail(QString email);
    QString firstName();
    void setFirstName(QString firstName);
    QString lastName();
    void setLastName(QString lastName);
    QString fileHash(const QString& filePath);
    void setFileHash(const QString& filePath, const QString& fileHash);
    void setSession(QString session);
    void setSessionInUserGroup(QString session);
    QString getSession();
    void removeEphemeralCredentials();
    void setEphemeralCredentials(const EphemeralCredentials &cred);
    EphemeralCredentials getEphemeralCredentials();

    unsigned long long transferIdentifier();
    long long lastTransferNotificationTimestamp();
    void setLastTransferNotificationTimestamp();
    long long totalStorage();
    void setTotalStorage(long long value);
    long long usedStorage();
    void setUsedStorage(long long value);
    long long availableStorage();

    long long cloudDriveStorage();
    void setCloudDriveStorage(long long value);
    long long vaultStorage();
    void setVaultStorage(long long value);
    long long rubbishStorage();
    void setRubbishStorage(long long value);
    long long inShareStorage();
    void setInShareStorage(long long value);
    long long versionsStorage();
    void setVersionsStorage(long long value);

    long long cloudDriveFiles();
    void setCloudDriveFiles(long long value);
    long long vaultFiles();
    void setVaultFiles(long long value);
    long long rubbishFiles();
    void setRubbishFiles(long long value);
    long long inShareFiles();
    void setInShareFiles(long long value);

    long long cloudDriveFolders();
    void setCloudDriveFolders(long long value);
    long long vaultFolders();
    void setVaultFolders(long long value);
    long long rubbishFolders();
    void setRubbishFolders(long long value);
    long long inShareFolders();
    void setInShareFolders(long long value);

    long long totalBandwidth();
    void setTotalBandwidth(long long value);
    int bandwidthInterval();
    void setBandwidthInterval(int value);
    bool isTemporalBandwidthValid();
    long long getMsDiffTimeWithSDK();
    void setDsDiffTimeWithSDK(long long diffTime);

    long long getOverStorageDialogExecution();
    void setOverStorageDialogExecution(long long timestamp);
    long long getOverStorageNotificationExecution();
    void setOverStorageNotificationExecution(long long timestamp);
    long long getAlmostOverStorageNotificationExecution();
    void setAlmostOverStorageNotificationExecution(long long timestamp);
    long long getPayWallNotificationExecution();
    void setPayWallNotificationExecution(long long timestamp);
    long long getAlmostOverStorageDismissExecution();
    void setAlmostOverStorageDismissExecution(long long timestamp);
    long long getOverStorageDismissExecution();
    void setOverStorageDismissExecution(long long timestamp);

    virtual std::chrono::system_clock::time_point getTransferOverQuotaDialogLastExecution();
    virtual void setTransferOverQuotaDialogLastExecution(std::chrono::system_clock::time_point timepoint);
    virtual std::chrono::system_clock::time_point getTransferOverQuotaOsNotificationLastExecution();
    virtual void setTransferOverQuotaOsNotificationLastExecution(std::chrono::system_clock::time_point timepoint);
    virtual std::chrono::system_clock::time_point getTransferOverQuotaUiAlertLastExecution();
    virtual void setTransferOverQuotaUiAlertLastExecution(std::chrono::system_clock::time_point timepoint);
    virtual std::chrono::system_clock::time_point getTransferAlmostOverQuotaOsNotificationLastExecution();
    virtual void setTransferAlmostOverQuotaOsNotificationLastExecution(std::chrono::system_clock::time_point timepoint);
    virtual std::chrono::system_clock::time_point getTransferAlmostOverQuotaUiAlertLastExecution();
    void setTransferAlmostOverQuotaUiAlertLastExecution(std::chrono::system_clock::time_point timepoint);

    virtual std::chrono::system_clock::time_point getTransferOverQuotaSyncDialogLastExecution();
    virtual void setTransferOverQuotaSyncDialogLastExecution(std::chrono::system_clock::time_point timepoint);
    virtual std::chrono::system_clock::time_point getTransferOverQuotaDownloadsDialogLastExecution();
    virtual void setTransferOverQuotaDownloadsDialogLastExecution(std::chrono::system_clock::time_point timepoint);
    virtual std::chrono::system_clock::time_point getTransferOverQuotaImportLinksDialogLastExecution();
    virtual void setTransferOverQuotaImportLinksDialogLastExecution(std::chrono::system_clock::time_point timepoint);
    virtual std::chrono::system_clock::time_point getTransferOverQuotaStreamDialogLastExecution();
    virtual void setTransferOverQuotaStreamDialogLastExecution(std::chrono::system_clock::time_point timepoint);
    std::chrono::system_clock::time_point getStorageOverQuotaUploadsDialogLastExecution();
    void setStorageOverQuotaUploadsDialogLastExecution(std::chrono::system_clock::time_point timepoint);
    std::chrono::system_clock::time_point getStorageOverQuotaSyncsDialogLastExecution();
    void setStorageOverQuotaSyncsDialogLastExecution(std::chrono::system_clock::time_point timepoint);

    int getStorageState();
    void setStorageState(int value);
    int getBusinessState();
    void setBusinessState(int value);
    int getBlockedState();
    void setBlockedState(int value);

    //**** Notifications ****/
    enum class NotificationsTypes
    {
        GENERAL_SWITCH_NOTIFICATIONS = 0,
        NEW_FOLDERS_SHARED_WITH_ME,
        NODES_SHARED_WITH_ME_CREATED_OR_REMOVED,
        FOLDERS_SHARED_WITH_ME_DELETED,
        NEW_CONTACT_REQUESTS,
        PENDING_CONTACT_REQUEST_REMINDER,
        CONTACT_ESTABLISHED,
        INFO_MESSAGES,
        COMPLETED_UPLOADS_DOWNLOADS,
        LAST
    };
    Q_ENUM(NotificationsTypes)

    static constexpr std::underlying_type<NotificationsTypes>::type
        notificationsTypeUT(NotificationsTypes type) noexcept{
        return static_cast<std::underlying_type<NotificationsTypes>::type>(type);
    }

    bool isNotificationEnabled(NotificationsTypes type, bool includingGeneralSwitch = true);
    bool isAnyNotificationEnabled(bool includingGeneralSwitch = true);
    bool isGeneralSwitchNotificationsOn();
    void enableNotifications(NotificationsTypes type, bool value);
    void recoverDeprecatedNotificationsSettings();

    static QString notificationsTypeToString(NotificationsTypes type);
    //**** END OF Notifications ****/

    //**** Stalled Issues ****/
    enum class StalledIssuesModeType
    {
        None = 0,
        Smart,
        Advance
    };
    StalledIssuesModeType stalledIssuesMode();
    void setStalledIssuesMode(StalledIssuesModeType value);
    bool isStalledIssueSmartModeActivated();

    QDate stalledIssuesEventLastDate();
    void updateStalledIssuesEventLastDate();

    //**** END OF Stalled Issues ****/

    void setTemporalBandwidthValid(bool value);
    long long temporalBandwidth();
    void setTemporalBandwidth(long long value);
    int temporalBandwidthInterval();
    void setTemporalBandwidthInterval(int value);
    long long usedBandwidth();
    void setUsedBandwidth(long long value);
    int accountType();
    void setAccountType(int value);
    long long proExpirityTime();
    void setProExpirityTime(long long value);
    bool startOnStartup();
    void setStartOnStartup(bool value);
    bool usingHttpsOnly();
    void setUseHttpsOnly(bool value);
    bool SSLcertificateException();
    void setSSLcertificateException(bool value);
    QString language();
    void setLanguage(QString &value);
    bool updateAutomatically();
    void setUpdateAutomatically(bool value);
    bool hasDefaultUploadFolder();
    bool hasDefaultDownloadFolder();
    void setHasDefaultUploadFolder(bool value);
    void setHasDefaultDownloadFolder(bool value);
    bool canUpdate(QString filePath);
    int uploadLimitKB();
    int downloadLimitKB();
    void setUploadLimitKB(int value);
    void setDownloadLimitKB(int value);
    int parallelUploadConnections();
    int parallelDownloadConnections();
    void setParallelUploadConnections(int value);
    void setParallelDownloadConnections(int value);
    unsigned long long upperSizeLimitValue();
    void setUpperSizeLimitValue(unsigned long long value);
    unsigned long long lowerSizeLimitValue();
    void setLowerSizeLimitValue(unsigned long long value);
    bool upperSizeLimit();
    void setUpperSizeLimit(bool value);
    bool lowerSizeLimit();
    void setLowerSizeLimit(bool value);
    int upperSizeLimitUnit();
    void setUpperSizeLimitUnit(int value);
    int lowerSizeLimitUnit();
    void setLowerSizeLimitUnit(int value);     
    bool cleanerDaysLimit();
    void setCleanerDaysLimit(bool value);
    int cleanerDaysLimitValue();
    void setCleanerDaysLimitValue(int value);
    int folderPermissionsValue();
    void setFolderPermissionsValue(int permissions);
    int filePermissionsValue();
    void setFilePermissionsValue(int permissions);
    int proxyType();
    void setProxyType(int value);
    int proxyProtocol();
    void setProxyProtocol(int value);
    QString proxyServer();
    void setProxyServer(const QString &value);
    unsigned short proxyPort();
    void setProxyPort(unsigned short value);
    bool proxyRequiresAuth();
    void setProxyRequiresAuth(bool value);
    QString getProxyUsername();
    void setProxyUsername(const QString &value);
    QString getProxyPassword();
    void setProxyPassword(const QString &value);
    QString proxyHostAndPort();
    long long lastExecutionTime();
    void setLastExecutionTime(long long time);
    long long installationTime();
    void setInstallationTime(long long time);
    long long accountCreationTime();
    void setAccountCreationTime(long long time);
    long long hasLoggedIn();
    void setHasLoggedIn(long long time);
    bool isFirstStartDone();
    void setFirstStartDone(bool value = true);
    bool isFirstSyncDone();
    void setFirstSyncDone(bool value = true);
    bool isFirstBackupDone();
    void setFirstBackupDone(bool value = true);
    bool isFirstFileSynced();
    void setFirstFileSynced(bool value = true);
    bool isFirstFileBackedUp();
    void setFirstFileBackedUp(bool value = true);
    bool isFirstWebDownloadDone();
    void setFirstWebDownloadDone(bool value = true);
    bool isFatWarningShown();
    void setFatWarningShown(bool value = true);
    QString lastCustomStreamingApp();
    void setLastCustomStreamingApp(const QString &value);
    unsigned long long getMaxMemoryUsage();
    void setMaxMemoryUsage(unsigned long long value);
    long long getMaxMemoryReportTime();
    void setMaxMemoryReportTime(long long timestamp);
    long long lastDailyStatTime();
    void setLastDailyStatTime(long long time);

    long long lastUpdateTime();
    void setLastUpdateTime(long long time);
    int lastUpdateVersion();
    void setLastUpdateVersion(int version);

    QString downloadFolder();
    void setDownloadFolder(QString value);
    mega::MegaHandle uploadFolder();
    void setUploadFolder(mega::MegaHandle value);
    mega::MegaHandle importFolder();
    void setImportFolder(mega::MegaHandle value);

    bool getImportMegaLinksEnabled();
    void setImportMegaLinksEnabled(const bool value);

    bool getDownloadMegaLinksEnabled();
    void setDownloadMegaLinksEnabled(const bool value);

    bool neverCreateLink();
    void setNeverCreateLink(bool value);

    // sync related
    void writeSyncSetting(std::shared_ptr<SyncSettings> syncSettings); //write sync into cache
    void removeAllSyncSettings(); //remove all sync from cache
    void removeSyncSetting(std::shared_ptr<SyncSettings> syncSettings); //remove one sync from cache
    QMap<mega::MegaHandle, std::shared_ptr<SyncSettings> > getLoadedSyncsMap() const; //return loaded syncs when loggedin/entered user
    void removeAllFolders(); //remove all syncs from cache
    // old cache transition related:
    void removeOldCachedSync(int position, QString email = QString());
    //get a list of cached syncs (withouth loading them in memory): intended for transition to sdk caching them.
    QList<SyncData> readOldCachedSyncs(int *cachedBusinessState = nullptr, int *cachedBlockedState = nullptr,
                                       int *cachedStorageState = nullptr, QString email = QString());
    void saveOldCachedSyncs(); //save the old cache (intended to clean them)

    QStringList getExcludedSyncNames();
    QStringList getExcludedSyncPaths();
    // preloads excluded sync names and adds missing defaults ones in previous versions
    void loadExcludedSyncNames();

    bool isOneTimeActionDone(int action);
    void setOneTimeActionDone(int action, bool done);
    void setSystemTrayPromptSuppressed(bool suppressed);
    bool isSystemTrayPromptSuppressed();
    void setAskOnExclusionRemove(bool value);
    bool isAskOnExclusionRemove();
    void setSystemTrayLastPromptTimestamp(long long timestamp);
    long long getSystemTrayLastPromptTimestamp();

    bool isOneTimeActionUserDone(int action);
    void setOneTimeActionUserDone(int action, bool done);

    QStringList getPreviousCrashes();
    void setPreviousCrashes(QStringList crashes);
    long long getLastReboot();
    void setLastReboot(long long value);
    long long getLastExit();
    void setLastExit(long long value);
    QSet<mega::MegaHandle> getDisabledSyncTags();
    void setDisabledSyncTags(QSet<mega::MegaHandle> disabledSyncs);
    bool getNotifyDisabledSyncsOnLogin();
    void setNotifyDisabledSyncsOnLogin(bool notify);

    void getLastHandleInfo(mega::MegaHandle &lastHandle, int &type, long long &timestamp);
    void setLastPublicHandle(mega::MegaHandle handle, int type);

    int getNumUsers();

    // enter user preferences and load syncs into loadedSyncsMap
    void enterUser(int i);
    bool enterUser(QString account);

    // leave user
    void leaveUser();

    int accountStateInGeneral();
    void setAccountStateInGeneral(int value);

    bool needsFetchNodesInGeneral();
    void setNeedsFetchNodesInGeneral(bool value);

    void unlink();
    void resetGlobalSettings();//Clear and remove any global setting. Not account specific ones.

    bool isCrashed();
    void setCrashed(bool value);
    bool getGlobalPaused();
    void setGlobalPaused(bool value);
    bool getUploadsPaused();
    void setUploadsPaused(bool value);
    bool getDownloadsPaused();
    void setDownloadsPaused(bool value);

    long long lastStatsRequest();
    void setLastStatsRequest(long long value);

    bool awakeIfActiveEnabled();
    void setAwakeIfActive(bool value);

    bool fileVersioningDisabled();
    void disableFileVersioning(bool value);
    bool overlayIconsDisabled();
    void disableOverlayIcons(bool value);
    bool leftPaneIconsDisabled();
    void disableLeftPaneIcons(bool value);
    bool error();

    QString getDataPath();
    QString getTempTransfersPath();
    void clearTempTransfersPath();
    void clearTemporalBandwidth();
    void clearAll();
    void sync();

    enum class ThemeType
    {
        LIGHT_THEME = 0,
        DARK_THEME = 1,
        LAST
    };
    Q_ENUM(ThemeType)

    void setThemeType(ThemeType theme);
    ThemeType getThemeType();

    enum {
        PROXY_TYPE_NONE = 0,
        PROXY_TYPE_AUTO   = 1,
        PROXY_TYPE_CUSTOM = 2
    };

    enum {
        PROXY_PROTOCOL_HTTP = 0,
        PROXY_PROTOCOL_SOCKS5H = 1
    };

    enum {
        BYTE_UNIT = 0,
        KILO_BYTE_UNIT = 1,
        MEGA_BYTE_UNIT = 2,
        GIGA_BYTE_UNIT = 3
    };

    enum AccountType {
        ACCOUNT_TYPE_FREE = 0,
        ACCOUNT_TYPE_PROI = 1,
        ACCOUNT_TYPE_PROII = 2,
        ACCOUNT_TYPE_PROIII = 3,
        ACCOUNT_TYPE_LITE = 4,
        ACCOUNT_TYPE_STARTER = 11,
        ACCOUNT_TYPE_BASIC = 12,
        ACCOUNT_TYPE_ESSENTIAL = 13,
        ACCOUNT_TYPE_BUSINESS = 100,
        ACCOUNT_TYPE_PRO_FLEXI = 101
    };

    enum {
        ONE_TIME_ACTION_DEPRECATED_OPERATING_SYSTEM = 0,
        ONE_TIME_ACTION_NO_SYSTRAY_AVAILABLE = 1,
        ONE_TIME_ACTION_ACTIVE_FINDER_EXT = 2,
        ONE_TIME_ACTION_REGISTER_UPDATE_TASK = 3,
        ONE_TIME_ACTION_OS_TOO_OLD = 4,
        ONE_TIME_ACTION_HGFS_WARNING = 5,
        ONE_TIME_ACTION_ONBOARDING_SHOWN
    };

    enum {
        STATE_BELOW_OVER_STORAGE = 0,
        STATE_ALMOST_OVER_STORAGE,
        STATE_OVER_STORAGE,
        STATE_OVER_STORAGE_DISMISSED,
        STATE_PAYWALL
    };

    enum {
        STATE_NOT_INITIATED = 0,
        STATE_LOGGED_OK = 1,
        STATE_LOGGED_FAILED = 2,
        STATE_FETCHNODES_OK = 3,
        STATE_FETCHNODES_FAILED = 4
    };

    static const int MAX_FILES_IN_NEW_SYNC_FOLDER;
    static const int MAX_FOLDERS_IN_NEW_SYNC_FOLDER;

    static long long MIN_UPDATE_STATS_INTERVAL;
    static long long OQ_DIALOG_INTERVAL_MS;
    static long long OQ_NOTIFICATION_INTERVAL_MS;
    static long long ALMOST_OQ_UI_MESSAGE_INTERVAL_MS;
    static long long OQ_UI_MESSAGE_INTERVAL_MS;
    static long long PAYWALL_NOTIFICATION_INTERVAL_MS;
    static long long USER_INACTIVITY_MS;
    static long long MIN_UPDATE_CLEANING_INTERVAL_MS;

    static std::chrono::milliseconds OVER_QUOTA_DIALOG_DISABLE_DURATION;
    static std::chrono::milliseconds OVER_QUOTA_OS_NOTIFICATION_DISABLE_DURATION;
    static std::chrono::milliseconds OVER_QUOTA_UI_ALERT_DISABLE_DURATION;
    static std::chrono::milliseconds ALMOST_OVER_QUOTA_UI_ALERT_DISABLE_DURATION;
    static std::chrono::milliseconds ALMOST_OVER_QUOTA_OS_NOTIFICATION_DISABLE_DURATION;
    static std::chrono::milliseconds OVER_QUOTA_ACTION_DIALOGS_DISABLE_TIME;

    static int STATE_REFRESH_INTERVAL_MS;
    static int NETWORK_REFRESH_INTERVAL_MS;
    static int FINISHED_TRANSFER_REFRESH_INTERVAL_MS;

    static long long MIN_UPDATE_NOTIFICATION_INTERVAL_MS;
    static int UPDATE_INITIAL_DELAY_SECS;
    static int UPDATE_RETRY_INTERVAL_SECS;
    static int UPDATE_TIMEOUT_SECS;
    static int MAX_LOGIN_TIME_MS;

    static long long MIN_REBOOT_INTERVAL_MS;
    static long long MIN_EXTERNAL_NODES_WARNING_MS;
    static long long MIN_TRANSFER_NOTIFICATION_INTERVAL_MS;

    static int PROXY_TEST_TIMEOUT_MS;
    static unsigned int MAX_IDLE_TIME_MS;
    static unsigned int MAX_COMPLETED_ITEMS;

    static unsigned int MUTEX_STEALER_MS; //to create a task that steals the sdk mutex for a while (how long)
    static unsigned int MUTEX_STEALER_PERIOD_MS; //periodicity (how often)
    static unsigned int MUTEX_STEALER_PERIOD_ONLY_ONCE; //if only done once

    static const QString UPDATE_CHECK_URL;
    static const QString CRASH_REPORT_URL;
    static const QString UPDATE_FOLDER_NAME;
    static const QString UPDATE_BACKUP_FOLDER_NAME;
    static const QString PROXY_TEST_URL;
    static const QString PROXY_TEST_SUBSTRING;
    static const char UPDATE_PUBLIC_KEY[];

    static const char CLIENT_KEY[];
    static const QString USER_AGENT;
    static const int VERSION_CODE;
    static const int BUILD_ID;
    static const QString VERSION_STRING;
    static QString SDK_ID;
    static const QString CHANGELOG;
    static const QString TRANSLATION_FOLDER;
    static const QString TRANSLATION_PREFIX;
    static const unsigned short HTTP_PORT;

    static const QStringList HTTPS_ALLOWED_ORIGINS;
    static bool HTTPS_ORIGIN_CHECK_ENABLED;
    static const QString FINDER_EXT_BUNDLE_ID;
    static QString BASE_URL;
    static QString BASE_MEGA_IO_URL;
    static QString BASE_MEGA_HELP_URL;

    static void setBaseUrl(const QString &value);
    template<typename T>
    static void overridePreference(const QSettings &settings, QString &&name, T &value);
    static void overridePreferences(const QSettings &settings);

    //Public keys for valueChanged signals
    static const QString stalledIssuesModeKey;
    static const QString wasPausedKey;

    //In this section, you need to move the keys to make them accessible from outside
    static const int minSyncStateChangeProcessingIntervalMs;

protected:
    QMutex mutex;
    void login(QString account);
    void logout();


    // sync related:
    void readFolders(); //read sync stored configuration

    void storeSessionInGeneral(QString session);
    QString getSessionInGeneral();

    std::chrono::system_clock::time_point getTimePoint(const QString& key);
    void setTimePoint(const QString& key, const std::chrono::system_clock::time_point& timepoint);
    template<typename T>
    T getValue (const QString &key);
    template<typename T>
    T getValue (const QString &key, const T &defaultValue);
    template<typename T>
    T getValueConcurrent(const QString &key);
    template<typename T>
    T getValueConcurrent(const QString &key, const T &defaultValue);
    void setAndCachedValue(const QString &key, const QVariant &value);
    //For this method, use notifyChange = true if someone needs to detect when it changes
    //Not all prefeerences need this, that´s why by default it is set to false
    void setValueConcurrently(const QString &key, const QVariant &value, bool notifyChange = false);

    std::unique_ptr<EncryptedSettings> mSettings;

    // sync configuration from old syncs
    QList<SyncData> oldSyncs;

    // loaded syncs when loggedin/entered user. This is intended to be used to load values that are not stored in the sdk (like sync name/last known remote path)
    // the actual SyncSettings model is stored in Model::configuredSyncsMap. That one is the one that will be updated and persistent accordingly
    // These are only used for retrieving values or removing at uninstall
    QMap<mega::MegaHandle, std::shared_ptr<SyncSettings>> loadedSyncsMap;

    QStringList excludedSyncNames;
    QStringList excludedSyncPaths;
    bool errorFlag;
    long long tempBandwidth;
    int tempBandwidthInterval;
    bool isTempBandwidthValid;
    QString mDataPath;
    long long diffTimeWithSDK;
    std::chrono::system_clock::time_point transferOverQuotaDialogDisabledUntil;
    std::chrono::system_clock::time_point transferOverQuotaOsNotificationDisabledUntil;
    std::chrono::system_clock::time_point transferAlmostOverQuotaOsNotificationDisabledUntil;
    std::chrono::system_clock::time_point transferAlmostOverQuotaUiAlertDisabledUntil;
    std::chrono::system_clock::time_point transferOverQuotaUiAlertDisableUntil;
    long long lastTransferNotification;
    std::chrono::system_clock::time_point transferOverQuotaSyncDialogDisabledUntil;
    std::chrono::system_clock::time_point transferOverQuotaDownloadsDialogDisabledUntil;
    std::chrono::system_clock::time_point transferOverQuotaImportLinksDialogDisabledUntil;
    std::chrono::system_clock::time_point transferOverQuotaStreamDialogDisabledUntil;
    std::chrono::system_clock::time_point storageOverQuotaUploadsDialogDisabledUntil;
    std::chrono::system_clock::time_point storageOverQuotaSyncsDialogDisabledUntil;

    static const QString currentAccountKey;
    static const QString currentAccountStatusKey;
    static const QString needsFetchNodesKey;
    static const QString syncsGroupKey;
    static const QString syncsGroupByTagKey;
    static const QString emailKey;
    static const QString firstNameKey;
    static const QString lastNameKey;
    static const QString totalStorageKey;
    static const QString usedStorageKey;
    static const QString cloudDriveStorageKey;
    static const QString vaultStorageKey;
    static const QString rubbishStorageKey;
    static const QString inShareStorageKey;
    static const QString versionsStorageKey;
    static const QString cloudDriveFilesKey;
    static const QString vaultFilesKey;
    static const QString rubbishFilesKey;
    static const QString inShareFilesKey;
    static const QString cloudDriveFoldersKey;
    static const QString vaultFoldersKey;
    static const QString rubbishFoldersKey;
    static const QString inShareFoldersKey;
    static const QString totalBandwidthKey;
    static const QString usedBandwidthKey;
    static const QString usedBandwidthIntervalKey;
    static const QString overStorageDialogExecutionKey;
    static const QString overStorageNotificationExecutionKey;
    static const QString almostOverStorageNotificationExecutionKey;
    static const QString payWallNotificationExecutionKey;
    static const QString almostOverStorageDismissExecutionKey;
    static const QString overStorageDismissExecutionKey;
    static const QString transferOverQuotaDialogLastExecutionKey;
    static const QString transferOverQuotaOsNotificationLastExecutionKey;
    static const QString transferAlmostOverQuotaOsNotificationLastExecutionKey;
    static const QString transferAlmostOverQuotaUiAlertLastExecutionKey;
    static const QString transferOverQuotaUiAlertLastExecutionKey;
    static const QString transferOverQuotaWaitUntilKey;
    static const QString storageStateQKey;
    static const QString transferOverQuotaSyncDialogLastExecutionKey;
    static const QString transferOverQuotaDownloadsDialogLastExecutionKey;
    static const QString transferOverQuotaImportLinksDialogLastExecutionKey;
    static const QString transferOverQuotaStreamDialogLastExecutionKey;
    static const QString storageOverQuotaUploadsDialogLastExecutionKey;
    static const QString storageOverQuotaSyncsDialogLastExecutionKey;
    static const QString businessStateQKey;
    static const QString blockedStateQKey;
    static const QString accountTypeKey;
    static const QString proExpirityTimeKey;
    static const QString startOnStartupKey;
    static const QString languageKey;
    static const QString updateAutomaticallyKey;
    static const QString uploadLimitKBKey;
    static const QString downloadLimitKBKey;
    static const QString parallelUploadConnectionsKey;
    static const QString parallelDownloadConnectionsKey;
    static const QString upperSizeLimitKey;
    static const QString lowerSizeLimitKey;
    static const QString upperSizeLimitValueKey;
    static const QString lowerSizeLimitValueKey;
    static const QString upperSizeLimitUnitKey;
    static const QString lowerSizeLimitUnitKey;
    static const QString cleanerDaysLimitKey;
    static const QString cleanerDaysLimitValueKey;
    static const QString folderPermissionsKey;
    static const QString filePermissionsKey;
    static const QString proxyTypeKey;
    static const QString proxyProtocolKey;
    static const QString proxyServerKey;
    static const QString proxyPortKey;
    static const QString proxyRequiresAuthKey;
    static const QString proxyUsernameKey;
    static const QString proxyPasswordKey;
    static const QString configuredSyncsKey;
    static const QString syncNameKey;
    static const QString syncIdKey;
    static const QString localFolderKey;
    static const QString megaFolderKey;
    static const QString megaFolderHandleKey;
    static const QString folderActiveKey;
    static const QString temporaryInactiveKey;
    static const QString downloadFolderKey;
    static const QString uploadFolderKey;
    static const QString hasDefaultUploadFolderKey;
    static const QString hasDefaultDownloadFolderKey;
    static const QString importFolderKey;
    static const QString localFingerprintKey;
    static const QString lastExecutionTimeKey;
    static const QString excludedSyncNamesKey;
    static const QString excludedSyncPathsKey;
    static const QString lastVersionKey;
    static const QString isCrashedKey;
    static const QString lastStatsRequestKey;
    static const QString wasUploadsPausedKey;
    static const QString wasDownloadsPausedKey;
    static const QString lastUpdateTimeKey;
    static const QString lastUpdateVersionKey;
    static const QString previousCrashesKey;
    static const QString lastRebootKey;
    static const QString lastExitKey;
    static const QString disableOverlayIconsKey;
    static const QString disableFileVersioningKey;
    static const QString disableLeftPaneIconsKey;
    static const QString sessionKey;
    static const QString ephemeralSessionKey;
    static const QString firstStartDoneKey;
    static const QString firstSyncDoneKey;
    static const QString firstBackupDoneKey;
    static const QString firstFileSyncedKey;
    static const QString firstFileBackedUpKey;
    static const QString firstWebDownloadKey;
    static const QString fatWarningShownKey;
    static const QString installationTimeKey;
    static const QString accountCreationTimeKey;
    static const QString hasLoggedInKey;
    static const QString transferDownloadMethodKey;
    static const QString transferUploadMethodKey;
    static const QString lastCustomStreamingAppKey;
    static const QString useHttpsOnlyKey;
    static const QString SSLcertificateExceptionKey;
    static const QString maxMemoryUsageKey;
    static const QString maxMemoryReportTimeKey;
    static const QString oneTimeActionDoneKey;
    static const QString oneTimeActionUserDoneKey;
    static const QString transferIdentifierKey;
    static const QString lastPublicHandleKey;
    static const QString lastPublicHandleTimestampKey;
    static const QString lastPublicHandleTypeKey;
    static const QString disabledSyncsKey;
    static const QString neverCreateLinkKey;
    static const QString notifyDisabledSyncsKey;
    static const QString importMegaLinksEnabledKey;
    static const QString downloadMegaLinksEnabledKey;
    static const QString systemTrayPromptSuppressed;
    static const QString systemTrayLastPromptTimestamp;
    static const QString lastDailyStatTimeKey;
    static const QString askOnExclusionRemove;
    static const QString themeKey;

    //Sleep mode
    static const QString awakeIfActiveKey;
    static const bool defaultAwakeIfActive;

    //Notifications Default value
    static const bool defaultShowNotifications;

    //Only for retrocompatibility purposes
    static const QString showDeprecatedNotificationsKey;
    static const bool defaultDeprecatedNotifications;

    //Stalled issues smart choice
    static const StalledIssuesModeType defaultStalledIssuesMode;

    //Stalled issues event date
    static const QString stalledIssuesEventDateKey;

    static const bool defaultStartOnStartup;
    static const bool defaultUpdateAutomatically;
    static const int  defaultUploadLimitKB;
    static const int  defaultDownloadLimitKB;
    static const unsigned long long defaultTransferIdentifier;
    static const int  defaultParallelUploadConnections;
    static const int  defaultParallelDownloadConnections;
    static const int  defaultProxyType;
    static const int  defaultProxyProtocol;
    static const long long defaultTimeStamp;
    static const QString  defaultProxyServer;
    static const unsigned short defaultProxyPort;
    static const bool defaultProxyRequiresAuth;
    static const QString defaultProxyUsername;
    static const QString defaultProxyPassword;
    static const bool defaultUpperSizeLimit;
    static const bool defaultLowerSizeLimit;
    static const unsigned long long defaultUpperSizeLimitValue;
    static const unsigned long long defaultLowerSizeLimitValue;
    static const int defaultUpperSizeLimitUnit;
    static const int defaultLowerSizeLimitUnit;
    static const bool defaultCleanerDaysLimit;
    static const int defaultCleanerDaysLimitValue;
    static const int defaultTransferDownloadMethod;
    static const int defaultTransferUploadMethod;
    static const int defaultFolderPermissions;
    static const int defaultFilePermissions;
    static const bool defaultUseHttpsOnly;
    static const bool defaultSSLcertificateException;
    static const int defaultAccountStatus;
    static const bool defaultNeedsFetchNodes;
    static const bool defaultNeverCreateLink;
    static const bool defaultImportMegaLinksEnabled;
    static const bool defaultDownloadMegaLinksEnabled;
    static const bool defaultSystemTrayPromptSuppressed;
    static const bool defaultAskOnExclusionRemove;
    static const ThemeType defaultTheme;

private:
    void updateFullName();

private slots:
    void setFullName(const QString& newFirstName, const QString& newLastName);

};

#endif // PREFERENCES_H
