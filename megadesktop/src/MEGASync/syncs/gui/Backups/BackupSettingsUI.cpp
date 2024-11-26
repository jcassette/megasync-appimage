#include "BackupSettingsUI.h"

#include "BackupTableView.h"
#include "BackupItemModel.h"
#include "BackupsController.h"

#include "QmlDialogWrapper.h"
#include "Backups.h"
#include "Onboarding.h"

#include "CreateRemoveBackupsManager.h"
#include "QMegaMessageBox.h"
#include "DialogOpener.h"

#include "ui_SyncSettingsUIBase.h"

BackupSettingsUI::BackupSettingsUI(QWidget *parent) :
    SyncSettingsUIBase(parent)
{
    setBackupsTitle();
    setTable<BackupTableView, BackupItemModel, BackupsController>();

    connect(&BackupsController::instance(), &BackupsController::backupMoveOrRemoveRemoteFolderError, this, [this](std::shared_ptr<mega::MegaError> err)
    {
        onSavingSyncsCompleted(SAVING_FINISHED);
        QMegaMessageBox::MessageBoxInfo msgInfo;
        msgInfo.title = tr("Error moving or removing remote backup folder");
        msgInfo.text = tr("Failed to move or remove the remote backup folder. Reason: %1")
                .arg(QCoreApplication::translate("MegaError", err->getErrorString()));
        QMegaMessageBox::warning(msgInfo);

    });

    mElements.initElements(this);
    ui->gSyncs->setUsePermissions(false);

    if(auto dialog = DialogOpener::findDialog<QmlDialogWrapper<Onboarding>>())
    {
        setAddButtonEnabled(!dialog->getDialog()->isVisible());
        connect(dialog->getDialog(), &QmlDialogWrapper<Onboarding>::finished, this, [this]()
        {
            setAddButtonEnabled(true);
        });
    }

    if(auto dialog = DialogOpener::findDialog<QmlDialogWrapper<Backups>>())
    {
        setAddButtonEnabled(!dialog->getDialog()->isVisible());
    }
}

BackupSettingsUI::~BackupSettingsUI()
{
}

void BackupSettingsUI::addButtonClicked(mega::MegaHandle)
{
    CreateRemoveBackupsManager::addBackup(true);
}

void BackupSettingsUI::changeEvent(QEvent *event)
{    
    if(event->type() == QEvent::LanguageChange)
    {
        mElements.retranslateUI();
        ui->retranslateUi(this);
        setBackupsTitle();
    }

    SyncSettingsUIBase::changeEvent(event);
}

void BackupSettingsUI::removeSync(std::shared_ptr<SyncSettings> backup)
{
    CreateRemoveBackupsManager::removeBackup(backup, this);
}

QString BackupSettingsUI::getFinishWarningIconString() const
{
#ifdef Q_OS_MACOS
    return QString::fromUtf8("settings-backups-error");
#else
    return QString::fromUtf8(":/images/settings-backups-warn.png");
#endif
}

QString BackupSettingsUI::getFinishIconString() const
{
#ifdef Q_OS_MACOS
    return QString::fromUtf8("settings-backup");
#else
    return QString::fromUtf8(":/images/settings-backup.png");
#endif
}

QString BackupSettingsUI::getOperationFailTitle() const
{
    return tr("Sync operation failed");
}

QString BackupSettingsUI::getOperationFailText(std::shared_ptr<SyncSettings> sync)
{
    return tr("Operation on sync '%1' failed. Reason: %2")
        .arg(sync->name(),
             QCoreApplication::translate("MegaSyncError", mega::MegaSync::getMegaSyncErrorCode(sync->getError())));
}

QString BackupSettingsUI::getErrorAddingTitle() const
{
    return tr("Error adding sync");
}

QString BackupSettingsUI::getErrorRemovingTitle()const
{
    return tr("Error removing backup");
}

QString BackupSettingsUI::getErrorRemovingText(std::shared_ptr<mega::MegaError> err)
{
    return tr("Your sync can't be removed. Reason: %1")
        .arg(QCoreApplication::translate("MegaError", err->getErrorString()));
}

void BackupSettingsUI::setBackupsTitle()
{
    setTitle(tr("Backups"));
}

QString BackupSettingsUI::disableString() const
{
    return tr("Some folders haven't been backed up. For more information, hover over the red icon.");
}
