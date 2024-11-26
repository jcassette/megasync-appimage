#pragma once

#include "SyncSettings.h"
#include "QTMegaListener.h"

#include "megaapi.h"

#include <QString>
#include <QStringList>
#include <QSet>
#include <QMutex>
#include <QVector>
#include <QMap>
#include <QList>
#include <QTimer>

#include <memory>

class Preferences;
/**
 * @brief The Sync Model class
 *
 * The idea of this class is to hold any state variable data and proceed to persist that
 * data when required. It is intended to alleviate Preferences from that burden.
 *
 * The SyncInfo covers the functionality related with updating the data it manages
 * (e.g: when a SyncConfiguration is updated, it will call platform specific callbacks that
 *  deal with that)
 *
 * The model will be updated when required and trigger events when changes occur that should
 * be noted by UI classes.
 *
 */

class SyncInfo : public QObject, public mega::MegaListener
{
    Q_OBJECT

signals:
    void syncStateChanged(std::shared_ptr<SyncSettings> syncSettings);
    void syncStatsUpdated(std::shared_ptr<::mega::MegaSyncStats>);
    void syncRemoved(std::shared_ptr<SyncSettings> syncSettings);
    void syncDisabledListUpdated();

private:
    static std::unique_ptr<SyncInfo> model;
    SyncInfo();

    std::shared_ptr<Preferences> preferences;
    bool mIsFirstTwoWaySyncDone;
    bool mIsFirstBackupDone;

    void saveUnattendedDisabledSyncs();
    void checkUnattendedDisabledSyncsForErrors();

    QTimer mShowErrorTimer;
    int mLastError = mega::MegaSync::NO_SYNC_ERROR;

public:
    using SyncType = mega::MegaSync::SyncType;

    // Data for display in Settings dialog Syncs/Backups.
    // Model data that needs to be kept even before the window is shown
    std::map<::mega::MegaHandle, std::shared_ptr<::mega::MegaSyncStats>> mSyncStatsMap;

    enum SyncOrigin
    {
        NONE = 0,
        ONBOARDING_ORIGIN,
        MAIN_APP_ORIGIN
    };
    Q_ENUM(SyncOrigin)

protected:
    mutable QMutex syncMutex;

    SyncOrigin mSyncToCreateOrigin;

    QMap<SyncType, QList<mega::MegaHandle>> configuredSyncs; //Tags of configured syncs
    QMap<mega::MegaHandle, std::shared_ptr<SyncSettings>> configuredSyncsMap;
    QMap<mega::MegaHandle, std::shared_ptr<SyncSettings>> syncsSettingPickedFromOldConfig;
    QMap<SyncType, QSet<mega::MegaHandle>> unattendedDisabledSyncs; //Tags of syncs disabled due to errors since last dismissed
    std::unique_ptr<mega::QTMegaListener> delegateListener;

public:
    static const QVector<SyncType> AllHandledSyncTypes;

    void reset();
    static SyncInfo *instance();
    /**
     * @brief Updates sync model
     * @param sync MegaSync object with the required information
     * @param addingState to distinguish adding cases (from onSyncAdded)
     * @return
     */
    std::shared_ptr<SyncSettings> updateSyncSettings(mega::MegaSync *sync);

    // transition sync to active: will trigger platform dependent behaviour
    void activateSync(std::shared_ptr<SyncSettings> cs);
    // transition sync to inactive: will trigger platform dependent behaviour
    void deactivateSync(std::shared_ptr<SyncSettings> cs);

    // store all megasync specific info of syncsettings into megasync cache
    void rewriteSyncSettings();

    // load into sync model the information from an old cached sync
    void pickInfoFromOldSync(const SyncData &osd, mega::MegaHandle backupId, bool loadedFromPreviousSessions);

    // remove syncs from model
    void removeSyncedFolderByBackupId(mega::MegaHandle backupId);
    void removeAllFolders();

