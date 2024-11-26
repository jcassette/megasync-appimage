#ifndef WINDOWSPLATFORM_H
#define WINDOWSPLATFORM_H

#include "AbstractPlatform.h"

#include "platform/win/WinShellDispatcherTask.h"
#include "platform/win/WinTrayReceiver.h"

#include <QApplication>
#include <QString>
#include <QFile>
#include <QHash>
#include <QPixmap>
#include <QThread>
#include <QDir>
#include <QProcess>
#include <QMenu>

#include <queue>

class PlatformImplementation : public AbstractPlatform
{
private:

    HRESULT CreateLink(LPCWSTR lpszPathObj, LPCWSTR lpszPathLink, LPCWSTR lpszDesc, LPCWSTR pszIconfile=NULL, int iIconindex=0);
    LPTSTR getCurrentSid();

public:
    PlatformImplementation() = default;

    void initialize(int argc, char *argv[]) override;
    void prepareForSync() override;
    bool enableTrayIcon(QString executable) override;
    void notifyItemChange(const QString& localPath, int newState) override;
    void notifySyncFileChange(std::string *localPath, int newState) override;
    bool startOnStartup(bool value) override;
    bool isStartOnStartupActive() override;
    bool showInFolder(QString pathIn) override;
    void startShellDispatcher(MegaApplication *receiver) override;
    void stopShellDispatcher() override;
    void syncFolderAdded(QString syncPath, QString syncName, QString syncID) override;
    void syncFolderRemoved(QString syncPath, QString syncName, QString syncID) override;
    void notifyRestartSyncFolders() override;
    void notifyAllSyncFoldersAdded() override;
    void notifyAllSyncFoldersRemoved() override;
    QByteArray encrypt(QByteArray data, QByteArray key) override;
    QByteArray decrypt(QByteArray data, QByteArray key) override;
    QByteArray getLocalStorageKey() override;
    QString getDefaultOpenApp(QString extension) override;
    void enableDialogBlur(QDialog *dialog) override;
    bool registerUpdateJob() override;
    void uninstall() override;
    bool shouldRunHttpServer() override;
    bool isUserActive() override;
    QString getDeviceName() override;

    void addSyncToLeftPane(QString syncPath, QString syncName, QString uuid) override;
    void removeAllSyncsFromLeftPane() override;
    bool makePubliclyReadable(const QString &fileName) override;

    void streamWithApp(const QString& app, const QString& url) override;
    void processSymLinks() override;
    DriveSpaceData getDriveData(const QString &path) override;

    static void notify(const std::string& path);

    void calculateInfoDialogCoordinates(const QRect& rect, int *posx, int *posy) override;

private:
    void removeSyncFromLeftPane(QString syncPath, QString syncName, QString uuid);

    void notifyItemChange(const QString& localPath, std::shared_ptr<AbstractShellNotifier> notifier);

    QString findMimeType(const QString& extensionWithDot);
    QString findAssociatedExecutable(const QString& extensionWithDot);
    QString findAssociatedModernApp(const QString& extensionWithDot);
    QString getDefaultVideoPlayer();

    static bool isTraditionalApp(const QString& app);
    QString buildWindowsModernAppCommand(const QString& app);

    QString findAssociatedData(const ASSOCSTR type, const QString& extensionWithDot);
    static QString buildAssociatedRegKey(const QString& extensionWithDot);
    static QString buildEditCommandRegKey(const QString& appId);
    static QString readEditCommand(const QString& regKey);

    static QString readAppIdFromRegistry(const QString& regKey);
    static int getRegKeyValuesCount(HKEY regKey);
    static QString getRegKeyValue(HKEY regKey, const int valueIndex);
    static bool openRegistry(HKEY baseKey, const QString& regKeyPath, HKEY& openedKey);

    WinShellDispatcherTask *shellDispatcherTask = nullptr;
    std::shared_ptr<AbstractShellNotifier> mSyncFileNotifier = nullptr;
};

#endif // WINDOWSPLATFORM_H
