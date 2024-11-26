#ifndef WINSHELLDISPATCHERTASK_H
#define WINSHELLDISPATCHERTASK_H

#include <QString>
#include <QThread>

#include "MegaApplication.h"
#include "megaapi.h"

#include <windows.h>
#include <winbase.h>
#include <Shlobj.h>
#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <Shobjidl.h>
#include <wchar.h>

#define CONNECTING_STATE 0
#define READING_STATE 1
#define WRITING_STATE 2
#define INSTANCES 21
#define PIPE_TIMEOUT 5000
#define BUFSIZE 512

typedef struct
{
   OVERLAPPED oOverlap;
   HANDLE hPipeInst;
   TCHAR chRequest[BUFSIZE];
   DWORD cbRead;
   TCHAR chReply[BUFSIZE];
   DWORD cbToWrite;
   DWORD dwState;
   BOOL fPendingIO;
} PIPEINST, *LPPIPEINST;

class WinShellDispatcherTask : public QThread
{
    Q_OBJECT

 public:
    WinShellDispatcherTask(MegaApplication *receiver);
    virtual ~WinShellDispatcherTask();
    void exitTask();

 protected:
    int dispatchPipe();
    VOID GetAnswerToRequest(LPPIPEINST pipe);
    QQueue<QString> uploadQueue;
    QQueue<QString> exportQueue;
    MegaApplication *receiver;
    QString lastPath;
    int lastState;
    int numHits;

 signals:
    void newUploadQueue(QQueue<QString> uploadQueue);
    void newExportQueue(QQueue<QString> exportQueue);
    void viewOnMega(QByteArray path, bool versions);

 protected:
   virtual void run();

};

#endif