    // Getters
    std::shared_ptr<SyncSettings> getSyncSetting(int num, SyncType type);
    std::shared_ptr<SyncSettings> getSyncSettingByTag(mega::MegaHandle tag) const;
    QList<std::shared_ptr<SyncSettings>> getSyncSettingsByType(const QVector<SyncType>& types);
    QList<std::shared_ptr<SyncSettings>> getSyncSettingsByType(SyncType type)
        {return getSyncSettingsByType(QVector<SyncType>({type}));}
    QList<std::shared_ptr<SyncSettings>> getAllSyncSettings()
        {return getSyncSettingsByType(AllHandledSyncTypes);}

        bool hasSyncs();

        int getNumSyncedFolders(const QVector<mega::MegaSync::SyncType>& types);

        int getNumSyncedFolders(SyncType type)
        {
            return getNumSyncedFolders(QVector<SyncType>({type}));
        }

    bool syncWithErrorExist(const QVector<SyncType>& types);
    bool syncWithErrorExist(SyncType type)
        {return syncWithErrorExist(QVector<SyncType>({type}));}
    bool hasUnattendedDisabledSyncs(const QVector<SyncType>& types) const;
    bool hasUnattendedDisabledSyncs(SyncType type) const
        {return hasUnattendedDisabledSyncs(QVector<SyncType>({type}));}
    const QSet<mega::MegaHandle> getUnattendedDisabledSyncs(const SyncType& type) const;
    void addUnattendedDisabledSync(mega::MegaHandle tag, SyncType type);
    void removeUnattendedDisabledSync(mega::MegaHandle tag, SyncType type);
    void setUnattendedDisabledSyncs(const QSet<mega::MegaHandle>& tags);
    void dismissUnattendedDisabledSyncs(const QVector<SyncType>& types);
    void dismissUnattendedDisabledSyncs(SyncType type)
        {return dismissUnattendedDisabledSyncs(QVector<SyncType>({type}));}

    QStringList getSyncNames(const QVector<SyncType>& types);
    QStringList getSyncNames(SyncType type)
        {return getSyncNames(QVector<SyncType>({type}));}
    QStringList getSyncIDs(const QVector<SyncType>& types);
    QStringList getSyncIDs(SyncType type)
        {return getSyncIDs(QVector<SyncType>({type}));}
    QStringList getMegaFolders(const QVector<SyncType>& types);
    QStringList getMegaFolders(SyncType type)
        {return getMegaFolders(QVector<SyncType>({type}));}
    QStringList getLocalFolders(const QVector<SyncType>& types);
    QStringList getLocalFolders(SyncType type)
        {return getLocalFolders(QVector<SyncType>({type}));}
    QMap<QString, SyncType> getLocalFoldersAndTypeMap(bool normalized = false);
    QList<mega::MegaHandle> getMegaFolderHandles(const QVector<SyncType>& types);
    QList<mega::MegaHandle> getMegaFolderHandles(SyncType type)
        {return getMegaFolderHandles(QVector<SyncType>({type}));}
    //cloudDrive = true: only cloud drive mega folders. If false will return only inshare syncs.
    QStringList getCloudDriveSyncMegaFolders(bool cloudDrive = true);
    static QSet<QString> getRemoteBackupFolderNames();

    void updateMegaFolder(QString newRemotePath, std::shared_ptr<SyncSettings> cs);

    void showSingleSyncDisabledNotification(std::shared_ptr<SyncSettings> syncSetting);
    void setSyncToCreateOrigin(SyncOrigin newSyncToCreate);

protected:
    void onEvent(mega::MegaApi* api, mega::MegaEvent* event) override;
    void onSyncStateChanged(mega::MegaApi *, mega::MegaSync *sync) override;
    void onSyncDeleted(mega::MegaApi *api, mega::MegaSync *sync) override;
    void onSyncAdded(mega::MegaApi *api, mega::MegaSync *sync) override;
    void onSyncFileStateChanged(mega::MegaApi *, mega::MegaSync *, std::string *localPath, int newState) override;
    void onSyncStatsUpdated(mega::MegaApi *api, mega::MegaSyncStats* syncStats) override;
};
