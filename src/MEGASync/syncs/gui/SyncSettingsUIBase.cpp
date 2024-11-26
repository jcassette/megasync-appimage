#include "SyncSettingsUIBase.h"

#include <QDateTime>

#include "DialogOpener.h"
#include "SyncTableView.h"
#include "SyncItemModel.h"
#include "SyncExclusions.h"

#ifndef Q_OS_WIN
#include <MegaApplication.h>
#include <QScreen>
#include <PermissionsDialog.h>
#endif

#include "ui_SyncSettingsUIBase.h"

QMap<mega::MegaSync::SyncType,QPointer<SyncItemModel>> SyncSettingsUIBase::mModels = QMap<mega::MegaSync::SyncType,QPointer<SyncItemModel>>();

SyncSettingsUIBase::SyncSettingsUIBase(QWidget *parent):
    QWidget(parent),
    ui(new Ui::SyncSettingsUIBase),
    mTable(nullptr),
    mParentDialog(nullptr),
    mSyncInfo(SyncInfo::instance()),
    mToolBarItem(nullptr)
{
    ui->setupUi(this);

    connect(&mOpeMegaIgnoreWatcher, &QFutureWatcher<bool>::finished, this, &SyncSettingsUIBase::onOpenMegaIgnoreFinished);

    connect(ui->gSyncs, &RemoteItemUi::addClicked, this, &SyncSettingsUIBase::addButtonClicked);
    connect(ui->gSyncs, &RemoteItemUi::deleteClicked, this, &SyncSettingsUIBase::removeSyncButtonClicked);

#ifndef Q_OS_WINDOWS
    connect(ui->gSyncs, &RemoteItemUi::permissionsClicked, this, &SyncSettingsUIBase::onPermissionsClicked);
#endif
}

void SyncSettingsUIBase::setTitle(const QString &title)
{
    ui->gSyncs->setTitle(title);
}

void SyncSettingsUIBase::insertUIElement(QWidget *widget, int position)
{
    ui->SyncSettingsLayout->insertWidget(position, widget);

    if(position == 0 && ui->SyncSettingsLayout->count() > 0)
    {
        auto secondWidget = ui->SyncSettingsLayout->itemAt(position +1)->widget();
        setTabOrder(widget,secondWidget);
    }
    else if(position > 0 && position < ui->SyncSettingsLayout->count())
    {
        auto previousWidget = ui->SyncSettingsLayout->itemAt(position-1)->widget();
        setTabOrder(previousWidget,widget);
        if(ui->SyncSettingsLayout->count() > (position + 1))
        {
            auto nextWidget = ui->SyncSettingsLayout->itemAt(position + 1)->widget();
            setTabOrder(widget, nextWidget);
        }
    }
    else if(position > 0  && position >= ui->SyncSettingsLayout->count())
    {
        auto previousWidget = ui->SyncSettingsLayout->itemAt(position-1)->widget();
        setTabOrder(previousWidget,widget);
    }
}

void SyncSettingsUIBase::onSavingSyncsCompleted(SyncStateInformation value)
{
    qint64 startTime(0);
    if(value == SyncStateInformation::SAVING_FINISHED)
    {
        startTime =  ui->wSpinningIndicatorSyncs->getStartTime();
    }
    auto closeDelay = std::max(0ll, 350ll - (QDateTime::currentMSecsSinceEpoch()
                                             - startTime));
    QTimer::singleShot(closeDelay, this, [this, value] () {
        syncsStateInformation(value);
    });
}

void SyncSettingsUIBase::syncsStateInformation(SyncStateInformation state)
{
        switch (state)
        {
            case SAVING:
                if(mParentDialog)
                {
                    emit disableParentDialog(false);
                }
                ui->wSpinningIndicatorSyncs->start();
                ui->sSyncsState->setCurrentWidget(ui->pSavingSyncs);
                break;
            case SAVING_FINISHED:
                if(mParentDialog)
                {
                    emit disableParentDialog(true);
                }
                ui->wSpinningIndicatorSyncs->stop();
                // If any sync is disabled, shows warning message
                if (mSyncInfo->syncWithErrorExist(mTable->getType()))
                {
                    ui->sSyncsState->setCurrentWidget(ui->pSyncsDisabled);

                    if(mToolBarItem)
                    {
#ifdef Q_OS_MACOS
                        QCustomMacToolbar* toolBar = dynamic_cast<QCustomMacToolbar*>(mToolBarItem->parent());
                        if(toolBar)
                        {
                            toolBar->customizeIconToolBarItem(mToolBarItem, getFinishWarningIconString());
                        }
#else
                        mToolBarItem->setIcon(QIcon(getFinishWarningIconString()));
#endif
                    }
                }
                else
                {
                    ui->sSyncsState->setCurrentWidget(ui->pNoErrorsSyncs);

                    if(mToolBarItem)
                    {
#ifdef Q_OS_MACOS
                        QCustomMacToolbar* toolBar = dynamic_cast<QCustomMacToolbar*>(mToolBarItem->parent());
                        if(toolBar)
                        {
                            toolBar->customizeIconToolBarItem(mToolBarItem, getFinishIconString());
                        }
#else
                        mToolBarItem->setIcon(QIcon(getFinishIconString()));
#endif
                    }
                }
                break;
            }
}

