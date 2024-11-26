#ifndef BUGREPORTDIALOG_H
#define BUGREPORTDIALOG_H

#include <QPointer>
#include <QDialog>
#include "megaapi.h"
#include "QTMegaTransferListener.h"
#include "MegaSyncLogger.h"

class QProgressDialog;

namespace Ui {
class BugReportDialog;
}

class BugReportDialog : public QDialog, public mega::MegaTransferListener
{
    Q_OBJECT

public:
    explicit BugReportDialog(QWidget *parent, MegaSyncLogger& logger);
    ~BugReportDialog();

    virtual void onTransferStart(mega::MegaApi *api, mega::MegaTransfer *transfer);
    virtual void onTransferUpdate(mega::MegaApi *api, mega::MegaTransfer *transfer);
    virtual void onTransferFinish(mega::MegaApi* api, mega::MegaTransfer *transfer, mega::MegaError* error);
    virtual void onTransferTemporaryError(mega::MegaApi *api, mega::MegaTransfer *transfer, mega::MegaError* e);

    void onRequestFinish(mega::MegaRequest *request, mega::MegaError* e);

private:
    MegaSyncLogger& logger;
    Ui::BugReportDialog *ui;
    int currentTransfer;
    QPointer<QProgressDialog> mSendProgress;

    long long totalBytes;
    long long transferredBytes;
    int lastpermil;

    bool warningShown;
    bool errorShown;
    bool preparing = false;
    bool mTransferFinished;
    int mTransferError;
    QString reportFileName;
    bool mHadGlobalPause;

    const static int mMaxDescriptionLength = 3000;

protected:
    mega::MegaApi *megaApi;
    std::unique_ptr<mega::QTMegaTransferListener> mDelegateTransferListener;

    void showErrorMessage(mega::MegaError* error = nullptr);
    void postUpload();
    void createSupportTicket();

private:
    void cancelCurrentReportUpload();

private slots:
    void on_bSubmit_clicked();
    void on_bCancel_clicked();
    void cancelSendReport();
    void onDescriptionChanged();
    void onReadyForReporting();
    void on_teDescribeBug_textChanged();
};

#endif // BUGREPORTDIALOG_H
