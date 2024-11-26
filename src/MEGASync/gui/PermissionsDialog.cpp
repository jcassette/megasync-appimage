#include "PermissionsDialog.h"
#include "ui_PermissionsDialog.h"
#include <qdebug.h>

PermissionsDialog::PermissionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PermissionsDialog)
{
    ui->setupUi(this);

    ui->wFileOwner->configurePermissions(PermissionsWidget::ENABLED_EXECUTION);

    connect(ui->wFileOwner, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
    connect(ui->wFileGroup, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
    connect(ui->wFilePublic, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
    connect(ui->wFolderGroup, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
    connect(ui->wFolderPublic, SIGNAL(onPermissionChanged()), this, SLOT(permissionsChanged()));
}

PermissionsDialog::~PermissionsDialog()
{
    delete ui;
}

int PermissionsDialog::folderPermissions()
{
    return ui->lFolderPermissions->text().toInt(NULL, 8);
}

void PermissionsDialog::setFolderPermissions(int permissions)
{
    int group  = (permissions >> 3) & 0x07;
    int others = permissions & 0x07;

    ui->wFolderGroup->setDefaultPermissions(group);
    ui->wFolderPublic->setDefaultPermissions(others);
}

int PermissionsDialog::filePermissions()
{
    return ui->lFilePermissions->text().toInt(NULL, 8);
}

void PermissionsDialog::setFilePermissions(int permissions)
{
    int owner  = (permissions >> 6) & 0x07;
    int group  = (permissions >> 3) & 0x07;
    int others = permissions & 0x07;

    ui->wFileOwner->setDefaultPermissions(owner);
    ui->wFileGroup->setDefaultPermissions(group);
    ui->wFilePublic->setDefaultPermissions(others);
}

void PermissionsDialog::permissionsChanged()
{
    ui->lFolderPermissions->setText(QString::fromUtf8("7%1%2").arg(ui->wFolderGroup->getCurrentPermissions())
                                                              .arg(ui->wFolderPublic->getCurrentPermissions()));

    ui->lFilePermissions->setText(QString::fromUtf8("%1%2%3").arg(ui->wFileOwner->getCurrentPermissions())
                                                             .arg(ui->wFileGroup->getCurrentPermissions())
                                                             .arg(ui->wFilePublic->getCurrentPermissions()));
}