#ifdef Q_OS_MACOS
void SyncSettingsUIBase::setToolBarItem(QMacToolBarItem *item)
{
    mToolBarItem = item;
}
#else
void SyncSettingsUIBase::setToolBarItem(QToolButton *item)
{
    mToolBarItem = item;
}
#endif

void SyncSettingsUIBase::setAddButtonEnabled(bool enabled)
{
    ui->gSyncs->setAddButtonEnabled(enabled);
}

#ifndef Q_OS_WIN
void SyncSettingsUIBase::onPermissionsClicked()
{
    MegaSyncApp->getMegaApi()->setDefaultFolderPermissions(Preferences::instance()->folderPermissionsValue());
    int folderPermissions = MegaSyncApp->getMegaApi()->getDefaultFolderPermissions();
    MegaSyncApp->getMegaApi()->setDefaultFilePermissions(Preferences::instance()->filePermissionsValue());
    int filePermissions = MegaSyncApp->getMegaApi()->getDefaultFilePermissions();

    QPointer<PermissionsDialog> dialog = new PermissionsDialog(this);
    dialog->setFolderPermissions(folderPermissions);
    dialog->setFilePermissions(filePermissions);
    DialogOpener::showDialog<PermissionsDialog>(dialog, [dialog, &folderPermissions, &filePermissions](){
        if (dialog->result() == QDialog::Accepted)
        {
            filePermissions = dialog->filePermissions();
            folderPermissions = dialog->folderPermissions();

            if (filePermissions != Preferences::instance()->filePermissionsValue()
                    || folderPermissions != Preferences::instance()->folderPermissionsValue())
            {
                Preferences::instance()->setFilePermissionsValue(filePermissions);
                Preferences::instance()->setFolderPermissionsValue(folderPermissions);
            }
        }
    });
}
#endif

void SyncSettingsUIBase::setDisabledSyncsText()
{
    ui->lDisabledSyncs->setText(disableString());
}

void SyncSettingsUIBase::removeSyncButtonClicked()
{
    if (mTable->selectionModel()->hasSelection())
    {
        QModelIndex index = mTable->selectionModel()->selectedRows().first();
        removeSync(index.data(Qt::UserRole).value<std::shared_ptr<SyncSettings>>());
    }
}

void SyncSettingsUIBase::openExclusionsDialog(std::shared_ptr<SyncSettings> sync)
{
    QFileInfo syncDir(sync->getLocalFolder());
    if(syncDir.exists())
    {
        QPointer<QmlDialogWrapper<SyncExclusions>> exclusions = new QmlDialogWrapper<SyncExclusions>(this, sync->getLocalFolder());
        DialogOpener::showDialog(exclusions);
    }
    else
    {
        showOpenMegaIgnoreError();
    }
}

void SyncSettingsUIBase::openMegaIgnore(std::shared_ptr<SyncSettings> sync)
{
    QString ignore(sync->getLocalFolder() + QDir::separator() + QString::fromUtf8(".megaignore"));
    auto future = QtConcurrent::run(QDesktopServices::openUrl, QUrl::fromLocalFile(ignore));
    mOpeMegaIgnoreWatcher.setFuture(future);
}

void SyncSettingsUIBase::onOpenMegaIgnoreFinished()
{
    auto result = mOpeMegaIgnoreWatcher.result();
    if(!result)
    {
        showOpenMegaIgnoreError();
    }
}

void SyncSettingsUIBase::initTable()
{
    ui->gSyncs->initView(mTable);
}

void SyncSettingsUIBase::showOpenMegaIgnoreError()
{
    QMegaMessageBox::MessageBoxInfo msgInfo;
    msgInfo.parent = this;
    msgInfo.title = QMegaMessageBox::errorTitle();
    msgInfo.text = tr("Error opening megaignore file");
    QMegaMessageBox::warning(msgInfo);
}

void SyncSettingsUIBase::rescanQuick(std::shared_ptr<SyncSettings> sync)
{
    MegaSyncApp->getMegaApi()->rescanSync(sync->backupId(), false);
}

void SyncSettingsUIBase::rescanDeep(std::shared_ptr<SyncSettings> sync)
{
     MegaSyncApp->getMegaApi()->rescanSync(sync->backupId(), true);
}
