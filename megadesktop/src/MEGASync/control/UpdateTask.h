#ifndef UPDATETASK_H
#define UPDATETASK_H

#include <QCoreApplication>
#include <QApplication>
#include <QProcess>
#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QTimer>
#include <QDir>
#include <QDirIterator>
#include <QDateTime>

#include "megaapi.h"
#include "Preferences.h"

class UpdateTask : public QObject
{
    Q_OBJECT

public:
    explicit UpdateTask(mega::MegaApi *megaApi, QString appFolder, bool isPublic = false, QObject *parent = 0);
    ~UpdateTask();

protected:
   void initialCleanup();
   void finalCleanup();
   void postponeUpdate();
   void downloadFile(QString url);
   QString readNextLine(QNetworkReply *reply);
   bool processUpdateFile(QNetworkReply *reply);
   bool processFile(QNetworkReply *reply);
   bool performUpdate();
   void rollbackUpdate(int fileNum);
   void addToSignature(QString value);
   void addToSignature(QByteArray bytes);
   void initSignature();
   bool checkSignature(QString value);
   bool alreadyInstalled(QString relativePath, QString fileSignature);
   bool alreadyDownloaded(QString relativePath, QString fileSignature);
   bool alreadyExists(QString absolutePath, QString fileSignature);

   std::shared_ptr<Preferences> preferences;
   QStringList downloadURLs;
   QStringList localPaths;
   QStringList fileSignatures;
   QNetworkAccessManager *m_WebCtrl;
   mega::MegaHashSignature *signatureChecker;
   char signature[512];
   int updateVersion;
   int currentFile;
   QDir updateFolder;
   QDir backupFolder;
   QDir appFolder;
   QString basePath;
   QTimer *updateTimer;
   QTimer *timeoutTimer;
   bool forceInstall;
   bool running;
   bool forceCheck;
   bool isPublic;
   mega::MegaApi *megaApi;

signals:
   void updateCompleted();
   void updateAvailable(bool requested);
   void updateNotFound(bool requested);
   void installingUpdate(bool requested);
   void updateError();

private slots:
   void downloadFinished(QNetworkReply* reply);
   void onProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*);

public slots:
   void startUpdateThread();
   void installUpdate();
   void checkForUpdates();
   void tryUpdate();
   void onTimeout();
};

#endif // UPDATETASK_H
